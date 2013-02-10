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

#ifndef NETWORKMANAGERBASE_H
#define NETWORKMANAGERBASE_H

#include <cstdint>
#include <vector>

#include "Sequence.h"

// forward delcarations
class NetworkPacket;

class NetworkManagerBase
{    
    void ParsePacket(NetworkPacket& packetData);

protected:
    static const std::size_t MinimumPacketSize = 3;
    static const std::size_t MinimumPacketSizeFromClient = MinimumPacketSize + 2;
    static const std::size_t OffsetSequence = 0;
    static const std::size_t OffsetLinkId = 2;
    static const std::size_t OffsetDataFromServer = 2;
    static const std::size_t OffsetDataFromClient = 4;
    static const std::size_t OffsetFragmentId = 2;
    static const std::size_t OffsetFragmentData = 3;
    static const std::size_t OffsetCommandKey = 2;
    static const std::size_t OffsetCommand = 6;
    static const std::size_t OffsetCommandData = 7;

    static const std::size_t MtuIp4 = 576;
    static const std::size_t MtuIp6 = 1280;
    static const std::size_t MtuEthernetV2 = 1500;
    static const std::size_t MtuEthernetLlcSnapPppoe = 1492;

    static const std::size_t SizeMaxMtu = MtuEthernetLlcSnapPppoe;
    static const std::size_t SizeIpHeaderMinimum = 20;
    static const std::size_t SizeUdpHeader = 8;

    static const std::size_t SizeMaxPacketSize = SizeMaxMtu - (SizeIpHeaderMinimum + SizeUdpHeader);

    enum class Command : uint8_t
    {
        Invalid = 0,
        Challenge,
        ChallengeResponse,
        Info,
        InfoResponse,
        Connect,
        // ConnectResponse doesn't exist, I just start sending deltas.

        // Argh, no easy way to tell if a uint8_t is a valid enum
        // Have to make sure they are all sequential from 0, and test
        // against this first. That or a huge convert case statement. Argh!
        // RAM: using a switch statement white list, so I don't need this.
        //CommandCount
    };

    enum class PacketEncoding
    {
        FromClient = 0,
        FromServer = 1
    };

    NetworkManagerBase(PacketEncoding details);

    // pure functional is good.
    static NetworkPacket PacketDefragment(const std::vector<NetworkPacket>& fragments);
    static std::vector<NetworkPacket>PacketFragment(NetworkPacket &whole);

private:
    PacketEncoding myEncodingDetails;

    std::vector<NetworkPacket> myFragments;
    uint8_t myFragmentCount;
    uint8_t myFragmentTotal;
    Sequence myFragmentSequence;

    virtual void ParseCommand(NetworkPacket& packetData) = 0;
    virtual void ParseDelta(NetworkPacket& packetData) = 0;

    Sequence SequenceFromPacket(const NetworkPacket& packetData);
};

#endif // NETWORKMANAGERBASE_H
