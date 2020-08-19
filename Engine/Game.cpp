/****************************************************************************************** 
 *	Chili DirectX Framework Version 16.07.20											  *	
 *	Game.cpp																			  *
 *	Copyright 2016 PlanetChili.net <http://www.planetchili.net>							  *
 *																						  *
 *	This file is part of The Chili DirectX Framework.									  *
 *																						  *
 *	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
 *	it under the terms of the GNU General Public License as published by				  *
 *	the Free Software Foundation, either version 3 of the License, or					  *
 *	(at your option) any later version.													  *
 *																						  *
 *	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
 *	GNU General Public License for more details.										  *
 *																						  *
 *	You should have received a copy of the GNU General Public License					  *
 *	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
 ******************************************************************************************/
#include "MainWindow.h"
#include "Game.h"
#include <random>

constexpr int max_objects = 10'000;
constexpr auto world_bounds = chili_rect_traits::construct( -5'000.f, -5'000.f, 5'000.f, 5'000.f );
constexpr auto screen_bounds = chili_rect_traits::construct( 
	-float( Graphics::ScreenWidth / 2 ),
	-float( Graphics::ScreenHeight / 2 ),
	float( Graphics::ScreenWidth / 2 ),
	float( Graphics::ScreenHeight / 2 )
);

//#define PTREE
#define VTREE

Game::Game( MainWindow& wnd )
	:
	wnd( wnd ),
	gfx( wnd ),
	//tree( world_bounds, []( Ball const& ball ) { return ball.get_aabb(); } ),
	vtree( world_bounds, []( Ball const& ball ) { return ball.get_aabb(); } )
{
	std::mt19937 rng;
	auto xDist = std::uniform_real_distribution<float>{
		world_bounds.left + Ball::radius,
		world_bounds.right - Ball::radius
	};
	auto yDist = std::uniform_real_distribution<float>{
		world_bounds.top + Ball::radius,
		world_bounds.bottom - Ball::radius
	};

	auto temp = std::vector<Rectf>{ max_objects };
	constexpr auto half_size = Vec2f{ Ball::radius, Ball::radius };
	for( auto& element:temp ) {
		auto pos = Vec2f{ xDist( rng ), yDist( rng ) };
		element = { pos - half_size, pos + half_size };
	}

	//tree.reserve( max_objects );
	for( const auto& rect : temp ) {
		vtree.emplace( chili_rect_traits::template center<ChiliVecAccess>( rect ) );
	}
}

void Game::Go()
{ 
	gfx.BeginFrame();	
	UpdateModel();
	ComposeFrame();
	gfx.EndFrame();
}

void rebound_off_walls( Ball& ball ) {
	auto pos = ball.get_position();
	auto dir = ball.get_direction();
	if( pos.x < world_bounds.left + Ball::radius ) {
		pos.x = world_bounds.left + Ball::radius;
		dir.x *= -1.f;
	}
	else if( pos.x >= world_bounds.right - Ball::radius ) {
		pos.x = world_bounds.right - Ball::radius;
		dir.x *= -1.f;
	}
	if( pos.y < world_bounds.top + Ball::radius ) {
		pos.y = world_bounds.top + Ball::radius;
		dir.y *= -1.f;
	}
	else if( pos.y >= world_bounds.bottom - Ball::radius ) {
		pos.y = world_bounds.bottom - Ball::radius;
		dir.y *= -1.f;
	}

	ball.set_position( pos );
	ball.set_direction( dir );
}

constexpr bool is_colliding( Ball const& lhs, Ball const& rhs )noexcept {
	const auto dist_sqr =
		chili_vec2_traits::length_sqr( lhs.get_position() - rhs.get_position() );
	constexpr auto ball_radius_sqr = Ball::radius * Ball::radius;

	return dist_sqr <= ball_radius_sqr;

}

void resolve( Ball& lhs, Ball& rhs )noexcept {
	auto reflect = []( Vec2f const& n, Vec2f const& d ) {
		return d - ( n * ( 2.f * chili_vec2_traits::dot( n, d ) ) );
	};

	// delta vector from rhs to lhs
	const auto delta = ( lhs.get_position() - rhs.get_position() );

	const auto norm = chili_vec2_traits::normalize( delta );
	const auto dist = chili_vec2_traits::dot( delta, norm );

	// Reflect using normal from lhs to rhs
	const auto rhs_rebound = reflect( -norm, rhs.get_direction() );
	// Reflect using normal from rhs to lhs
	const auto lhs_rebound = reflect(  norm, lhs.get_direction() );

	// Separate to avoid overlap
	const auto overlap = ( Ball::radius - dist ) * 2.f;
	lhs.set_position( lhs.get_position() + (  norm * overlap ) );
	rhs.set_position( rhs.get_position() + ( -norm * overlap ) );

	// Set new directions
	lhs.set_direction( lhs_rebound );
	rhs.set_direction( rhs_rebound );
}

void Game::UpdateModel()
{
#if !defined(DEBUG) && !defined(_DEBUG)
	const auto dt = timer.mark();
#else
	//const auto dt = .016f;
#endif
	const auto dt = timer.mark();

	wnd.SetText( std::to_wstring( 1.f / dt ) );

	for( int count = 0; auto& ball : vtree ) {
		ball.update( dt );
		rebound_off_walls( ball );
		ball.set_normal_color();
		++count;
	}
	
	/*for( auto it = vtree.end(); it != vtree.begin(); --it ) {
		auto pit = it;
		--pit;
		if( !chili_rect_traits::contains( pit.bounds(), pit->get_aabb() ) ) {
			auto temp = std::move( *pit );
			vtree.erase( pit );
			vtree.push( std::move( temp ) );
		}
	}

	for( auto it = vtree.begin(); it != vtree.end(); ++it ) {
		auto colliding = vtree.query( it->get_aabb() );
		std::erase_if( colliding, [&]( Ball const* pvalue ) {return &( *it ) == pvalue; } );

		bool deleted = false;
		for( auto* pball : colliding ) {
			if( is_colliding( *it, *pball ) ) {
				resolve( *it, *pball );
				it->set_collide_color();
				pball->set_collide_color();
				break;
			}
			else {
				it->set_contained_color();
				pball->set_contained_color();
			}
		}
	}*/
}

void Game::ComposeFrame()
{
	/*auto in_view = vtree.query( screen_bounds );
	for( const auto* pball : in_view ) {
		pball->draw( gfx );
	}*/
	//for( const auto& ball : vtree ) {
	//	const auto aabb = ball.get_aabb();
	//	if( chili_rect_traits::intersects( screen_bounds, aabb ) ) {
	//		ball.draw( gfx );
	//	}
	//}
}