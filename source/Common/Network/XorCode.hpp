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

#ifndef XORCODE_H
#define XORCODE_H

#include <cstdint>
#include <vector>
#include <type_traits>

namespace GameInABox { namespace Common { namespace Network {

// TODO! Support std::array as the iterators!

// Only allow uint8_t iterators by checking that the iterator
// type (T::value_type) is a uint8_t.
template<class Uint8tIterator,
         class CodeArrayType,
         class = typename std::enable_if<
             std::is_same<typename Uint8tIterator::value_type, std::uint8_t>::value>
         ::type,
        class = typename std::enable_if<
            std::is_same<typename CodeArrayType::value_type, std::uint8_t>::value>
        ::type>
void XorCode(
        Uint8tIterator begin,
        Uint8tIterator end,
        const CodeArrayType& toCodeAgainst)
{
    if (!toCodeAgainst.empty())
    {
        std::size_t codeSize(toCodeAgainst.size());
        std::size_t index(0);

        for (; begin != end; begin++)
        {
            *begin = *begin ^ toCodeAgainst[index];
            index = (index + 1) % codeSize;
        }
    }
}

}}} // namespace

#endif // XORCODE_H