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

#include "GameSimple.hpp"

namespace GameInABox { namespace State { namespace Implementation { namespace GameSimple {

// /////////////////
// Tests
// /////////////////

bool CanThink(const Entity& target)
{
    // Items that update their state via the constants.
    return
            (target.type == EntityType::Player) ||
            (target.type == EntityType::Missle);
}

bool CanCreate(const Entity& target)
{
    return (target.type == EntityType::Player);
}

bool CanCollide(const Entity& target)
{
    return
            (target.type == EntityType::Player) ||
            (target.type == EntityType::Missle) ||
            (target.type == EntityType::Map);
}

bool CanMove(const Entity& target)
{
    return
            (target.type == EntityType::Player) ||
            (target.type == EntityType::Missle);
}

bool Collides(const Entity& first, const Entity& second)
{
    if ((first.type == EntityType::Map) && (second.type == EntityType::Map))
    {
        // maps don't collide with maps.
        return false;
    }

    // RAM: TODO:
    return false;
}

// /////////////////
// Actions
// /////////////////

Entity Think(Entity target, const EntityConstants&)
{
    // RAM: TODO:
    return target;
}

std::pair<Entity, Entity> Create(Entity target)
{
    // RAM: TODO:
    return
    {
        {
            EntityType::None,
            EntityNone{}
        },
        target
    };
}

std::pair<Entity, Entity> Resolve(Entity first, Entity second)
{
    // RAM: TODO:
    return {first, second};
}

Entity Move(Entity target)
{
    // RAM: TODO:
    return target;
}

}}}} // namespace
