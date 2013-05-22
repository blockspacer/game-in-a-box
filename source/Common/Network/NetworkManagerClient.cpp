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
#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <array>
#endif

#include "Common/Logging/Logging.hpp"
#include "Common/IStateManager.hpp"
#include "Common/BitStream.hpp"
#include "Common/BitStreamReadOnly.hpp"

#include "INetworkProvider.hpp"
#include "NetworkPacket.hpp"
#include "PacketChallenge.hpp"
#include "PacketChallengeResponse.hpp"
#include "PacketDelta.hpp"
#include "PacketTypes.hpp"
#include "XorCode.hpp"
#include "BufferSerialisation.hpp"
#include "Handshake.hpp"

#include "NetworkManagerClient.hpp"

using std::string;
using namespace std::chrono;
using namespace GameInABox::Common::Network;
using namespace GameInABox::Common::Logging;

// RAM: TODO! Make sure you check recieved packets are from the server address.
NetworkManagerClient::NetworkManagerClient(
        std::vector<MotleyUniquePointer<INetworkProvider>> networks,
        IStateManager& stateManager)
    : INetworkManager()
    , myNetworks()
    , myConnectedNetwork(nullptr)
    , myStateManager(stateManager)
    , myState(State::Idle)
    , myServerKey(GetNetworkKeyNil())
    , myServerAddress()
    , myStateHandle()
    , myFailReason()
    , myClientId(0)
    , myDeltaHelper()
    , myCompressor(stateManager.GetHuffmanFrequencies())
    , myLastSequenceProcessed(0)
    , myLastSequenceAcked(0)
    , myPacketSentCount(0)
    , myLastPacketSent()
{
    for (auto& network : networks)
    {
        myNetworks.push_back({move(network), {stateManager}});
        // RAM: TODO: Why doesn't emplace_back compile?
        //myNetworks.emplace_back(move(network), {stateManager});
        //myNetworks.emplace_back({move(network), {stateManager}});
    }
}

NetworkManagerClient::~NetworkManagerClient()
{

}


void NetworkManagerClient::Connect(boost::asio::ip::udp::endpoint serverAddress)
{
    // reset state please.
    for (auto& network : myNetworks)
    {
        network.first->Reset();
        network.second.StartClient();
    }

    myState = State::Challenging;
    myConnectedNetwork = nullptr;
    myServerKey = GetNetworkKeyNil();
    myServerAddress = serverAddress;
    myStateHandle = {};
    myFailReason = "";

    myLastSequenceProcessed = Sequence(0);
    myLastSequenceAcked = Sequence(0);

    myPacketSentCount = 0;
    myLastPacketSent = std::chrono::steady_clock::time_point::min();

    // Kick off an OOB send.
    PrivateSendState();
}

void NetworkManagerClient::Disconnect()
{
    Fail("Client disconnected.");
}

