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

#ifndef NETWORKMANAGERCLIENTNEW_H
#define NETWORKMANAGERCLIENTNEW_H

#include <vector>
#include <memory>
#include <chrono>
#include <utility>
#include <boost/asio/ip/udp.hpp>
#include "INetworkManager.h"
#include "Common/IStateManager.h"

// forward delcarations
class NetworkProvider;
class IStateManager;

// RAM: LOL, std forgot about this one
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// RAM: TODO! Move this to it's own class
class PacketDelta
{
    uint16_t Sequence() { return 0; }

};

// RAM: TODO! Move this to it's own class
# include "NetworkPacket.h"
#include "PacketSimple.h"
class NetworkPacketHelper
{
public:
    static uint32_t GetKeyFromPacket(NetworkPacket) { return 0; }
    static std::unique_ptr<PacketConnectResponse> GetConnectResponsePacket(NetworkPacket) { return std::unique_ptr<PacketConnectResponse>(nullptr); }
    static PacketCommand::Command GetPacketType(const NetworkPacket&) { return PacketCommand::Command::Invalid; }
    static std::string GetPacketString(NetworkPacket) { return "TODO: Fix stub function"; }
    static bool IsDeltaPacket(NetworkPacket&) { return false; }

    void DefragmentPackets(NetworkPacket&) {};
    std::vector<NetworkPacket> GetDefragmentedPackets() { return {}; }
};

class NetworkManagerClientNew : public INetworkManager
{
    CLASS_NOCOPY_ASSIGN_MOVE(NetworkManagerClientNew)

public:
    NetworkManagerClientNew(
            std::vector<std::unique_ptr<NetworkProvider>> networks,
            IStateManager& stateManager);

    void Connect(boost::asio::ip::udp::endpoint serverAddress);
    void Disconnect();

    virtual ~NetworkManagerClientNew();

private:
    static constexpr uint8_t HandshakeRetries{5};
    static constexpr std::chrono::milliseconds HandshakeRetryPeriod{1000};

    enum class State
    {
        Idle,
        Challenging,
        Connecting,
        Connected,
        FailedConnection,
    };

    void PrivateProcessIncomming() override;
    void PrivateSendState() override;

    void Fail(std::string failReason);
    void DeltaReceive();
    void DeltaSend();

    std::vector<std::unique_ptr<NetworkProvider>> myNetworks;
    IStateManager& myStateManager;
    NetworkProvider* myConnectedNetwork;

    State myState;
    uint32_t myServerKey;
    boost::asio::ip::udp::endpoint myServerAddress;
    IStateManager::ClientHandle* myStateHandle;
    std::string myFailReason;

    NetworkPacketHelper myPacketHelper;

    uint8_t myPacketSentCount;
    std::chrono::steady_clock::time_point myLastPacketSent;
};

#endif // NETWORKMANAGERCLIENTNEW_H