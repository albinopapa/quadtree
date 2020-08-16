#pragma once

#include <cmath>
#include <utility>

template<typename Vec2MemberAccess> struct vector2_traits {
	using access_traits = Vec2MemberAccess;
	using vector_type = typename Vec2MemberAccess::vector_type;
	using scalar_type = decltype( Vec2MemberAccess::x( std::declval<typename access_traits::vector_type>() ) );

	static constexpr auto construct( scalar_type x_, scalar_type y_ )noexcept {
		return access_traits::construct( x_, y_ );
	}
	static constexpr auto x( vector_type const& vec )noexcept {
		return access_traits::x( vec );
	}
	static constexpr void x( vector_type& vec, scalar_type value )noexcept {
		access_traits::x( vec, value );
	}

	static constexpr auto y( vector_type const& vec )noexcept {
		return access_traits::y( vec );
	}
	static constexpr void y( vector_type& vec, scalar_type value )noexcept {
		access_traits::y( vec, value );
	}

	static constexpr auto dot( vector_type const& lhs, vector_type const& rhs )noexcept {
		return ( x( lhs ) * x( rhs ) ) + ( y( lhs ) * y( rhs ) );
	}
	static constexpr auto length_sqr( vector_type const& lhs )noexcept {
		return dot( lhs, lhs );
	}
	static auto length( vector_type const& vec )noexcept {
		return std::sqrt( length_sqr( vec ) );
	}
	static auto normalize( vector_type const& vec )noexcept {
		static constexpr auto zero = scalar_type( 0 );
		if( x( vec ) == zero && y( vec ) == zero )return vec;

		if constexpr( std::is_floating_point_v<scalar_type> ) {
			const auto len = scalar_type( 1 ) / length( vec );
			return vector_type{ x( vec ) * len, y( vec ) * len };
		}
		else {
			const auto len = length( vec );
			return vector_type{ x( vec ) / len, y( vec ) / len };
		}
	}
};

