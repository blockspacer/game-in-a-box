/*
    Game-in-a-box. Simple First Person Shooter Network Game.
    Copyright (C) 2012 Richard Maxwell <jodi.the.tigger@gmail.com>

    This file is part of Game-in-a-box

    Game-in-a-box is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef USING_PRECOMPILED_HEADERS
#else
#include "Common/PrecompiledHeaders.hpp"
#endif

#include "Common/Logging/Logging.hpp"
#include "Common/IStateManager.hpp"

#include "INetworkProvider.hpp"
#include "NetworkPacket.hpp"
#include "PacketDelta.hpp"
#include "PacketDeltaClient.hpp"
#include "PacketDeltaFragmentManager.hpp"
#include "XorCode.hpp"
#include "BufferSerialisation.hpp"

#include "NetworkManagerServer.hpp"

using namespace GameInABox::Common::Network;

NetworkManagerServer::NetworkManagerServer(
        MotleyUniquePointer<INetworkProvider> network,
        IStateManager& stateManager)
    : INetworkManager()
    , myNetwork(move(network))
    , myStateManager(stateManager)
    , myAddressToState()
    , myCompressor(stateManager.GetHuffmanFrequencies())
{
}

NetworkManagerServer::~NetworkManagerServer()
{
    Disconnect();
}

void NetworkManagerServer::PrivateProcessIncomming()
{
    // NOTE: Quake 3 does an array search to match ip address to
    // connected clients. That would work fine for 8-64 players.
    // However I would assume > 64 players hashing might be better.
    // But, assumption == nullptr. Do metrics or shut up.

    // Only keep a hash of network address to connetions.
    // If we get delta packets from an unrecognised endpoint
    // check the connection array incase it's an exisiting connection
    // that's had its port changed, otherwise ignore.
    // All other packets from an unrecognised address are treated as a new
    // connection.
    std::vector<NetworkPacket> responses{};

    auto packets = myNetwork->Receive();

    // process all the packets.

    for (auto& packet: packets)
    {
        if (myAddressToState.count(packet.address) > 0)
        {
            auto &connection = myAddressToState.at(packet.address).connection;
            auto response = connection.Process(move(packet.data));

            if (!response.empty())
            {
                if (myStateManager.CanPacketSend(connection.IdClient(), response.size()))
                {
                    responses.emplace_back(move(response), packet.address);
                }
            }
        }
        else
        {
            // If it's a delta packet, see if it's an existing connection.
            /* RAM: TODO: soooo broken!
            if (PacketDeltaClient::IsPacket(packet.data))
            {
                auto delta = PacketDeltaClient{packet.data};
                if (delta.IsValid())
                {
                    auto id = delta.IdConnection();

                    if (id)
                    {
                        for (auto &addressToState : myAddressToState)
                        {
                            // same address?
                            if (addressToState.first.address() == packet.address.address())
                            {
                                auto currentId = addressToState.second.connection.IdConnection();

                                if (currentId == id)
                                {
                                    // copy the connection, don't care. Use move if metrics
                                    // say that this is too slow.
                                    myAddressToState.emplace(packet.address, addressToState.second);

                                    // remove the last one
                                    myAddressToState.erase(addressToState.first);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            else */
            {
                myAddressToState.emplace(packet.address, State{Connection{myStateManager}, Sequence{0}});

                auto &connection = myAddressToState.at(packet.address).connection;

                connection.Start(Connection::Mode::Server);

                auto response = connection.Process(move(packet.data));

                if (!response.empty())
                {
                    if (myStateManager.CanPacketSend(connection.IdClient(), response.size()))
                    {
                        responses.emplace_back(move(response), packet.address);
                    }
                }
            }
        }
    }

    if (!responses.empty())
    {
        myNetwork->Send(responses);
    }

    // Drop any disconnects, parse any deltas.
    // Disconnections are handled in privatesendstate by testing myStateManager.IsConnected().
    for (auto& addressToState : myAddressToState)
    {
        auto& connection = addressToState.second.connection;

        if (connection.HasFailed())
        {
            myAddressToState.erase(addressToState.first);

            Logging::Log(
                Logging::LogLevel::Notice,
                addressToState.first.address().to_string().c_str(),
                ": ",
                addressToState.first.port(),
                " failed due to: ",
                connection.FailReason().c_str());
        }

        if (connection.IsConnected())
        {
            auto delta = connection.GetDefragmentedPacket();

            if (delta.IsValid())
            {
                if (delta.GetSequence() > addressToState.second.lastAcked)
                {
                    auto client = connection.IdClient();

                    if (client)
                    {
                        // decrypt, decompress, parse.
                        std::vector<uint8_t> payload(GetPayloadBuffer(delta));

                        std::array<uint8_t, 4> code;
                        Push(begin(code), delta.GetSequence().Value());
                        Push(begin(code) + 2, delta.GetSequenceAck().Value());
                        XorCode(begin(code), end(code), connection.Key().data);
                        XorCode(begin(payload), end(payload), code);

                        auto decompressed = move(myCompressor.Decode(payload));

                        // Pass to gamestate (which will decompress the delta itself).
                        auto deltaData = Delta{
                                delta.GetSequenceBase(),
                                delta.GetSequence(),
                                move(decompressed)};

                        addressToState.second.lastAcked = myStateManager.DeltaParse(*client, deltaData);
                    }
                }
            }
        }
    }
}

