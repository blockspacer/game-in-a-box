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

#include "NetworkManagerClientNew.h"

#include <string>
#include <chrono>
#include <vector>
#include <memory>

#include "NetworkProvider.h"
#include "Common/IStateManager.h"
#include "NetworkPacket.h"
#include "PacketChallenge.h"
#include "PacketDelta.h"
#include "Common/BitStreamReadOnly.h"

using std::string;
using namespace std::chrono;

NetworkManagerClientNew::NetworkManagerClientNew(
        std::vector<std::unique_ptr<NetworkProvider>> networks,
        IStateManager& stateManager)
    : INetworkManager()
    , myNetworks(move(networks))
    , myStateManager(stateManager)
    , myConnectedNetwork(nullptr)
    , myState(State::Idle)
    , myServerKey(0)
    , myServerAddress()
    , myStateHandle(nullptr)
    , myFailReason()
    , myPacketHelper()
    , myCompressor(stateManager.GetHuffmanFrequencies())
    , myLastSequenceProcessed(0)
    , myLastSequenceAcked(0)
    , myPacketSentCount(0)
    , myLastPacketSent()
{

}

NetworkManagerClientNew::~NetworkManagerClientNew()
{

}


void NetworkManagerClientNew::Connect(boost::asio::ip::udp::endpoint serverAddress)
{
    // reset state please.
    for (auto& network : myNetworks)
    {
        network->Reset();
    }

    myConnectedNetwork = nullptr;

    myState = State::Challenging;
    myConnectedNetwork = nullptr;
    myServerKey = 0;
    myServerAddress = serverAddress;
    myStateHandle = nullptr;
    myFailReason = "";

    myLastSequenceProcessed = Sequence(0);
    myLastSequenceAcked = Sequence(0);

    myPacketSentCount = 0;
    myLastPacketSent = std::chrono::steady_clock::time_point::min();

    // Kick off an OOB send.
    PrivateSendState();
}

void NetworkManagerClientNew::Disconnect()
{
    Fail("Client disconnected.");
}

