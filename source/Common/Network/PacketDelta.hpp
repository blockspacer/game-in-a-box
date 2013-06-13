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

#ifndef PACKETDELTA_HPP
#define PACKETDELTA_HPP

#include "Common/Sequence.hpp"
#include "Packet.hpp"

namespace GameInABox { namespace Common { namespace Network {

class PacketDelta : public Packet
{
public:
    // Delta distance is stored as a byte.
    static constexpr std::size_t MaximumDeltaDistance() { return std::numeric_limits<uint8_t>::max(); }
    static bool IsPacket(const std::vector<uint8_t>& buffer);

    PacketDelta() : PacketDelta(std::vector<uint8_t>()) {}
    explicit PacketDelta(std::vector<uint8_t> rawData);

    PacketDelta(
            Sequence sequence,
            Sequence sequenceAck,
            uint8_t sequenceAckDelta,
            std::vector<uint8_t> deltaPayload);

    // Rule of 5 (class contents are just one vector, so use defaults).
    PacketDelta(const PacketDelta&) = default;
    PacketDelta(PacketDelta&&) = default;
    PacketDelta& operator=(const PacketDelta&) = default;
    PacketDelta& operator=(PacketDelta&&) = default;
    virtual ~PacketDelta() = default;

    Sequence GetSequence() const;
    Sequence GetSequenceBase() const;
    Sequence GetSequenceAck() const;

    bool IsValid() const override { return IsPacket(data); }

    std::size_t OffsetPayload() const override { return OffsetData; }

protected:

    // No, I'm not going to use a struct to determine offsets.
    static const std::size_t OffsetSequence = 0;
    static const std::size_t OffsetIsFragmented = OffsetSequence;
    static const std::size_t OffsetSequenceAck = 2;
    static const std::size_t OffsetIsServerFlags = 2;
    static const std::size_t OffsetDeltaBase = 4;
    static const std::size_t OffsetData = 5;
    static const std::size_t MinimumPacketSize = OffsetData;

    static const uint8_t MaskTopByteIsServerPacket = 0x80;
    static const uint8_t MaskTopByteIsFragmented = 0x80;
    static const uint16_t MaskIsServerPacket = MaskTopByteIsServerPacket << 8;
    static const uint16_t MaskIsFragmented = MaskTopByteIsFragmented << 8;
    static const uint16_t MaskSequenceAck = MaskIsServerPacket - 1;
    static const uint16_t MaskSequence = MaskIsFragmented - 1;

};

}}} // namespace

#endif // PACKETDELTA_HPP
