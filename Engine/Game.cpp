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

constexpr auto max_objects = std::size_t{ 20'000 };
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
		//vtree.emplace( chili_rect_traits::template center<ChiliVecAccess>( rect ) );
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
	const auto dt = .016f;
#endif
	win_title = L"FPS: " + std::to_wstring( 1.f / dt );

	std::vector<Ball> remove_list;
	for( auto& node : vtree ) {
		UpdatePositions( node.bounds(), node.elements(), remove_list, dt );
	}

	for( auto& ball : remove_list ) {
		vtree.push( std::move( ball ) );
	}
	
	for( auto it = vtree.begin(); it != vtree.end(); ++it ) {
		UpdateCollisions( it->elements() );
	}

	collide_min = std::min( collide_min, collide_count );
	collide_max = std::max( collide_max, collide_count );
	const auto collide_avg = std::midpoint( collide_min, collide_max );

	win_title = win_title + L', ' + std::wstring{
		L"Comparisons( " +
		std::to_wstring( collide_min ) + L',\t' +
		std::to_wstring( collide_max ) + L',\t' +
		std::to_wstring( collide_avg ) + L' )'
	};
	wnd.SetText( win_title );
	collide_count = 0;
}

void Game::UpdatePositions( Rectf const& node_bounds, std::vector<Ball>& balls, std::vector<Ball>& remove_list, float dt )
{
	for( auto ball_it = balls.begin(); ball_it != balls.end(); ) {
		auto& ball = *ball_it;
		ball.update( dt );
		rebound_off_walls( ball );
		ball.set_normal_color();

		if( chili_rect_traits::contains( node_bounds, ball.get_aabb() ) ) {
			++ball_it;
		}
		else {
			remove_list.push_back( std::move( ball ) );
			ball_it = balls.erase( ball_it );
		}
	}
}

void Game::UpdateCollisions( std::vector<Ball>& balls )
{
	for( auto& lball : balls ) {
		vtree.query( lball.get_aabb(), [&]( Ball& rball ) {
			++collide_count;
			if( std::addressof( rball ) == std::addressof( lball ) )return;

			if( !is_colliding( lball, rball ) ) {
				lball.set_contained_color();
				rball.set_contained_color();
			}
			else {
				resolve( lball, rball );
				lball.set_collide_color();
				rball.set_collide_color();
			}
		} );
	}
}

void Game::ComposeFrame()
{
	vtree.query( screen_bounds, [&]( Ball const& ball ) {
		if( chili_rect_traits::intersects( screen_bounds, ball.get_aabb() ) )
			ball.draw( gfx );
	} );
}