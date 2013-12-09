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

#ifndef UNITSSI_HPP
#define UNITSSI_HPP

namespace GameInABox { namespace State { namespace Implementation {

// /////////////////////
// Unit Structs
// /////////////////////
struct Radians
{
    float value;
};

// /////////////////////
// User defined literals for units.
// /////////////////////
Radians constexpr operator"" _radians(long double value)
{
    return {static_cast<float>(value)};
}

}}} // namespace

namespace GameInABox { namespace State { namespace Implementation { namespace Units {

template<int M, int K, int S>
struct Unit
{
    // MKS = Meters, Kilograms, Seconds.
    enum { m=M, kg=K, s=S };
};

template<typename UNIT, typename T=float>
struct Quantity
{
    T value;
};

// ///////////////////
// Basic SI Units
// ///////////////////

// Have a read of this if you want to make this more complicated.
// http://www.csee.umbc.edu/portal/help/theory/units.shtml

//Speed         = Length / Time
//Acceleration	= Speed	 / Time
//Force         = Mass	 * Acceleration
//AngularSpeed	= Angle	 / Time
//Length        = Angle	 * Length
//Area          = Length * Length
//Volume        = Area	 * Length
//Density       = Mass	 / Volume
//MassFlowRate	= Mass	 / Time
//Frequency     = float	 / Time
//Pressure      = Force	 / Area

using Unitless = Unit<0,0,0>;

using Metre = Unit<1,0,0>;
using Metres = Metre;
// Grrr, Americans.
using Meter = Metre;
using Meters = Metre;

using SquareMetre = Unit<2,0,0>;
using SquareMetres = SquareMetre;
using SquareMeter = SquareMetre;
using SquareMeters = SquareMetre;

using CubicMetre = Unit<3,0,0>;
using CubicMetres = CubicMetre;
using CubicMeter = CubicMetre;
using CubicMeters = CubicMetre;

using Kilogram = Unit<0,1,0>;
using Kilograms = Kilogram;

using Second = Unit<0,0,1>;
using Seconds = Second;

using Newton = Unit<1,1,-2>;
using Newtons = Newton;

using Joule = Unit<2,1,-2>;
using Joules = Joule;

using MetersPerSecond = Unit<1,0,-1>;
using MetersPerSecondSquared = Unit<1,0,-2>;

using Radian = Unitless;
using Radians = Radian;

// ///////////////////
// Comparison
// ///////////////////
template<typename UNIT, typename T1, typename T2>
inline constexpr bool operator==(const Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs){return lhs.value==rhs.value;}
template<typename UNIT, typename T1, typename T2>
inline constexpr bool operator!=(const Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs){return !operator==(lhs,rhs);}
template<typename UNIT, typename T1, typename T2>
inline constexpr bool operator< (const Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs){return lhs.value< rhs.value;}
template<typename UNIT, typename T1, typename T2>
inline constexpr bool operator> (const Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs){return  operator< (rhs,lhs);}
template<typename UNIT, typename T1, typename T2>
inline constexpr bool operator<=(const Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs){return !operator> (lhs,rhs);}
template<typename UNIT, typename T1, typename T2>
inline constexpr bool operator>=(const Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs){return !operator< (lhs,rhs);}

// ///////////////////
// Simple Maths
// ///////////////////
template<typename UNIT, typename T1, typename T2>
inline Quantity<UNIT, T1>& operator+=(Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs)
{
    lhs.value+=rhs.value;
    return lhs;
}

template<typename UNIT, typename T1, typename T2>
inline Quantity<UNIT, T1>& operator-=(Quantity<UNIT, T1>& lhs, const Quantity<UNIT, T2>& rhs)
{
    lhs.value-=rhs.value;
    return lhs;
}

template<typename UNIT, typename T1, typename T2>
inline Quantity<UNIT, T1> operator*=(Quantity<UNIT, T1>& lhs, const Quantity<Unitless, T2>& rhs)
{
    lhs.value*=rhs.value;
    return lhs;
}

template<typename UNIT, typename T1, typename T2>
inline Quantity<UNIT, T1> operator/=(Quantity<UNIT, T1>& lhs, const Quantity<Unitless, T2>& rhs)
{
    lhs.value/=rhs.value;
    return lhs;
}

template<typename UNIT, typename T>
inline constexpr Quantity<UNIT, T> operator-(const Quantity<UNIT, T>& lhs)
{
    return Quantity<UNIT, T>{-lhs.value};
}

template<typename UNIT, typename T1, typename T2>
inline Quantity<UNIT, T1> operator+(Quantity<UNIT, T1> lhs, const Quantity<UNIT, T2>& rhs){ lhs += rhs;  return lhs; }
template<typename UNIT, typename T1, typename T2>
inline Quantity<UNIT, T1> operator-(Quantity<UNIT, T1> lhs, const Quantity<UNIT, T2>& rhs){ lhs -= rhs;  return lhs; }

template<int M1, int M2, int K1, int K2, int S1, int S2, typename T1, typename T2>
inline Quantity<Unit<M1+M2, K1+K2, S1+S2>, T1> operator*(const Quantity<Unit<M1, K1, S1>, T1>& lhs, const Quantity<Unit<M2, K2, S2>, T2>& rhs)
{
    return {lhs.value * rhs.value};
}
template<int M1, int M2, int K1, int K2, int S1, int S2, typename T1, typename T2>
inline Quantity<Unit<M1-M2, K1-K2, S1-S2>, T1> operator/(const Quantity<Unit<M1, K1, S1>, T1>& lhs, const Quantity<Unit<M2, K2, S2>, T2>& rhs)
{
    return {lhs.value / rhs.value};
}

// ///////////////////
// Vector Maths
// ///////////////////
template<typename UNIT, typename T>
inline constexpr Quantity<UNIT, T> Magnitude(const Quantity<UNIT, T>& lhs)
{
    return {Magnitude(lhs.value)};
}

template<typename UNIT, typename T>
inline constexpr Quantity<UNIT, float> MagnitudeF(const Quantity<UNIT, T>& lhs)
{
    return {MagnitudeF(lhs.value)};
}

template<int M, int K, int S, typename T>
inline constexpr Quantity<Unit<M*2, K*2, S*2>, T> PlaneArea(const Quantity<Unit<M, K, S>, T>& lhs)
{
    return {PlaneArea(lhs.value)};
}

template<int M, int K, int S, typename T>
inline constexpr Quantity<Unit<M*2, K*2, S*2>, float> PlaneAreaF(const Quantity<Unit<M, K, S>, T>& lhs)
{
    return {PlaneAreaF(lhs.value)};
}

template<typename UNIT, typename T>
inline constexpr Quantity<Unitless, T> Normalise(const Quantity<UNIT, T>& lhs)
{
    return {Normalise(lhs.value)};
}

template<int M1, int M2, int K1, int K2, int S1, int S2, typename T1, typename T2>
inline Quantity<Unit<M1+M2, K1+K2, S1+S2>, T1> Dot(const Quantity<Unit<M1, K1, S1>, T1>& lhs, const Quantity<Unit<M2, K2, S2>, T2>& rhs)
{
    return {Dot(lhs.value, rhs.value)};
}

template<int M1, int M2, int K1, int K2, int S1, int S2, typename T1, typename T2>
inline Quantity<Unit<M1+M2, K1+K2, S1+S2>, float> DotF(const Quantity<Unit<M1, K1, S1>, T1>& lhs, const Quantity<Unit<M2, K2, S2>, T2>& rhs)
{
    return {DotF(lhs.value, rhs.value)};
}


}}}} // namespace


#endif // UNITSSI_HPP
