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

#ifndef PACKETCHALLENGERESPONSE_H
#define PACKETCHALLENGERESPONSE_H

#ifndef USING_PRECOMPILED_HEADERS
#include <cstdint>
#endif

#include "PacketCommandWithKey.hpp"

namespace GameInABox { namespace Network { namespace Implementation {

class PacketChallengeResponse : public PacketCommandWithKey<Command::ChallengeResponse>
{
public:
    PacketChallengeResponse(uint8_t version, NetworkKey key);
    PacketChallengeResponse(uint8_t version, NetworkKey key, std::vector<uint8_t> payload);
    explicit PacketChallengeResponse(std::vector<uint8_t> buffer) : PacketCommandWithKey(buffer) {}
    virtual ~PacketChallengeResponse();

    virtual bool IsValid() const override;    
    virtual std::size_t OffsetPayload() const override { return PacketSize; }

    uint8_t Version() const;

private:
    static const std::size_t OffsetVersion = PacketCommandWithKey::OffsetKey + PacketCommandWithKey::PayloadSize;
    static const std::size_t PacketSize = OffsetVersion + sizeof(uint8_t);
};

}}} // namespace

#endif // PACKETCHALLENGERESPONSE_H
