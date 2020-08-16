#pragma once

#include "vector_traits.h"
#include <numeric>
#include <utility>

template<typename RectMemberAccess>
struct rect_traits {
	using access_traits = RectMemberAccess;
	using rect_type = typename RectMemberAccess::rect_type;
	using scalar_type = decltype( RectMemberAccess::left( std::declval<rect_type>() ) );

	static constexpr auto construct( scalar_type left_, scalar_type top_, scalar_type right_, scalar_type bottom_ )noexcept {
		return access_traits::construct( left_, top_, right_, bottom_ );
	}
	static constexpr auto left( rect_type const& rect )noexcept {
		return access_traits::left( rect );
	}
	static constexpr auto top( rect_type const& rect )noexcept {
		return access_traits::top( rect );
	}
	static constexpr auto right( rect_type const& rect )noexcept {
		return access_traits::right( rect );
	}
	static constexpr auto bottom( rect_type const& rect )noexcept {
		return access_traits::bottom( rect );
	}

	static constexpr auto width( rect_type const& rect )noexcept {
		return access_traits::width( rect );
	}
	static constexpr auto height( rect_type const& rect )noexcept {
		return access_traits::height( rect );
	}
	template<typename Vec2MemberAccess>
	static constexpr auto center( rect_type const& rect )noexcept {
		using vec_traits = vector2_traits<Vec2MemberAccess>;
		return vec_traits::construct(
			std::midpoint( left( rect ), right( rect ) ),
			std::midpoint( top( rect ), bottom( rect ) )
		);
	}
	static constexpr bool intersects( rect_type const& lhs, rect_type const& rhs )noexcept {
		return
			( left( lhs ) < right( rhs ) && right( lhs ) > left( rhs ) ) &&
			( top( lhs ) < bottom( rhs ) && bottom( lhs ) > top( rhs ) );
	}
	static constexpr bool contains( rect_type const& lhs, rect_type const& rhs )noexcept {
		return
			( left( lhs ) < left( rhs ) && right( lhs ) > right( rhs ) ) &&
			( top( lhs ) < top( rhs ) && bottom( lhs ) > bottom( rhs ) );
	}
	template<typename Vec2AccessTraits>
	static constexpr bool contains( rect_type const& lhs, typename Vec2AccessTraits::vector_type const& rhs )noexcept {
		return
			( Vec2AccessTraits::x( rhs ) >= left( lhs ) && Vec2AccessTraits::x( rhs ) < right( lhs ) ) &&
			( Vec2AccessTraits::y( rhs ) >= top( lhs ) && Vec2AccessTraits::y( rhs ) < bottom( lhs ) );
	}
};