// RAM: TODO! Use Handshake!
void NetworkManagerClient::PrivateProcessIncomming()
{
    if (myConnectedNetwork != nullptr)
    {
        // already connected, just deal with that.

        if (myStateManager.IsConnected(*myStateHandle))
        {
            auto packets = myConnectedNetwork->first->Receive();

            for (auto& packet : packets)
            {
                if (PacketDelta::IsPacketDelta(packet.data))
                {
                    myDeltaHelper.AddPacket(PacketDelta(packet.data));
                }
                else
                {
                    // Pass though state incase we get disconnect packets.
                    auto response = myConnectedNetwork->second.Process(packet.data);
                    myConnectedNetwork->first->Send({{response, myServerAddress}});

                    if (myConnectedNetwork->second.HasFailed())
                    {
                        Fail(myConnectedNetwork->second.FailReason());
                        break;
                    }
                }
            }

            // Do some work :-)
            if (myConnectedNetwork->second.IsConnected())
            {
                DeltaReceive();
            }
        }
        else
        {
            // No longer connected, quit out.
            // RAM: TODO! Need way of telling the server this too!
            Fail("State is no longer connected.");
        }
    }
    else
    {
        // talking to all interfaces for now.
        for (auto& network : myNetworks)
        {
            if (!network.first->IsDisabled())
            {
                auto packets = network.first->Receive();
                std::vector<NetworkPacket> toSend{};

                for (auto& packet : packets)
                {
                    auto response = network.second.Process(packet.data);
                    toSend.emplace_back(response, myServerAddress);

                    // Win, lose or nothing?
                    if (network.second.IsConnected())
                    {
                        // Win
                        myConnectedNetwork = &network;

                        // Don't accidently drop a delta packet.
                        if (PacketDelta::IsPacketDelta(packet.data))
                        {
                            myDeltaHelper.AddPacket(PacketDelta(packet.data));
                        }

                        break;
                    }
                    else
                    {
                        if (network.second.HasFailed())
                        {
                            // Lose.
                            network.first->Send(toSend);
                            network.first->Flush();
                            network.first->Disable();
                            break;
                        }
                    }
                }

                if (!network.first->IsDisabled())
                {
                    network.first->Send(toSend);
                }
            }
        }
    }

    /*
    switch (myState)
    {

        case State::Challenging:
        {
            NetworkKey key(GetNetworkKeyNil());
            boost::asio::ip::udp::endpoint serverAddress;

            // talking to all interfaces for now.
            for (auto& network : myNetworks)
            {
                bool exit(false);

                if (!network->IsDisabled())
                {
                    auto packets = network->Receive();

                    for (auto& packet : packets)
                    {                        
                        Command commandType(Packet::GetCommand(packet.data));

                        if (commandType == Command::Disconnect)
                        {
                            PacketDisconnect disconnect(packet.data);

                            if (disconnect.IsValid())
                            {
                                Fail(disconnect.Message());
                                exit = true;
                                break;
                            }
                        }
                        else
                        {
                            if (commandType == Command::ChallengeResponse)
                            {
                                PacketChallengeResponse response(packet.data);

                                if (response.IsValid())
                                {
                                    key = response.Key();
                                }
                            }
                        }

                        if (!(key.is_nil()))
                        {
                            serverAddress = packet.address;
                            break;
                        }
                    }
                }

                if (!(key.is_nil()))
                {
                    myConnectedNetwork = network.get();
                    myServerAddress = serverAddress;
                    exit = true;
                }

                if (exit)
                {
                    break;
                }
            }

            // Level up?
            if (myConnectedNetwork != nullptr)
            {
                // disable all other networks
                for (auto& network : myNetworks)
                {
                    if (network.get() != myConnectedNetwork)
                    {
                        network->Disable();
                    }
                }

                // set key and change state
                myServerKey = key;
                myState = State::Connecting;
            }

            break;
        }

        case State::Connecting:
        {
            auto packets = myConnectedNetwork->Receive();

            // Fail?
            if (myConnectedNetwork->IsDisabled())
            {
                Fail("Network Failed Unexpectedly.");
                break;
            }

            for (auto& packet : packets)
            {
                bool exit(false);

                switch (Packet::GetCommand(packet.data))
                {
                    case Command::ConnectResponse:
                    {
                        PacketConnectResponse connection(packet.data);

                        if (connection.IsValid())
                        {
                            string failReason;

                            auto handle = myStateManager.Connect(connection.GetBuffer(), failReason);

                            if (!handle)
                            {
                                // Respond with a failed message please.
                                // Only one will do, the server can timeout if it misses it.
                                myConnectedNetwork->Send({{
                                      PacketDisconnect(myServerKey, failReason).TakeBuffer(),
                                      myServerAddress}});

                                Fail(failReason);
                            }
                            else
                            {
                                myStateHandle = handle;
                                myState = State::Connected;
                            }

                            // Don't support connecting to multiple servers at the same time.
                            exit = true;
                        }

                        break;
                    }

                    case Command::Disconnect:
                    {
                        PacketDisconnect disconnect(packet.data);

                        if (disconnect.IsValid())
                        {
                            Fail(disconnect.Message());
                            exit = true;
                        }

                        break;
                    }

                    default:
                    {
                        // ignore
                        break;
                    }
                }

                if (exit)
                {
                    break;
                }
            }

            break;
        }

        case State::Connected:
        {
            // Cannot get disconnected unless the gamestate tells us to.
            // That is, ignore disconnect state packets.
            if (myStateManager.IsConnected(*myStateHandle))
            {
                auto packets = myConnectedNetwork->Receive();

                for (auto& packet : packets)
                {
                    if (PacketDelta::IsPacketDelta(packet.data))
                    {
                        myDeltaHelper.AddPacket(PacketDelta(packet.data));
                    }
                }

                // Do the work :-)
                DeltaReceive();
            }
            else
            {
                // No longer connected, quit out.
                // RAM: TODO! Need way of telling the server this too!
                Fail("State is no longer connected.");
            }

            break;
        }

        default:
        {
            break;
        }
    }
    */
}

