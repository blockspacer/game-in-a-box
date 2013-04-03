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

#ifndef PACKETDELTA_H
#define PACKETDELTA_H

#include "WrappingCounter.h"

#include <vector>

class PacketDelta
{
public:
    PacketDelta() : PacketDelta(std::vector<uint8_t>()) {}
    PacketDelta(std::vector<uint8_t> rawData);

    PacketDelta(
            WrappingCounter<uint16_t> sequence,
            WrappingCounter<uint16_t> sequenceAck,
            uint8_t sequenceAckDelta,
            uint16_t* clientId,
            std::vector<uint8_t> deltaPayload);

    WrappingCounter<uint16_t> GetSequence();
    WrappingCounter<uint16_t> GetSequenceAck();
    WrappingCounter<uint16_t> GetSequenceAckBase();

    bool IsValid() const;
    bool HasClientId() const;
    uint16_t ClientId() const;
    std::vector<uint8_t> TakeBuffer() { return move(myBuffer); }

    // TODO: once we have namespaces, take this out of the class.
    // Hmm, couldn't we just use streams for this somehow?
    static uint16_t GetUint16(const std::vector<uint8_t>& buffer, std::size_t offset);
    static void Push(std::vector<uint8_t>& buffer, uint16_t data);

private:
    static const std::size_t OffsetSequence = 0;
    static const std::size_t OffsetSequenceAck = 2;
    static const std::size_t OffsetIsServerFlags = 2;
    static const std::size_t OffsetDeltaBaseAndFlags = 4;
    static const std::size_t OffsetClientId = 5;
    static const std::size_t OffsetDataClient = 7;
    static const std::size_t OffsetDataServer = 5;
    static const std::size_t MinimumPacketSizeClient = OffsetDataClient;
    static const std::size_t MinimumPacketSizeServer = OffsetDataServer;
    static const std::size_t MinimumPacketSizeCommon = MinimumPacketSizeServer;
    static const uint8_t MaskDeltaBase = ((1 << 6) - 1);
    static const uint8_t MaskTopByteIsServerPacket = 0x80;
    static const uint16_t MaskIsServerPacket = MaskTopByteIsServerPacket << 8;
    static const uint16_t MaskSequenceAck = MaskIsServerPacket - 1;

    std::vector<uint8_t> myBuffer;
};

#endif // PACKETDELTA_H