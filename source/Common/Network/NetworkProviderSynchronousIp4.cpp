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

#include "Common/MakeUnique.hpp"
#include "NetworkProviderSynchronousIp4.hpp"
#include "NetworkPacket.hpp"

using boost::asio::ip::udp;
using namespace GameInABox::Common::Network;

NetworkProviderSynchronousIp4::NetworkProviderSynchronousIp4(boost::asio::ip::udp::endpoint bindAddress)
    : INetworkProvider()
    , myBindAddress(bindAddress)
    , myIoService()
    , mySocket(make_unique<boost::asio::ip::udp::socket>(myIoService, myBindAddress))
{
}

NetworkProviderSynchronousIp4::~NetworkProviderSynchronousIp4() noexcept(true)
{
}

std::vector<NetworkPacket> NetworkProviderSynchronousIp4::PrivateReceive()
{
    std::vector<NetworkPacket> result;

    if (mySocket->is_open() && mySocket->available() > 0)
    {
        // don't check for packet type, as assume you cannot get ip4 on an ip6 socket.
        // for now loop one packet at a time until empty or max number of packets
        // in one receive.
    }

    return result;
}

void NetworkProviderSynchronousIp4::PrivateSend(std::vector<NetworkPacket> packets)
{
    if (mySocket->is_open() && !packets.empty())
    {
        for (auto packet : packets)
        {
            // test to see if they are the same network type (ip4/ip6)
            // group them to destination address
            // send the groups.
            // check for errors, fail gracfully.
        }
    }
}

void NetworkProviderSynchronousIp4::PrivateReset()
{
    PrivateDisable();

    // TODO! Check for errors, fail gracfully on error.
    // RAM: how? mySocket = make_unique<boost::asio::ip::udp::socket>(myIoService, myBindAddress));
}

void NetworkProviderSynchronousIp4::PrivateFlush()
{
    // Sends and Receives are blocking, nothing to flush.
}

void NetworkProviderSynchronousIp4::PrivateDisable()
{
    // RAM: WTF? worked without smart pointer. mySocket->shutdown();
    mySocket->close();
}

bool NetworkProviderSynchronousIp4::PrivateIsDisabled() const
{
    return !(mySocket->is_open());
}
