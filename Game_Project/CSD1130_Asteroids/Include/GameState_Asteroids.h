/******************************************************************************/
/*!
\file		GameState_Asteroids.h
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	Asteroids Game Main state file. Holds the function Declarations of the 6 main state
functions. Also contains additional functions declaration to help split chunks of code into more separate areas
for easy access/management.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#ifndef CSD1130_GAME_STATE_PLAY_H_
#define CSD1130_GAME_STATE_PLAY_H_

// ---------------------------------------------------------------------------

/*!*************************************************************************
\brief Load assets for asteroid game
***************************************************************************/
void GameStateAsteroidsLoad(void);
/*!*************************************************************************
\brief Initialize any objects/variables for asteroid game
***************************************************************************/
void GameStateAsteroidsInit(void);
/*!*************************************************************************
\brief Update for asteroid game
***************************************************************************/
void GameStateAsteroidsUpdate(void);
/*!*************************************************************************
\brief Render any objects for asteroid game
***************************************************************************/
void GameStateAsteroidsDraw(void);
/*!*************************************************************************
\brief free any variables/objects for asteroid game
***************************************************************************/
void GameStateAsteroidsFree(void);
/*!*************************************************************************
\brief Unload assets for asteroid game
***************************************************************************/
void GameStateAsteroidsUnload(void);

// ---------------------------------------------------------------------------

// Additional Functions
/*!*************************************************************************
\brief Updates the AABB collision boxes for game objects
***************************************************************************/
void UpdateGOCollisionBoxes();
/*!*************************************************************************
\brief Checks the collision between game objects
***************************************************************************/
void CheckGOCollision();
/*!*************************************************************************
\brief Updates the game object instances
***************************************************************************/
void UpdateGO();
/*!*************************************************************************
\brief Create an asteroid with the fixed variables
***************************************************************************/
void CreateAsteroid(AEVec2 _pos, AEVec2 _vel, AEVec2 _scale);
/*!*************************************************************************
\brief Random asteroid at within an area
	

\param[in] min_xPos - the minimum x position
	

\param[in] max_xPos - the maximum x position
	

\param[in] min_yPos - the minimum y position
	

\param[in] max_yPos - the maximuim y position
	

***************************************************************************/
void RandomiseAsteroid(f32 min_xPos, f32 max_xPos, f32 min_yPos, f32 max_yPos);
/*!*************************************************************************
\brief Spawns a new wave of 4 asteroids on each side of the screen
***************************************************************************/
void NewAsteroidWave();
/*!*************************************************************************
\brief Resets the ship when its hit
***************************************************************************/
void  ResetShip();
/*!*************************************************************************
\brief Spawns a random power up
***************************************************************************/
void CreateRandPowerup();
/*!*************************************************************************
\brief Update the power up timers
***************************************************************************/
void UpdatePowerups();
#endif // CSD1130_GAME_STATE_PLAY_H_


