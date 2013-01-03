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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gmock/gmock.h"
#include "INetworkAddress.h"

class NetworkAddressMock : public INetworkAddress
{
private:
    MOCK_CONST_METHOD0(PrivatePort, uint16_t());
    MOCK_CONST_METHOD0(PrivateAddress, uint64_t());
    MOCK_CONST_METHOD0(PrivateAddressString, std::string());
};