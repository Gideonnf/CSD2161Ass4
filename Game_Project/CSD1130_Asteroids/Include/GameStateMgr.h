/******************************************************************************/
/*!
\file		GameStateMgr.h
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief		Handles the variables to control the current, next and previous states and function declarations
of GameStateMgr

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#ifndef CSD1130_GAME_STATE_MGR_H_
#define CSD1130_GAME_STATE_MGR_H_
#include <string>

// ---------------------------------------------------------------------------

#include "AEEngine.h"

// ---------------------------------------------------------------------------
// include the list of game states

#include "GameStateList.h"

// ---------------------------------------------------------------------------
// externs

extern unsigned int gGameStateInit;
extern unsigned int gGameStateCurr;
extern unsigned int gGameStatePrev;
extern unsigned int gGameStateNext;

// ---------------------------------------------------------------------------

extern void (*GameStateLoad)();
extern void (*GameStateInit)();
extern void (*GameStateUpdate)();
extern void (*GameStateDraw)();
extern void (*GameStateFree)();
extern void (*GameStateUnload)();
extern void (*GameStateProcessMessage)();

// ---------------------------------------------------------------------------
// Function prototypes

// 
/*!*************************************************************************
\brief Initializes the Game State Manager. Call this at the beginning and AFTER all game states are added to the manager
	
\param[in] gameStateInit - The state to start the game in

***************************************************************************/
void GameStateMgrInit(unsigned int gameStateInit);

/*!*************************************************************************
\brief Updates the function pointers for the game state manager to the next state functions
***************************************************************************/
void GameStateMgrUpdate();

// ---------------------------------------------------------------------------

#endif