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
#else
#include "Common/PrecompiledHeaders.hpp"
#endif

#include "IStateManager.hpp"
#include "BitStream.hpp"
#include "BitStreamReadOnly.hpp"

using namespace GameInABox::Common;

std::array<uint64_t, 256> IStateManager::GetHuffmanFrequencies() const
{
    return PrivateGetHuffmanFrequencies();
}

std::vector<uint8_t> IStateManager::StateInfo(const boost::optional<ClientHandle>& client) const
{
    return PrivateStateInfo(client);
}

boost::optional<ClientHandle> IStateManager::Connect(
        std::vector<uint8_t> connectData,
        std::string &failReason)
{
    return PrivateConnect(connectData, failReason);
}

void IStateManager::Disconnect(ClientHandle toDisconnect)
{
    PrivateDisconnect(toDisconnect);
}

bool IStateManager::IsConnected(ClientHandle client) const
{
    return PrivateIsConnected(client);
}

void IStateManager::DeltaGet(
        ClientHandle client,
        Sequence& tickTo,
        Sequence& tickFrom,
        Sequence lastTickAcked,
        BitStream& result) const
{
    PrivateDeltaGet(client, tickTo, tickFrom, lastTickAcked, result);
}

void IStateManager::DeltaSet(
        ClientHandle client,
        Sequence tickTo,
        Sequence tickFrom,
        BitStreamReadOnly& source)
{
    PrivateDeltaSet(client, tickTo, tickFrom, source);
}
