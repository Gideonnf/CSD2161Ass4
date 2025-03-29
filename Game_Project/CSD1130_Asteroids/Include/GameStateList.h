/******************************************************************************/
/*!
\file		GameStateList.h
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   		February 09 2024
\brief		Holds the enums for the game states

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#ifndef CSD1130_GAME_STATE_LIST_H_
#define CSD1130_GAME_STATE_LIST_H_

// ---------------------------------------------------------------------------
// game state list

// Holds all the game states 
enum
{
	// list of all game states 
	GS_ASTEROIDS = 0, 
	GS_MENU,
	
	// special game state. Do not change
	GS_RESTART,
	GS_QUIT, 
	GS_NONE
};

// ---------------------------------------------------------------------------

#endif // CSD1130_GAME_STATE_LIST_H_