void NetworkManagerServer::PrivateSendState()
{
    std::vector<NetworkPacket> responses{};

    for (auto& addressToState : myAddressToState)
    {
        auto& connection = addressToState.second.connection;

        if (connection.IsConnected())
        {
            auto client = connection.IdClient();

            if (client)
            {
                if (myStateManager.IsConnected(*client))
                {
                    // get the packet, and fragment it, then send it.
                    auto deltaData = myStateManager.DeltaCreate(*client, connection.LastSequenceAck());

                    auto distance = deltaData.to - deltaData.base;
                    if (distance <= PacketDelta::MaximumDeltaDistance())
                    {
                        // Compress, encrypt, send
                        auto compressed = move(myCompressor.Encode(deltaData.deltaPayload));

                        std::array<uint8_t, 4> code;
                        Push(begin(code), deltaData.to.Value());
                        Push(begin(code) + 2, addressToState.second.lastAcked.Value());
                        XorCode(begin(code), end(code), connection.Key().data);
                        XorCode(begin(compressed), end(compressed), code);

                        auto deltaPacket = PacketDelta{
                                deltaData.to,
                                addressToState.second.lastAcked,
                                static_cast<uint8_t>(distance),
                                move(compressed)};

                        if (deltaPacket.data.size() <= MaxPacketSizeInBytes)
                        {
                            auto fragments = PacketDeltaFragmentManager::FragmentPacket(deltaPacket);

                            for (auto& fragment: fragments)
                            {
                                if (!fragment.empty())
                                {
                                    if (myStateManager.CanPacketSend(*client, fragment.size()))
                                    {
                                        responses.emplace_back(move(fragment), addressToState.first);
                                    }
                                }
                            }
                        }
                        else
                        {
                            Logging::Log(Logging::LogLevel::Informational, "Packetsize is > MaxPacketSizeInBytes. Not sending.");
                        }
                    }
                    else
                    {
                        // Delta distance to too far. fail.
                        Logging::Log(Logging::LogLevel::Informational, "Delta distance > 255.");
                    }
                }
                else
                {
                    connection.Disconnect("IStateManager: Not Connected.");
                }
            }
            else
            {
                // WTF?
                // Ah well, clean up anyway.
                connection.Disconnect("Connection with no ClientId, wtf?");
                Logging::Log(Logging::LogLevel::Warning, "Connection with no ClientId, wtf?");
            }
        }
        else
        {
            auto response = connection.Process({});

            if (!response.empty())
            {
                if (myStateManager.CanPacketSend(connection.IdClient(), response.size()))
                {
                    responses.emplace_back(move(response), addressToState.first);
                }
            }
        }
    }

    if (!responses.empty())
    {
        myNetwork->Send(responses);
    }
}

void NetworkManagerServer::Disconnect()
{
    // Disconnect all clients, send their last packet,
    // flush the network buffers, disable them then quit.
    for (auto& connection : myAddressToState)
    {
        if (connection.second.connection.IsConnected())
        {
            connection.second.connection.Disconnect("Server shutdown.");
        }
    }

    PrivateSendState();
    myNetwork->Flush();
    myNetwork->Disable();
}
