#pragma once

#include "Vec2.h"
#include "rect_traits.h"

template<typename T>
class Rect
{
public:
	Rect() = default;
	constexpr Rect( T left_in, T right_in, T top_in, T bottom_in )noexcept
		:
		left( left_in ),
		right( right_in ),
		top( top_in ),
		bottom( bottom_in )
	{
	}
	constexpr Rect( const Vec2<T>& topLeft, const Vec2<T>& bottomRight )noexcept
		:
		Rect( topLeft.x, bottomRight.x, topLeft.y, bottomRight.y )
	{
	}
	constexpr Rect( const Vec2<T>& topLeft, T width, T height )noexcept
		:
		Rect( topLeft, topLeft + Vec2<T>( width, height ) )
	{
	}
	static Rect FromCenter( const Vec2<T>& center, T halfWidth, T halfHeight )noexcept
	{
		const Vec2<T> half( halfWidth, halfHeight );
		return { center - half, center + half };
	}
public:
	T left = {};
	T right = {};
	T top = {};
	T bottom = {};
};

using Rectf = Rect<float>;
using Recti = Rect<int>;

struct ChiliRectAccess {
	using rect_type = Rectf;
	using scalar_type = float;

	static constexpr auto construct( scalar_type left_, scalar_type top_, scalar_type right_, scalar_type bottom_ )noexcept {
		return rect_type{ left_, right_, top_, bottom_ };
	}
	static constexpr auto left( rect_type const& rect )noexcept {
		return rect.left;
	}
	static constexpr auto top( rect_type const& rect )noexcept {
		return rect.top;
	}
	static constexpr auto right( rect_type const& rect )noexcept {
		return rect.right;
	}
	static constexpr auto bottom( rect_type const& rect )noexcept {
		return rect.bottom;
	}
	static constexpr auto width( rect_type const& rect )noexcept {
		return rect.right - rect.left;
	}
	static constexpr auto height( rect_type const& rect )noexcept {
		return rect.bottom - rect.top;
	}
};

using chili_rect_traits = rect_traits<ChiliRectAccess>;