// RAM: TODO! Use Handshake!
void NetworkManagerClient::PrivateSendState()
{        
    if (myConnectedNetwork != nullptr)
    {
        // Don't bother checkng state, as when connected it can't do
        // anything without receiving a packet first.
        //auto response = myConnectedNetwork->second.Process(packet.data);
        //myConnectedNetwork->first->Send({{response, myServerAddress}});

        //if (myConnectedNetwork->second.HasFailed())
        //{
        //    Fail(myConnectedNetwork->second.FailReason());
        //    return;
        //}
        DeltaSend();
    }
    else
    {
        // talking to all interfaces for now.
        for (auto& network : myNetworks)
        {
            if (!network.first->IsDisabled())
            {
                auto response = network.second.Process({});
                network.first->Send({{response, myServerAddress}});

                // Lose?
                if (network.second.HasFailed())
                {
                    // Lose.
                    network.first->Flush();
                    network.first->Disable();
                    break;
                }
            }
        }
    }
    /*
    // Deal with timeout and resend logic here during
    // Connection handshaking. Otherwise get the lastest
    // state from the client and send a delta packet.
    switch (myState)
    {
        case State::Challenging:
        case State::Connecting:
        {
            if (myPacketSentCount > HandshakeRetries)
            {
                std::string failString("Timeout: ");

                if (myState == State::Challenging)
                {
                    failString += "Challenge.";
                }
                else
                {
                    failString += "Connecting.";
                }

                Fail(failString);
            }
            else
            {
                auto sinceLastPacket = steady_clock::now() - myLastPacketSent;

                if (duration_cast<milliseconds>(sinceLastPacket) > HandshakeRetryPeriod())
                {
                    // send challenge packet please.
                    if (myState == State::Challenging)
                    {
                        myConnectedNetwork->Send({{
                              PacketChallenge().TakeBuffer(),
                              myServerAddress}});
                    }
                    else
                    {
                        myConnectedNetwork->Send({{
                              PacketConnect(myServerKey).TakeBuffer(),
                              myServerAddress}});
                    }
                }
            }

            break;
        }

        case State::Connected:
        {
            DeltaSend();
            break;
        }

        default:
        {
            break;
        }
    }
    */
}

void NetworkManagerClient::Fail(std::string failReason)
{
    for (auto& network : myNetworks)
    {
        if (!network.first->IsDisabled())
        {
            network.second.Disconnect();
            auto lastPacket = network.second.Process({});

            network.first->Send({{lastPacket, myServerAddress}});
            network.first->Flush();
            network.first->Disable();
        }
    }

    myFailReason = failReason;
    myState = State::FailedConnection;

    Logging::Log(Logging::LogLevel::Notice, failReason.c_str());
}

void NetworkManagerClient::DeltaReceive()
{
    PacketDelta delta(myDeltaHelper.GetDefragmentedPacket());
    Sequence latestSequence(myLastSequenceProcessed);

    if (delta.IsValid())
    {
        if (delta.GetSequence() > latestSequence)
        {
            latestSequence = delta.GetSequence();

            // First copy
            std::vector<uint8_t> payload(delta.GetPayload());

            // Decrypt (XOR based).
            // NOTE: nothing too complex for encryption, even more simple than q3.
            // As someone wanting to hack can. If we want security, use public key
            // private key to pass a super long seed to a pseudo random generator
            // that's used to decrypt. Otherwise it's just easily hackable.
            // Reason for excryption in the fist place is to prevent easy man-in-the-middle
            // attacks to control someone else's connection.
            std::vector<uint8_t> code(4);
            Push(begin(code), delta.GetSequence().Value());
            Push(begin(code) + 2, delta.GetSequenceAck().Value());
            XorCode(begin(code), end(code), myServerKey.data);
            XorCode(begin(payload), end(payload), code);

            // Bah, I wrote Huffman and Bitstream before I knew about iterators
            // or streams. This results in lots of copies that arn't really needed.
            // Need to benchmark to see if the copies matter, and if so, rewrite
            // to use iterators or streams.

            // Decompress (2nd Copy)
            auto decompressed = move(myCompressor.Decode(payload));

            // Pass to gamestate (which will decompress the delta itself).
            BitStreamReadOnly payloadBitstream(*decompressed);
            myStateManager.DeltaSet(
                *myStateHandle,
                delta.GetSequence().Value(),
                delta.GetSequenceBase().Value(),
                payloadBitstream);

            // Now see what the last packet the other end has got.
            myLastSequenceAcked = delta.GetSequenceAck();
        }
    }
}

void NetworkManagerClient::DeltaSend()
{
    BitStream payloadBitstream(MaxPacketSizeInBytes);
    Sequence from;
    Sequence to;

    myStateManager.DeltaGet(
                *myStateHandle,
                to,
                from,
                myLastSequenceAcked,
                payloadBitstream);

    if (payloadBitstream.SizeInBytes() <= MaxPacketSizeInBytes)
    {
        // ignore if the delta distance is greater than 255, as we
        // store the distance as a byte.
        // RAM: TODO: Make this part of the type, so I don't use magic numbers.
        uint16_t distance(to - from);
        if (distance < 256)
        {
            PacketDelta delta(
                    from,
                    myLastSequenceProcessed,
                    uint8_t(distance),
                    &myClientId,
                    *(payloadBitstream.TakeBuffer()));

            // Fragment (if needed)
            std::vector<NetworkPacket> packets;
            auto fragments(PacketDeltaFragmentManager::FragmentPacket(delta));
            for (auto& fragment : fragments)
            {
                packets.emplace_back(fragment.TakeBuffer(), myServerAddress);
            }

            // send
            myConnectedNetwork->first->Send(packets);
        }
        else
        {
            Logging::Log(Logging::LogLevel::Informational, "Delta distance > 255.");
        }
    }
    else
    {
        // RAM: TODO: Gamestate should realise that packets are too big and make them smaller instead.
        Logging::Log(Logging::LogLevel::Informational, "Packetsize is > MaxPacketSizeInBytes. Not sending.");
    }
}


