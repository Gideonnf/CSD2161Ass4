/******************************************************************************/
/*!
\file		GameState_Menu.h
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	The menu game state file. Holds the 6 main declaration functions. This file will
Update input in the main menu and render any objects needed for the main menu.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/


#ifndef GAMESTATE_MENU_H
#define GAMESTATE_MENU_H
#include "Main.h"


/*!*************************************************************************
\brief Loads assets for menu game state
***************************************************************************/
void GameStateMenuLoad();

/*!*************************************************************************
\brief Initialize data for menu game state
***************************************************************************/
void GameStateMenuInit();

/*!*************************************************************************
\brief Update loop for menu game st ate
***************************************************************************/
void GameStateMenuUpdate();

/*!*************************************************************************
\brief Render for menu game st ate
***************************************************************************/
void GameStateMenuDraw();

/*!*************************************************************************
\brief free any variables for menu game st ate
***************************************************************************/
void GameStateMenuFree();

/*!*************************************************************************
\brief Unload any assets for menu game st ate
***************************************************************************/
void GameStateMenuUnload();

#endif
