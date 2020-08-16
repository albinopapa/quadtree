#pragma once

#include "vector_traits.h"

template<typename T>
class Vec2
{
public:
	constexpr Vec2() = default;
	constexpr Vec2( T x_in, T y_in )
		:
		x( x_in ),
		y( y_in )
	{
	}

	constexpr Vec2& operator+=( const Vec2& rhs )
	{
		return *this = *this + rhs;
	}
	constexpr Vec2& operator*=( T rhs )
	{
		return *this = *this * rhs;
	}
	constexpr Vec2& operator-=( const Vec2& rhs )
	{
		return *this = *this - rhs;
	}
	constexpr Vec2& operator/=( T rhs )
	{
		return *this = *this / rhs;
	}

	constexpr Vec2 operator+( const Vec2& rhs ) const
	{
		return Vec2( x + rhs.x, y + rhs.y );
	}
	constexpr Vec2 operator*( T rhs ) const
	{
		return Vec2( x * rhs, y * rhs );
	}
	constexpr Vec2 operator-()const noexcept {
		return { -x, -y };
	}
	constexpr Vec2 operator-( const Vec2& rhs ) const
	{
		return Vec2( x - rhs.x, y - rhs.y );
	}
	constexpr Vec2 operator/( T rhs ) const
	{
		return Vec2( x / rhs, y / rhs );
	}
public:
	T x;
	T y;
};

using Vec2f = Vec2<float>;
using Point = Vec2<int>;

struct ChiliVecAccess{
	using vector_type = Vec2f;
	using scalar_type = float;

	static constexpr auto construct( scalar_type x_, scalar_type y_ )noexcept {
		return vector_type{ x_, y_ };
	}
	static constexpr auto x( vector_type const& vec )noexcept {
		return vec.x;
	}
	static constexpr auto y( vector_type const& vec )noexcept {
		return vec.y;
	}

	static constexpr void x( vector_type& vec, float value )noexcept {
		vec.x = value;
	}
	static constexpr void y( vector_type& vec, float value )noexcept {
		vec.y = value;
	}
};

using chili_vec2_traits = vector2_traits<ChiliVecAccess>;