void NetworkManagerClientNew::PrivateProcessIncomming()
{
    switch (myState)
    {
        case State::Challenging:
        {
            uint32_t key(0);
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
                        if (NetworkPacketHelper::GetPacketType(packet) == PacketCommand::Command::Disconnect)
                        {
                            Fail(NetworkPacketHelper::GetPacketString(packet));
                            exit = true;
                            break;
                        }

                        key = NetworkPacketHelper::GetKeyFromPacket(packet);

                        if (key != 0)
                        {
                            serverAddress = packet.address;
                            break;
                        }
                    }
                }

                if (key != 0)
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

            for (auto& packet : packets)
            {
                bool exit(false);

                switch (NetworkPacketHelper::GetPacketType(packet))
                {
                    case PacketCommand::Command::ConnectResponse:
                    {
                        auto connection(NetworkPacketHelper::GetConnectResponsePacket(packet));

                        bool failed;
                        string failReason;
                        IStateManager::ClientHandle* handle;

                        handle = myStateManager.Connect(connection->GetBuffer(), failed, failReason);

                        if (failed)
                        {
                            // Respond with a failed message please.
                            // Only one will do, the server can timeout if it misses it.
                            myConnectedNetwork->Send({{
                                  myServerAddress,
                                  PacketDisconnect(failReason).myBuffer}});

                            // wrong game type I assume.
                            Fail(failReason);
                        }
                        else
                        {
                            myStateHandle = handle;
                            myState = State::Connected;
                        }

                        // Don't support connecting to multilpe servers at the same time.
                        exit = true;
                        break;
                    }

                    case PacketCommand::Command::Disconnect:
                    {
                        Fail(NetworkPacketHelper::GetPacketString(packet));
                        exit = true;
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
            auto packets = myConnectedNetwork->Receive();

            for (auto& packet : packets)
            {
                if (NetworkPacketHelper::GetPacketType(packet) == PacketCommand::Command::Disconnect)
                {
                    // RAM: TODO: Put key into disconnected packet to prevent
                    // disconnected attack by spoofing disconnect message from
                    // server.
                    // RAM: TODO: Make GetPacketString a GetDisconnectString to typesafe it.
                    Fail(NetworkPacketHelper::GetPacketString(packet));
                    break;
                }

                if (NetworkPacketHelper::IsDeltaPacket(packet))
                {
                    myPacketHelper.DefragmentPackets(packet);
                }
            }

            // Do the work :-)
            DeltaReceive();

            break;
        }

        default:
        {
            break;
        }
    }
}

void NetworkManagerClientNew::PrivateSendState()
{    
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

                if (duration_cast<milliseconds>(sinceLastPacket) > HandshakeRetryPeriod)
                {
                    // send challenge packet please.
                    if (myState == State::Challenging)
                    {
                        myConnectedNetwork->Send({{
                              myServerAddress,
                              PacketChallenge().myBuffer}});
                    }
                    else
                    {
                        myConnectedNetwork->Send({{
                              myServerAddress,
                              PacketConnect(myServerKey).myBuffer}});
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
}

void NetworkManagerClientNew::Fail(std::string failReason)
{
    for (auto& network : myNetworks)
    {
        network->Flush();
        network->Disable();
    }

    myFailReason = failReason;
    myState = State::FailedConnection;
}

void NetworkManagerClientNew::DeltaReceive()
{
    PacketDelta mostRecent;
    Sequence latestSequence(myLastSequenceProcessed);

    // first, get the most recent packet.
    for (auto packet : myPacketHelper.GetDefragmentedPackets())
    {
        PacketDelta delta(packet.data);

        if (delta.IsValid())
        {
            if (delta.GetSequence() > latestSequence)
            {
                mostRecent = delta;
                latestSequence = delta.GetSequence();
            }
        }
    }

    if (mostRecent.IsValid())
    {
        // First copy
        std::vector<uint8_t> payload(mostRecent.GetPayload());

        // Bah, I wrote Huffman and Bitstream before I knew about iterators
        // or streams. This results in lots of copies that arn't really needed.
        // Need to benchmark to see if the copies matter, and if so, rewrite
        // to use iterators or streams.

        // Decrypt (XOR based).
        NetworkPacketHelper::CodeBufferInPlace(
                    payload,
                    myServerKey,
                    // RAM: TODO: Mayve just pass in a uint16_t buffer instead?
                    mostRecent.GetSequence().Value(),
                    mostRecent.GetSequenceAck().Value());

        // Decompress (2nd Copy)
        // NOTE: nothing too complex for encryption, even more simple than q3.
        // As someone wanting to hack can. If we want security, use public key
        // private key to pass a super long seed to a pseudo random generator
        // that's used to decrypt. Otherwise it's just easily hackable.
        std::unique_ptr<std::vector<uint8_t>> decompressed;
        decompressed = move(myCompressor.Decode(payload));

        // Pass to gamestate (which will decompress the delta itself).
        BitStreamReadOnly payloadBitstream(payload);
        myStateManager.DeltaSet(
            *myStateHandle,
            mostRecent.GetSequence().Value(),
            mostRecent.GetSequenceBase().Value(),
            payloadBitstream);

        // Now see what the last packet the other end has got.
        myLastSequenceAcked = mostRecent.GetSequenceAck();
    }
}

void NetworkManagerClientNew::DeltaSend()
{
    //BitStream payloadBitstream;
    //uint16_t

    // bool DeltaGet(uint16_t tickFrom, uint16_t tickTo, BitStream& result) const;

    //bool isOk(myStateManager.DeltaGet(myLastSequenceAcked.Value(), latest, payloadBitstream));

    // who owns the largest sequence number? network or state? -> State.

    //if ()

    // RAM: TODO! TODOOOOOOOOOO!
}


