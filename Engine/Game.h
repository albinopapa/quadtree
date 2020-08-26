/****************************************************************************************** 
 *	Chili DirectX Framework Version 16.07.20											  *	
 *	Game.h																				  *
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

#include "Ball.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Graphics.h"
#include "Rect.h"
#include "Timer.h"
#include "Vec2.h"
#include "qtree.h"
#include <memory>

class Game
{
public:
	Game( class MainWindow& wnd );
	Game( const Game& ) = delete;
	Game& operator=( const Game& ) = delete;
	void Go();
private:
	void ComposeFrame();
	void UpdateModel();
	void UpdatePositions( Rectf const& node_bounds, std::vector<Ball>& balls, std::vector<Ball>& remove_list, float dt );
	void UpdateCollisions( std::vector<Ball>& balls );
private:
	MainWindow& wnd;
	Graphics gfx;
	/********************************/
	/*  User Variables              */
	/********************************/
	Timer timer;
	qtree<100, chili_rect_traits, chili_vec2_traits, Ball> vtree;
	int collide_min = std::numeric_limits<int>::max();
	int collide_max = std::numeric_limits<int>::min();
	int collide_count = 0;
	std::wstring win_title;
};