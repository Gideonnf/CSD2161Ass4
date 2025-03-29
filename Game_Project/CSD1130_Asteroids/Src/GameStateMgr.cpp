/******************************************************************************/
/*!
\file		GameStateMgr.cpp
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief		Handles the variables to control the current, next and previous states and function definitions
of GameStateMgr

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#include "main.h"

// ---------------------------------------------------------------------------
// globals

// variables to keep track the current, previous and next game state
unsigned int	gGameStateInit;
unsigned int	gGameStateCurr;
unsigned int	gGameStatePrev;
unsigned int	gGameStateNext;

// pointer to functions for game state life cycles functions
void (*GameStateLoad)()		= 0;
void (*GameStateInit)()		= 0;
void (*GameStateUpdate)()	= 0;
void (*GameStateDraw)()		= 0;
void (*GameStateFree)()		= 0;
void (*GameStateUnload)()	= 0;
void (*GameStateProcessMessage)(std::string msg) = 0;

/******************************************************************************/
/*!

*/
/******************************************************************************/
void GameStateMgrInit(unsigned int gameStateInit)
{
	// set the initial game state
	gGameStateInit = gameStateInit;

	// reset the current, previoud and next game
	gGameStateCurr = 
	gGameStatePrev = 
	gGameStateNext = gGameStateInit;

	// call the update to set the function pointers
	GameStateMgrUpdate();
}

/******************************************************************************/
/*!

*/
/******************************************************************************/
void GameStateMgrUpdate()
{
	if ((gGameStateCurr == GS_RESTART) || (gGameStateCurr == GS_QUIT))
		return;

	switch (gGameStateCurr)
	{
		// Assign the function pointers here
	case GS_ASTEROIDS:
		GameStateLoad = GameStateAsteroidsLoad;
		GameStateInit = GameStateAsteroidsInit;
		GameStateUpdate = GameStateAsteroidsUpdate;
		GameStateDraw = GameStateAsteroidsDraw;
		GameStateFree = GameStateAsteroidsFree;
		GameStateUnload = GameStateAsteroidsUnload;
		GameStateProcessMessage = GameStateAsteroidsProcessMessage;
		break;
	case GS_MENU:
		GameStateLoad = GameStateMenuLoad;
		GameStateInit = GameStateMenuInit;
		GameStateUpdate = GameStateMenuUpdate;
		GameStateDraw = GameStateMenuDraw;
		GameStateFree = GameStateMenuFree;
		GameStateUnload = GameStateMenuUnload;
		break;
	default:
		AE_FATAL_ERROR("invalid state!!");
	}
}
