#pragma once

#include "vector_traits.h"
#include <numeric>
#include <utility>

// RectMemberAccess template parameter must be a struct with 
// the folling format

//
//	struct RectAccess {
//		using rect_type = Rectf;	// Some type representing a rectangle
//		using scalar_type = float;	// a type that would define a bounary value like float or int
//	
//		static constexpr auto construct( scalar_type left_, scalar_type top_, scalar_type right_, scalar_type bottom_ )noexcept;
//		static constexpr auto left( rect_type const& rect )noexcept;
//		static constexpr auto top( rect_type const& rect )noexcept;
//		static constexpr auto right( rect_type const& rect )noexcept;
//		static constexpr auto bottom( rect_type const& rect )noexcept;
//		static constexpr auto width( rect_type const& rect )noexcept;
//		static constexpr auto height( rect_type const& rect )noexcept;
//	};
//	
//	An example using SFML FloatRect would be:
//
//	struct SFMLRectAccess {
//		using rect_type = FloatRect;
//		using scalar_type = float;
//
//		static constexpr auto construct( scalar_type left_, scalar_type top_, scalar_type right_, scalar_type bottom_ )noexcept {
//			return rect_type{ left_, top_, right_ - left_, top_ - bottom_ };
//		}
//		static constexpr auto left( rect_type const& rect )noexcept {
//			return rect.left;
//		}
//		static constexpr auto top( rect_type const& rect )noexcept {
//			return rect.top;
//		}
//		static constexpr auto right( rect_type const& rect )noexcept {
//			return rect.left + rect.width;
//		}
//		static constexpr auto bottom( rect_type const& rect )noexcept {
//			return rect.top + rect.height;
//		}
//		static constexpr auto width( rect_type const& rect )noexcept {
//			return rect.width;
//		}
//		static constexpr auto height( rect_type const& rect )noexcept {
//			return rect.height;
//		}
//	};
//
// Parameters are passed to your RectAccess::construct() function
// so that left <= right and top <= bottom ( oriented based on screen coordinate system )

template<typename RectMemberAccess> 
struct rect_traits{
	using access_traits = RectMemberAccess;
	using rect_type = typename RectMemberAccess::rect_type;
	using scalar_type = decltype( RectMemberAccess::left( std::declval<rect_type>() ) );

	static constexpr auto construct( scalar_type left_, scalar_type top_, scalar_type right_, scalar_type bottom_ )noexcept {
		auto minmax = []( scalar_type a, scalar_type b ) {
			if( a <= b )
				return std::pair{ a,b };
			else
				return std::pair{ b,a };
		};

		auto [left_bound, right_bound] = minmax( left_, right_ );
		auto [top_bound, bottom_bound] = minmax( top_, bottom_ );

		return access_traits::construct( left_bound, top_bound, right_bound, bottom_bound );
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
		return Vec2MemberAccess::construct(
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
