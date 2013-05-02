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

#ifndef GAMEINABOX_H
#define GAMEINABOX_H

#include <cstdint>
#include "IStateManager.hpp"

namespace GameInABox { namespace Common {

class GameSimple : public IStateManager
{
public:
    GameSimple() {};
    virtual ~GameSimple() {};
    
private:
    IStateManager::ClientHandle* PrivateConnect(
            std::vector<uint8_t> connectData,
            bool& fail,
            std::string& failReason) override;

    void PrivateDisconnect(ClientHandle* playerToDisconnect) override;
        
    virtual void PrivateDeltaGet(
            const ClientHandle& client,
            Sequence& tickTo,
            Sequence& tickFrom,
            Sequence lastTickAcked,
            BitStream& result) const override;

    virtual void PrivateDeltaSet(
            const ClientHandle& client,
            Sequence tickTo,
            Sequence tickFrom,
            BitStreamReadOnly& source) override;
};

}} // namespace

#endif // GAMEINABOX_H