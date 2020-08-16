#pragma once

#include "Colors.h"
#include "Rect.h"
#include "Vec2.h"
#include "Graphics.h"

class Ball {
public:
	Ball() = default;
	Ball( Vec2f const& pos )
		:
		position( pos )
	{
		static int i = 0;
		if( i % 4 == 0 ) {
			direction = chili_vec2_traits::normalize( { .707f, .707f } );
		}
		else if( i % 4 == 1 ) {
			direction = chili_vec2_traits::normalize( { .707f, -.707f } );
		}
		else if( i % 4 == 2 ) {
			direction = chili_vec2_traits::normalize( { -.707f, .707f } );
		}
		else {
			direction = chili_vec2_traits::normalize( { -.707f, -.707f } );
		}
		++i;
	}
	void update( float dt ) {
		position += ( direction * ( speed * dt ) );
	}
	void draw( Graphics& gfx )const noexcept {
		gfx.DrawCircle(
			int( position.x ) + ( Graphics::ScreenWidth / 2 ),
			int( position.y ) + ( Graphics::ScreenHeight / 2 ),
			int( radius ),
			color
		);
	}

	Rectf get_aabb()const noexcept {
		const auto half_size = Vec2f{ radius, radius };
		return { position - half_size, position + half_size };
	}
	Vec2f const& get_position()const noexcept {
		return position;
	}
	Vec2f const& get_direction()const noexcept {
		return direction;
	}
	void set_position( Vec2f const& value )noexcept {
		position = value;
	}
	void set_direction( Vec2f const& value )noexcept {
		direction = value;
	}
	float get_radius()const noexcept {
		return radius;
	}
	void set_normal_color()noexcept {
		color = normal_color;
	}
	void set_contained_color()noexcept {
		color = contained_color;
	}
	void set_collide_color()noexcept {
		color = collide_color;
	}
public:
	static constexpr float radius = 10.f;
private:
	static constexpr Color normal_color = Colors::Red;
	static constexpr Color contained_color = Colors::Yellow;
	static constexpr Color collide_color = Colors::Green;
	static constexpr float speed = 240.f;
	Vec2f position;
	Vec2f direction;

	Color color = normal_color;
};

