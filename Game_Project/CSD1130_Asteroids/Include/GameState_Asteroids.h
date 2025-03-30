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
#include <string>

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

/*!*************************************************************************
\brief For receiving messages
***************************************************************************/
void GameStateAsteroidsProcessMessage(std::string msg);

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


/*!

	Data for game

*/
const unsigned int	GAME_OBJ_NUM_MAX = 32;			// The total number of different objects (Shapes)
const unsigned int	GAME_OBJ_INST_NUM_MAX = 2048;			// The total number of different game object instances
/******************************************************************************/
/*!
	Struct/Class Definitions
*/
/******************************************************************************/

//Game object structure
struct GameObj
{
	unsigned long		type;		// object type
	AEGfxVertexList* pMesh;		// This will hold the triangles which will form the shape of the object
	AEGfxTexture* pTexture;     // Hold the textures
};

// ---------------------------------------------------------------------------

//Game object instance structure
struct GameObjInst
{
	GameObj* pObject;	// pointer to the 'original' shape
	unsigned long		flag;		// bit flag or-ed together
	AEVec2				scale;		// scaling value of the object instance
	AEVec2				posCurr;	// object current position

	AEVec2				posPrev;	// object previous position -> it's the position calculated in the previous loop

	AEVec2				velCurr;	// object current velocity
	float				dirCurr;	// object current direction
	AABB				boundingBox;// object bouding box that encapsulates the object
	AEMtx33				transform;	// object transformation matrix: Each frame, 
	// calculate the object instance's transformation matrix and save it here
};


struct GameData
{
	/******************************************************************************/
/*!
	Static Variables
*/
/******************************************************************************/

// list of original object
	 GameObj				sGameObjList[GAME_OBJ_NUM_MAX];				// Each element in this array represents a unique game object (shape)
	 unsigned long		sGameObjNum;								// The number of defined game objects

	// list of object instances
	 GameObjInst			sGameObjInstList[GAME_OBJ_INST_NUM_MAX];	// Each element in this array represents a unique game object instance (sprite)
	 unsigned long		sGameObjInstNum;							// The number of used game object instances

	// pointer to the ship object
	 GameObjInst* spShip;										// Pointer to the "Ship" game object instance

	// pointer to the wall object
	//static GameObjInst *		spWall;										// Pointer to the "Wall" game object instance

	// the score = number of asteroid destroyed
	 unsigned long		sScore;										// Current score

	 bool onValueChange = true;

	TextObj scoreText;
	TextObj endText;
	TextObj endText2;

	TextObj textList[4];

};


#endif // CSD1130_GAME_STATE_PLAY_H_


