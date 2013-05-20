/*
    Game-in-a-box. Simple First Person Shooter Network Game.
    Copyright (C) 2012-2013 Richard Maxwell <jodi.the.tigger@gmail.com>
    
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
#include <chrono>
#include <tuple>
#include <sstream>
#endif

#include "Common/IStateManager.hpp"
#include "NetworkPacket.hpp"
#include "PacketTypes.hpp"
#include "PacketChallenge.hpp"
#include "PacketChallengeResponse.hpp"
#include "PacketDelta.hpp"


#include "NetworkStateServer.hpp"

using namespace std::chrono;
using Timepoint = std::chrono::steady_clock::time_point;

namespace GameInABox { namespace Common { namespace Network {

enum class State
{
    Idle,
    FailedConnection,
    Connected,
    Disconnecting,

    // Server States
    Listening,

    // Client States
    Challenging,
    Connecting,
};

NetworkStateServer::NetworkStateServer(
        IStateManager& stateManager,
        boost::asio::ip::udp::endpoint address)
    : myStateManager(stateManager)
    , myAddress(address)
    , myState(State::Idle)
    , myFailReason("")
    , myKey(GetNetworkKeyNil())
    , myPacketCount(0)
    , myLastTimestamp(Timepoint::min())
    , myStateHandle()
{
}

void NetworkStateServer::StartClient()
{
    myKey = GetNetworkKeyNil();
    myStateHandle.reset();
    Reset(State::Challenging);
}

void NetworkStateServer::StartServer()
{
    myKey = GetNetworkKeyRandom();
    myStateHandle.reset();
    Reset(State::Listening);
}

void NetworkStateServer::Disconnect()
{
    myStateManager.Disconnect(*myStateHandle);
    Reset(State::Disconnecting);
}

std::vector<NetworkPacket> NetworkStateServer::Process(NetworkPacket packet)
{
    std::vector<NetworkPacket> result;

    // NOTE:
    // pure things:
    // State, NetworkPacket, packetCount, timeLastPacket, timeCurrent, failReason
    // impure things:
    // IStateManager, Disconnecting, Fail()
    // tried to have this all in a pure function, but calling IStateManager connect
    // and disconnect was impure, wrong place to try.

    // Done in two parts, parse the packets then generate packets.
    // Parse first as that can change the state we're in.

    // ///////////////////
    // Parse
    // ///////////////////
    if (packet.address == myAddress)
    {
        switch (myState)
        {
            case State::Idle:
            case State::FailedConnection:
            case State::Disconnecting:
            {
                // Nothing, ignore everything
                break;
            }

            case State::Connected:
            {
                // check to see if we disconnect.
                Disconnected(packet);
                break;
            }

            case State::Challenging:
            {
                if (!Disconnected(packet))
                {
                    if (Packet::GetCommand(packet.data) == Command::ChallengeResponse)
                    {
                        auto response = PacketChallengeResponse{packet.data};

                        if (response.IsValid())
                        {
                            if (response.Version() == Version)
                            {
                                myKey = response.Key();
                                Reset(State::Connecting);
                            }
                            else
                            {
                                std::ostringstream theFail{};

                                theFail
                                    << "Network Protocol Mistmatch, expecting version: "
                                    << Version
                                    << " recieved: "
                                    << response.Version()
                                    << ".";

                                Fail(theFail.str());
                            }
                        }
                    }
                }

                break;
            }

            case State::Connecting:
            {
                if (!Disconnected(packet))
                {
                    if (Packet::GetCommand(packet.data) == Command::ConnectResponse)
                    {
                        auto connection = PacketConnectResponse{packet.data};

                        if (connection.IsValid())
                        {
                            std::string failReason{};
                            auto handle = myStateManager.Connect(connection.GetBuffer(), failReason);

                            if (!handle)
                            {
                                result.emplace_back(
                                    PacketDisconnect(myKey, failReason).TakeBuffer(),
                                    myAddress);

                                Fail(failReason);
                            }
                            else
                            {
                                myStateHandle = handle;
                                Reset(State::Connected);
                            }
                        }
                    }
                }

                break;
            }

            // ///////////////////
            // Server
            // ///////////////////
            case State::Listening:
            {
                auto command = Packet::GetCommand(packet.data);

                // Deal with floods but not sending the response.
                if (myPacketCount > FloodTrigger)
                {
                    switch (command)
                    {
                        case Command::Challenge:
                        {
                            auto challenge = PacketChallenge{packet.data};

                            if (challenge.IsValid())
                            {
                                result.emplace_back(
                                    PacketChallengeResponse(Version, myKey).TakeBuffer(),
                                    myAddress);

                                myLastTimestamp = GetTimeNow();
                                ++myPacketCount;
                            }

                            break;
                        }

                        case Command::Info:
                        {
                            auto info = PacketConnect{packet.data};

                            if (info.IsValid())
                            {
                                if (info.Key() == myKey)
                                {
                                    auto infoData = myStateManager.StateInfo({});
                                    PacketInfoResponse packet{};
                                    packet.Append(infoData);

                                    // NOTE: Packet will be UDP fragmented if too big.
                                    // But I'm not going to do anything about that.
                                    result.emplace_back(
                                        packet.TakeBuffer(),
                                        myAddress);

                                    myLastTimestamp = GetTimeNow();
                                    ++myPacketCount;
                                }
                            }

                            break;
                        }

                        case Command::Connect:
                        {
                            auto connect = PacketConnect{packet.data};

                            if (connect.IsValid())
                            {
                                if (connect.Key() == myKey)
                                {
                                    // register the client with the game state
                                    // and if successful send the packet.
                                    if (!myStateHandle)
                                    {
                                        std::string failMessage{};
                                        auto handle = myStateManager.Connect(connect.GetBuffer(), failMessage);

                                        if (handle)
                                        {
                                            myStateHandle = handle;
                                        }
                                        else
                                        {
                                            result.emplace_back(
                                                PacketDisconnect(myKey, failMessage).TakeBuffer(),
                                                myAddress);

                                            Fail(failMessage);
                                        }
                                    }

                                    if (myStateHandle)
                                    {
                                        auto infoData = myStateManager.StateInfo(myStateHandle);
                                        PacketConnectResponse packet{};
                                        packet.Append(infoData);

                                        result.emplace_back(
                                            packet.TakeBuffer(),
                                            myAddress);

                                        myLastTimestamp = GetTimeNow();
                                        ++myPacketCount;
                                    }
                                }
                            }
                            else
                            {
                                result.emplace_back(
                                    PacketDisconnect(myKey, "Invalid Key.").TakeBuffer(),
                                    myAddress);
                                ++myPacketCount;
                            }

                            break;
                        }

                        case Command::Unrecognised:
                        {
                            // If we get deltas, that means we're connected.
                            if (PacketDelta::IsPacketDelta(packet.data))
                            {
                                Reset(State::Connected);
                            }

                            break;
                        }

                        default:
                        {
                            // nothing.
                            break;
                        }
                    }
                }

                break;
            }
        }
    }


    // ///////////////////
    // Generate
    // ///////////////////


    switch (myState)
    {
        // ///////////////////
        // Common
        // ///////////////////
        case State::Idle:
        {
            // Nothing, ignore everything
            break;
        }

        case State::FailedConnection:
        {
            // Again, Nothing, ignore everything.
            break;
        }

        case State::Connected:
        {
            if (packet.address == myAddress)
            {
                // check to see if we disconnect.
                Disconnected(packet);
            }

            break;
        }

        case State::Disconnecting:
        {
            auto failReason = std::string{"Normal Disconnect."};

            result.emplace_back(
                PacketDisconnect(myKey, failReason).TakeBuffer(),
                myAddress);
            Fail(failReason);
            break;
        }

        // ///////////////////
        // Client
        // ///////////////////
        case State::Challenging:
        {
            if (myPacketCount > HandshakeRetries)
            {
                Fail("Timeout: Challenging.");
            }
            else
            {
                auto sinceLastPacket = GetTimeNow() - myLastTimestamp;

                if (duration_cast<milliseconds>(sinceLastPacket) > HandshakeRetryPeriod())
                {
                    result.emplace_back(
                        PacketChallenge().TakeBuffer(),
                        myAddress);

                    myLastTimestamp = GetTimeNow();
                    ++myPacketCount;
                }
            }

            break;
        }

        case State::Connecting:
        {
            // RAM: duplicate code! remove!

            if (myPacketCount > HandshakeRetries)
            {
                Fail("Timeout: Connecting.");
            }
            else
            {
                auto sinceLastPacket = GetTimeNow() - myLastTimestamp;

                if (duration_cast<milliseconds>(sinceLastPacket) > HandshakeRetryPeriod())
                {
                    auto info = myStateManager.StateInfo({});

                    // may be so big it UDP fragments, not my problem.
                    result.emplace_back(
                        PacketConnect(myKey, info).TakeBuffer(),
                        myAddress);

                    myLastTimestamp = GetTimeNow();
                    ++myPacketCount;
                }
            }

            break;
        }

        // ///////////////////
        // Server
        // ///////////////////
        case State::Listening:
        {
            auto sinceLastPacket = GetTimeNow() - myLastTimestamp;

            if (duration_cast<milliseconds>(sinceLastPacket) > (HandshakeRetries * HandshakeRetryPeriod()))
            {
                // Meh, timeout.
                Fail("Timeout: Connecting to server.");
            }

            break;
        }
    }

    return result;
}

bool NetworkStateServer::IsConnected() const
{
    return myState == State::Connected;
}

bool NetworkStateServer::HasFailed() const
{
    return myState == State::FailedConnection;
}

void NetworkStateServer::Reset(State resetState)
{
    myState         = resetState;
    myFailReason    = "";
    myPacketCount   = 0;
    myLastTimestamp = Timepoint::min();
}

void NetworkStateServer::Fail(std::string failReason)
{
    myState = State::FailedConnection;
    myFailReason = failReason;
}

bool NetworkStateServer::Disconnected(const NetworkPacket& packet)
{
    bool result{false};

    if (Packet::GetCommand(packet.data) == Command::Disconnect)
    {
        auto disconnect = PacketDisconnect{packet.data};

        if (disconnect.IsValid())
        {
            if (disconnect.Key() == myKey)
            {
                Fail(disconnect.Message());
                result = true;
            }
        }
    }

    return result;
}

std::chrono::steady_clock::time_point NetworkStateServer::GetTimeNow()
{
    return std::chrono::steady_clock::now();
}

}}} // namespace
