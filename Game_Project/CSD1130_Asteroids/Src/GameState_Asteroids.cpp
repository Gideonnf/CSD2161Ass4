/******************************************************************************/
/*!
\file		GameState_Asteroids.cpp
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	Asteroids Game Main state file. Holds the function definitions of the 6 main state
functions. Also contains additional functions to help split chunks of code into more separate areas
for easy access/management.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include <Network.h>
#include "main.h"
#include "ProcessReceive.h"
#include <sstream>
#include <iostream>
/******************************************************************************/
/*!
	Define constant variables that we use in our game
*/
/******************************************************************************/

const float			SHIP_SCALE_X = 45.0f;		// ship scale x
const float			SHIP_SCALE_Y = 45.0f;		// ship scale y
const float			BULLET_SCALE_X = 20.0f;		// bullet scale x
const float			BULLET_SCALE_Y = 3.0f;			// bullet scale y
const float			ASTEROID_MIN_SCALE_X = 10.0f;		// asteroid minimum scale x
const float			ASTEROID_MAX_SCALE_X = 60.0f;		// asteroid maximum scale x
const float			ASTEROID_MIN_SCALE_Y = 10.0f;		// asteroid minimum scale y
const float			ASTEROID_MAX_SCALE_Y = 60.0f;		// asteroid maximum scale y
const float         POWERUP_SCALE = 25.0f;

const float			WALL_SCALE_X = 64.0f;		// wall scale x
const float			WALL_SCALE_Y = 164.0f;		// wall scale y

const float			SHIP_ACCEL_FORWARD = 100.0f;		// ship forward acceleration (in m/s^2)
const float			SHIP_ACCEL_BACKWARD = 100.0f;		// ship backward acceleration (in m/s^2)
const float			SHIP_ROT_SPEED = (2.0f * PI);	// ship rotation speed (degree/second)

const float			BULLET_SPEED = 400.0f;		// bullet speed (m/s)

const float         BOUNDING_RECT_SIZE = 1.0f;         // this is the normalized bounding rectangle (width and height) sizes - AABB collision data

const float			SCREEN_WIDTH = 1280.0f;
const float			SCREEN_HEIGHT = 720.0f;
const float			POWERUP_ACCEL = 50.0f;
const float         POWERUP_TIME = 5.0f;
const float         WAVE_TIME = 4.0f;
const float			FIXED_DELTA_TIME = 0.01667f; // Fixed time step for 60 FPS

// -----------------------------------------------------------------------------
enum TYPE
{
	// list of game object types
	TYPE_SHIP = 0,
	TYPE_BULLET,
	TYPE_ASTEROID,
	//TYPE_WALL,
	//TYPE_POWERUP,
	TYPE_NUM
};

enum POWERUP_TYPE
{
	// list of power up types in the game
	POWERUP_TYPE_SHIELD = 0,
	POWERUP_TYPE_SHOTGUN,
	POWERUP_TYPE_NUM
};

// -----------------------------------------------------------------------------
// object flag definition

const unsigned long FLAG_ACTIVE = 0x00000001;



// Structs shfited to entity.h

// ---------------------------------------------------------------------------

// functions to create/destroy a game object instance
GameObjInst *gameObjInstCreate(unsigned long type, AEVec2 *scale,
	AEVec2 *pPos, AEVec2 *pVel, float dir);
GameObjInst *bulletObjInstCreate(AEVec2 *pPos, AEVec2 *pVel, float dir, uint32_t id = BULLET_ID_MAX);
void				gameObjInstDestroy(GameObjInst *pInst);

// To help render game object instances that holds mesh textures
void RenderMeshObj(GameObjInst *GO);

// Local variables needed are all declared here
// Declared in namespace so it wont affect other files
namespace
{
	static GameData gameData;
	BGObject bgGO;
	AEGfxTexture *asteroidTexture;

	AEGfxTexture *shipTexture;
	AEGfxTexture *shipFireTexture;
	AEGfxTexture *powerUpTexture;


	s8 textFont;
	s8 fontSize;

	// For seeding srand
	time_t timeVar;

	//f64 waveTimer;

	bool gameOver;

	double accumulatedTime = 0.0;


}

/******************************************************************************/
/*!
	"Load" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsLoad(void)
{
	srand((unsigned)time(&timeVar)); //Seed srand in load

	// zero the game object array
	memset(gameData.sGameObjList, 0, sizeof(GameObj) * GAME_OBJ_NUM_MAX);
	// No game objects (shapes) at this point
	gameData.sGameObjNum = 0;

	// zero the game object instance array
	memset(gameData.sGameObjInstList, 0, sizeof(GameObjInst) * GAME_OBJ_INST_NUM_MAX);
	// No game object instances (sprites) at this point
	gameData.sGameObjInstNum = 0;

	// The ship object instance hasn't been created yet, so this "spShip" pointer is initialized to 0
	for (int i = 0; i < 4; ++i)
	{
		gameData.spShip[i] = nullptr;

	}

	// Load the textures
	asteroidTexture = AEGfxTextureLoad("../Resources/Textures/asteroid.png");
	shipTexture = AEGfxTextureLoad("../Resources/Textures/myship.png");
	shipFireTexture = AEGfxTextureLoad("../Resources/Textures/myship_fire.png");
	powerUpTexture = AEGfxTextureLoad("../Resources/Textures/powerup.png");


	// load/create the mesh data (game objects / Shapes)
	GameObj *pObj;

	// =====================
	// create the ship shape
	// =====================

	pObj = gameData.sGameObjList + gameData.sGameObjNum++;
	pObj->type = TYPE_SHIP;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // bottom-left: red
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right: green
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left: blue

	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right: green
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,    // top-right: white
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left: blue

	pObj->pMesh = AEGfxMeshEnd();
	pObj->pTexture = shipTexture;
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");


	// =======================
	// create the bullet shape
	// =======================

	pObj = gameData.sGameObjList + gameData.sGameObjNum++;
	pObj->type = TYPE_BULLET;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFF00, 0.0f, 0.0f,
		0.5f, 0.5f, 0xFFFFFF00, 0.0f, 0.0f,
		-0.5f, 0.5f, 0xFFFFFF00, 0.0f, 0.0f);
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFF00, 0.0f, 0.0f,
		0.5f, -0.5f, 0xFFFFFF00, 0.0f, 0.0f,
		0.5f, 0.5f, 0xFFFFFF00, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");

	// =========================
	// create the asteroid shape
	// =========================

	pObj = gameData.sGameObjList + gameData.sGameObjNum++;
	pObj->type = TYPE_ASTEROID;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // bottom-left: red
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right: green
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left: blue

	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right: green
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,    // top-right: white
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left: blue

	pObj->pMesh = AEGfxMeshEnd();
	pObj->pTexture = asteroidTexture;
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");

	// Create the Background object

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // bottom-left: red
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right: green
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left: blue

	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right: green
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,    // top-right: white
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left: blue

	bgGO.mesh = AEGfxMeshEnd();
	bgGO.meshTex = AEGfxTextureLoad("../Resources/Textures/bg.png");
	AE_ASSERT_MESG(bgGO.mesh, "fail to create object!!");
	AE_ASSERT_MESG(bgGO.meshTex, "fail to create object!!");

	fontSize = 36;
	textFont = AEGfxCreateFont("../Resources/Fonts/Arial-Italic.ttf", fontSize);



}

/******************************************************************************/
/*!
	"Initialize" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsInit(void)
{
	// establish connection???
	NetworkClient::Instance().Init();

	std::cout << AEGfxGetWinMinX() << ", " << AEGfxGetWinMaxX() << ", " << AEGfxGetWinMinY() << ", " << AEGfxGetWinMaxY() << std::endl;

	//waveTimer = 0.0f;

	AEVec2Set(&bgGO.goScale, SCREEN_WIDTH, SCREEN_HEIGHT);
	AEVec2Set(&bgGO.goPosition, 0, 0);

	// Initalizes all the text objects

	AEVec2Set(&gameData.textList[0].pos, (0), (SCREEN_HEIGHT * 0.45f));
	gameData.textList[0].textSize = 20;
	gameData.textList[0].Font = &textFont;
	gameData.textList[0].str = "Score: ";

	AEVec2Set(&gameData.textList[1].pos, 0, 0);
	gameData.textList[1].textSize = 25;
	gameData.textList[1].Font = &textFont;
	gameData.textList[1].str = "GAME OVER";

	AEVec2Set(&gameData.textList[2].pos, 0, -20);
	gameData.textList[2].textSize = 12;
	gameData.textList[2].Font = &textFont;
	gameData.textList[2].str = "Press R to restart";

	AEVec2Set(&gameData.textList[3].pos, 0, -40);
	gameData.textList[3].textSize = 12;
	gameData.textList[3].Font = &textFont;
	gameData.textList[3].str = "Press B to go back to menu";


	// create the main ship
	// i think sending info of ship will be done in update
	AEVec2 scale;
	AEVec2 pos, vel;
	AEVec2Set(&scale, SHIP_SCALE_X, SHIP_SCALE_Y);
	AEVec2Set(&pos, 0, 0);
	AEVec2Set(&vel, 0, 0);

	// instantiate all 4 first
	// but we're only gonna be using the ones that are being used by clients
	for (int i = 0; i < 4; ++i)
	{
		gameData.spShip[i] = gameObjInstCreate(TYPE_SHIP, &scale, &pos, &vel, 0.0f);
		AE_ASSERT(gameData.spShip[i]);
		gameData.spShip[i]->posPrev.x = gameData.spShip[i]->posCurr.x;
		gameData.spShip[i]->posPrev.y = gameData.spShip[i]->posCurr.y;
		gameData.spShip[i]->active = false;
	}


	// no creating anything new i think
	// cause it has to send messages so i comment out for now
	// Create 4 random asteroids
	//NewAsteroidWave();


	// reset the score and the number of ships
	gameData.sScore = 0;
	accumulatedTime = 0.0;

	gameOver = false;
	gameData.onValueChange = true; // To reprint the console if player choose to restart
}

/******************************************************************************/
/*!
	"Update" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsUpdate(void)
{
	int playerInput = 0;
	accumulatedTime += AEFrameRateControllerGetFrameTime();
	while (accumulatedTime >= FIXED_DELTA_TIME)
	{
		// =========================================================
		// update according to input
		// =========================================================
		gameData.spShip[gameData.currID]->pObject->pTexture = shipTexture;
		if (AEInputCheckCurr(AEVK_UP))
		{
			// Get the direction and calculate the velocity and apply it to the current velocity.
			AEVec2 dir;
			AEVec2Set(&dir, cosf(gameData.spShip[gameData.currID]->dirCurr), sinf(gameData.spShip[gameData.currID]->dirCurr));
			AEVec2Normalize(&dir, &dir);

			AEVec2Scale(&dir, &dir, SHIP_ACCEL_FORWARD * (float)(AEFrameRateControllerGetFrameTime()) * 0.99f);
			AEVec2Add(&gameData.spShip[gameData.currID]->velCurr, &gameData.spShip[gameData.currID]->velCurr, &dir);

			gameData.spShip[gameData.currID]->pObject->pTexture = shipFireTexture;

			playerInput = 1;
		}

		if (AEInputCheckCurr(AEVK_DOWN))
		{
			// Get the direction and calculate the velocity and apply it to the current velocity.
			// Apply speed negatively since its going backward
			AEVec2 dir;
			AEVec2Set(&dir, cosf(gameData.spShip[gameData.currID]->dirCurr), sinf(gameData.spShip[gameData.currID]->dirCurr));
			AEVec2Normalize(&dir, &dir);
			AEVec2Scale(&dir, &dir, -SHIP_ACCEL_BACKWARD * (float)(AEFrameRateControllerGetFrameTime()) * 0.99f);
			AEVec2Add(&gameData.spShip[gameData.currID]->velCurr, &gameData.spShip[gameData.currID]->velCurr, &dir);
			gameData.spShip[gameData.currID]->pObject->pTexture = shipFireTexture;

			playerInput = 2;
		}

		if (AEInputCheckCurr(AEVK_LEFT))
		{
			// Rotate the ship, wrap the angle
			gameData.spShip[gameData.currID]->dirCurr += SHIP_ROT_SPEED * (float)(AEFrameRateControllerGetFrameTime());
			gameData.spShip[gameData.currID]->dirCurr = AEWrap(gameData.spShip[gameData.currID]->dirCurr, -PI, PI);

			playerInput = 3;
		}

		if (AEInputCheckCurr(AEVK_RIGHT))
		{
			// Rotate the ship, wrap the angle
			gameData.spShip[gameData.currID]->dirCurr -= SHIP_ROT_SPEED * (float)(AEFrameRateControllerGetFrameTime());
			gameData.spShip[gameData.currID]->dirCurr = AEWrap(gameData.spShip[gameData.currID]->dirCurr, -PI, PI);

			playerInput = 4;
		}

		if (gameOver)
		{
			if (AEInputCheckTriggered(AEVK_R))
			{
				gGameStateNext = GS_RESTART;
			}

			if (AEInputCheckTriggered(AEVK_B))
			{
				gGameStateNext = GS_MENU;
			}
		}

		// Shoot a bullet if space is triggered (Create a new object instance)
		if (AEInputCheckTriggered(AEVK_SPACE))
		{
			AEVec2 pos, vel;
			AEVec2 scale;
			// Creates bullets based on the ship's direction and angle
			pos.x = gameData.spShip[gameData.currID]->posCurr.x;	pos.y = gameData.spShip[gameData.currID]->posCurr.y;
			vel.x = BULLET_SPEED * cosf(gameData.spShip[gameData.currID]->dirCurr);
			vel.y = BULLET_SPEED * sinf(gameData.spShip[gameData.currID]->dirCurr);
			AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
			GameObjInst* bulletObj = bulletObjInstCreate(&pos, &vel, gameData.spShip[gameData.currID]->dirCurr);
			{
				Packet pck(CMDID::BULLET_CREATED);
				pck << gameData.currID << NetworkClient::Instance().GetTimeDiff() << bulletObj->serverID << pos.x << pos.y << vel.x << vel.y << gameData.spShip[gameData.currID]->dirCurr;
				NetworkClient::Instance().CreateMessage(pck);
			}

		}
		if (AEInputCheckTriggered(AEVK_TAB))
		{
			Packet pck(CLIENT_REQ_HIGHSCORE);
			pck << gameData.currID << NetworkClient::Instance().GetTimeDiff();

			// Send packet to server
			NetworkClient::Instance().CreateMessage(pck);
		}

		// Save previous positions
		//  -- For all instances
		// [DO NOT UPDATE THIS PARAGRAPH'S CODE]
		// ======================================================================
		for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
		{
			// Update all the GO Instances's prev positions here
			// Before doing any update in collision and movement
			GameObjInst *pInst = gameData.sGameObjInstList + i;

			// skip non-active object
			if ((pInst->flag & FLAG_ACTIVE) == 0)
				continue;

			pInst->posPrev.x = pInst->posCurr.x;
			pInst->posPrev.y = pInst->posCurr.y;
		}

		// Updates the collision boxes (AABB) of the game objects
		UpdateGOCollisionBoxes();

		// ======================================================================
		// check for dynamic-static collisions (one case only: Ship vs Wall)
		// [DO NOT UPDATE THIS PARAGRAPH'S CODE]
		// ======================================================================
		//Helper_Wall_Collision();

		// ======================================================================
		// check for dynamic-dynamic collisions
		// ======================================================================
		// Loop through GO lists and check collisions between objects
		CheckGOCollision();

		// Update the GOs (i.e movement, etc)
		UpdateGO();


		// =====================================================================
		// calculate the matrix for all objects
		// =====================================================================

		for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
		{
			GameObjInst *pInst = gameData.sGameObjInstList + i;
			AEMtx33		 trans, rot, scale;

			// skip non-active object
			if ((pInst->flag & FLAG_ACTIVE) == 0)
				continue;

			// Compute the scaling matrix
			AEMtx33Scale(&scale, pInst->scale.x, pInst->scale.y);
			// Compute the rotation matrix 
			AEMtx33Rot(&rot, pInst->dirCurr);
			// Compute the translation matrix
			AEMtx33Trans(&trans, pInst->posCurr.x, pInst->posCurr.y);
			// Concatenate the 3 matrix in the correct order in the object instance's "transform" matrix
			AEMtx33Concat(&pInst->transform, &rot, &scale);
			AEMtx33Concat(&pInst->transform, &trans, &pInst->transform);
		}



		if (playerInput != 0)
		{
			Packet pck(CMDID::SHIP_MOVE);
			pck << gameData.currID << NetworkClient::Instance().GetTimeDiff() << playerInput <<
				gameData.spShip[gameData.currID]->posCurr.x << gameData.spShip[gameData.currID]->posCurr.y <<
				gameData.spShip[gameData.currID]->velCurr.x << gameData.spShip[gameData.currID]->velCurr.y <<
				gameData.spShip[gameData.currID]->dirCurr;
			NetworkClient::Instance().CreateMessage(pck);





			//  "Time:" << timestamp << ' ' <<
			//	"Input:" << playerInput << ' ' <<
			//	"Pos:" << gameData.spShip->posCurr.x << ' ' << gameData.spShip->posCurr.y << ' ' <<
			//	"Vel:" << gameData.spShip->velCurr.x << ' ' << gameData.spShip->velCurr.y << ' ' <<
			//	"Dir:" << gameData.spShip->dirCurr;
		}

		accumulatedTime -= FIXED_DELTA_TIME;
	}

}

/******************************************************************************/
/*!
	"Draw" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsDraw(void)
{
	char strBuffer[1024];

	// Render the BG mesh object (The space bg)
	RenderMeshObj(&bgGO);

	// draw all object instances in the list
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst *pInst = gameData.sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;

		if (!pInst->active) continue; // dont render inactive objs

		// If its an gamobject that has a texture on it
		if (pInst->pObject->pTexture != nullptr)
		{
			// Render it using render mesh if it has a texture
			RenderMeshObj(pInst);
		}
		// If its a normal color object
		else
		{
			// If no texture then render it by color (i.e the wall)
			AEGfxSetRenderMode(AE_GFX_RM_COLOR);
			AEGfxTextureSet(NULL, 0, 0);

			// Set blend mode to AE_GFX_BM_BLEND
			// This will allow transparency.
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);
			AEGfxSetTransparency(1.0f);

			// Set the current object instance's transform matrix using "AEGfxSetTransform"
			AEGfxSetTransform(pInst->transform.m);
			// Draw the shape used by the current object instance using "AEGfxMeshDraw"
			AEGfxMeshDraw(pInst->pObject->pMesh, AE_GFX_MDM_TRIANGLES);
		}
	}

	// Handles the rendering of texture objects
	for (int i = 0; i < 4; ++i)
	{
		// Specific checks so  certain text dont render until required
		if (!gameOver && (i == 1 || i == 2 || i == 3)) continue;

		if (gameOver)
		{
			if (gameData.sScore >= 5000)
			{
				gameData.textList[2].str = "GAME OVER! YOU WON";

			}
		}

		// 0 is for score
		// 1 2 and 3 is for game over texts
		// They need to pass in the values when rendering
		if (i == 0)
			snprintf(strBuffer, sizeof(strBuffer), "%s %d", gameData.textList[i].str, gameData.sScore);
		else
			snprintf(strBuffer, sizeof(strBuffer), "%s", gameData.textList[i].str);

		// Render the text object after assigning the string val
		RenderText(&gameData.textList[i], fontSize, strBuffer);
	}


	// Updates the console print of the score and lives when value is changed
	if (gameData.onValueChange)
	{
		sprintf_s(strBuffer, "Score: %d", gameData.sScore);
		//AEGfxPrint(10, 10, (u32)-1, strBuffer);
		printf("%s \n", strBuffer);
		//AEGfxPrint(600, 10, (u32)-1, strBuffer);
		printf("%s \n", strBuffer);

		// Set to false so that it doesn't flood the console with print messages
		gameData.onValueChange = false;
	}
}

/******************************************************************************/
/*!
	"Free" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsFree(void)
{
	// kill all object instances in the array using "gameObjInstDestroy"
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst *pInst = gameData.sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;

		// Destroy the instance (set to the flag)
		gameObjInstDestroy(pInst);
	}

}

/******************************************************************************/
/*!
	"Unload" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsUnload(void)
{
	// free all mesh data (shapes) of each object using "AEGfxTriFree"
	for (int i = 0; i < GAME_OBJ_NUM_MAX; ++i)
	{
		// Check if there is a mesh to free
		if (gameData.sGameObjList[i].pMesh != nullptr)
		{
			// Free the mesh
			AEGfxMeshFree(gameData.sGameObjList[i].pMesh);
		}

		// Check if there is any texture currently pointing to
		if (gameData.sGameObjList[i].pTexture != nullptr)
		{
			// Dont delete the texture here as it is a shared texture
			gameData.sGameObjList[i].pTexture = nullptr;
		}
	}

	// Destroy/unload everything else that the game loaded in
	AEGfxDestroyFont(textFont);
	AEGfxMeshFree(bgGO.mesh);
	AEGfxTextureUnload(asteroidTexture);
	AEGfxTextureUnload(shipTexture);
	AEGfxTextureUnload(shipFireTexture);
	AEGfxTextureUnload(powerUpTexture);
	AEGfxTextureUnload(bgGO.meshTex);
}

/******************************************************************************/
/*!
	Process messages
*/
/******************************************************************************/

void GameStateAsteroidsProcessMessage()
{
	Packet msg = NetworkClient::Instance().GetIncomingMessage();
	// so that i dont have to work on the same file
	// ill handle all message processing in ProcessReceive.h
	ProcessPacketMessages(msg, gameData);


}

/******************************************************************************/
/*!
	Create a game object
*/
/******************************************************************************/
GameObjInst *gameObjInstCreate(unsigned long type,
	AEVec2 *scale,
	AEVec2 *pPos,
	AEVec2 *pVel,
	float dir)
{
	AEVec2 zero;
	AEVec2Zero(&zero);

	AE_ASSERT_PARM(type < gameData.sGameObjNum);

	// loop through the object instance list to find a non-used object instance
	for (unsigned long i = 400; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst *pInst = gameData.sGameObjInstList + i;

		// check if current instance is not used
		if (pInst->flag == 0)
		{
			// it is not used => use it to create the new instance
			pInst->pObject = gameData.sGameObjList + type;

			pInst->flag = FLAG_ACTIVE;
			pInst->active = true;
			pInst->scale = *scale;
			pInst->posCurr = pPos ? *pPos : zero;
			pInst->posPrev = pInst->posCurr;
			pInst->velCurr = pVel ? *pVel : zero;
			pInst->dirCurr = dir;

			// return the newly created instance
			return pInst;
		}
	}

	// cannot find empty slot => return 0
	return 0;
}

GameObjInst *bulletObjInstCreate(AEVec2 *pPos, AEVec2 *pVel, float dir, uint32_t id)
{
	GameObjInst *pInst;
	if (id >= BULLET_ID_MAX)
	{
		pInst = gameData.sGameObjInstList + 4 + (gameData.currID * BULLET_ID_MAX) + gameData.bulletIDCount;
		pInst->serverID = 4 + (gameData.currID * BULLET_ID_MAX) + gameData.bulletIDCount;
		if (++gameData.bulletIDCount >= BULLET_ID_MAX)
		{
			gameData.bulletIDCount = 0;
		}
	}
	else // valid ID
	{
		pInst = gameData.sGameObjInstList + id;
		pInst->serverID = id;
	}
	pInst->pObject = gameData.sGameObjList + TYPE_BULLET;
	pInst->flag = FLAG_ACTIVE;
	pInst->active = true;
	AEVec2 scale;
	AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
	pInst->scale = scale;

	pInst->posCurr = *pPos;
	pInst->posPrev = pInst->posCurr;
	pInst->velCurr = *pVel;
	pInst->dirCurr = dir;

	return pInst;
}

/******************************************************************************/
/*!
	Destroy game object instance
*/
/******************************************************************************/
void gameObjInstDestroy(GameObjInst *pInst)
{
	// if instance is destroyed before, just return
	if (pInst->flag == 0)
		return;

	// zero out the flag
	pInst->flag = 0;
}

/// <summary>
/// For updating active game object instances
/// </summary>
void UpdateGO()
{
	// ===================================================================
	// update active game object instances
	// Example:
	//		-- Wrap specific object instances around the world (Needed for the assignment)
	//		-- Removing the bullets as they go out of bounds (Needed for the assignment)
	//		-- If you have a homing missile for example, compute its new orientation 
	//			(Homing missiles are not required for the Asteroids project)
	//		-- Update a particle effect (Not required for the Asteroids project)
	// ===================================================================
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst *pInst = gameData.sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;

		pInst->posCurr.x += (float)(AEFrameRateControllerGetFrameTime()) * pInst->velCurr.x;
		pInst->posCurr.y += (float)(AEFrameRateControllerGetFrameTime()) * pInst->velCurr.y;

		// check if the object is a ship
		switch (pInst->pObject->type)
		{
		case TYPE_SHIP:
			// Wrap the ship from one end of the screen to the other
			pInst->posCurr.x = AEWrap(pInst->posCurr.x, AEGfxGetWinMinX() - SHIP_SCALE_X,
				AEGfxGetWinMaxX() + SHIP_SCALE_X);
			pInst->posCurr.y = AEWrap(pInst->posCurr.y, AEGfxGetWinMinY() - SHIP_SCALE_Y,
				AEGfxGetWinMaxY() + SHIP_SCALE_Y);

			break;
			// Power up and asteroid has the same movement so reuse, reduce, recycle
		//case TYPE_POWERUP:
		case TYPE_ASTEROID:

			// Wrap asteroids here
			pInst->posCurr.x = AEWrap(pInst->posCurr.x, AEGfxGetWinMinX() - pInst->scale.x,
				AEGfxGetWinMaxX() + pInst->scale.x);
			pInst->posCurr.y = AEWrap(pInst->posCurr.y, AEGfxGetWinMinY() - pInst->scale.y,
				AEGfxGetWinMaxY() + pInst->scale.y);
			break;
		case TYPE_BULLET:
			// Remove bullets that go out of bounds
			if (pInst->posCurr.x < AEGfxGetWinMinX() || pInst->posCurr.x > AEGfxGetWinMaxX() || pInst->posCurr.y < AEGfxGetWinMinY() || pInst->posCurr.y > AEGfxGetWinMaxY())
			{
				// Out of bounds
				gameObjInstDestroy(pInst);
			}
			break;
		}
	}
}

/// <summary>
/// For checking collisions between GOs
/// </summary>
void CheckGOCollision()
{
	//return; // no collision for now
	for (uint32_t i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst *pInst_1 = gameData.sGameObjInstList + i;

		// skip non-active object
		if ((pInst_1->flag & FLAG_ACTIVE) == 0)
			continue;

		// If its an asteroid, then start checking for collision against ship or bullet
		if (pInst_1->pObject->type == TYPE_ASTEROID)
		{
			for (uint32_t j = 0; j < GAME_OBJ_INST_NUM_MAX; j++)
			{
				GameObjInst *pInst_2 = gameData.sGameObjInstList + j;
				if ((pInst_2->flag & FLAG_ACTIVE) == 0) continue;
				if ((pInst_2->pObject->type == TYPE_ASTEROID)) continue;
				float firstTimeOfCollision = 0;

				switch (pInst_2->pObject->type)
				{
				case TYPE_BULLET:
					if (pInst_2->serverID < 4 + (gameData.currID * BULLET_ID_MAX) || pInst_2->serverID > 4 + ((gameData.currID + 1) * BULLET_ID_MAX))
						break;
					if (CollisionIntersection_RectRect(pInst_1->boundingBox, pInst_1->velCurr, pInst_2->boundingBox, pInst_2->velCurr, firstTimeOfCollision))
					{
						// Destroy both the bullet and asteroid after
						gameObjInstDestroy(pInst_1);
						gameObjInstDestroy(pInst_2);

						if (!gameOver)
						{
							// increment score
							gameData.sScore += 100;
							gameData.onValueChange = true;
							if (gameData.sScore >= 5000)
								gameOver = true;
						}

						{
							Packet pck(CMDID::BULLET_COLLIDE);

							pck << gameData.currID << NetworkClient::Instance().GetTimeDiff() << j << i << gameData.sScore;
							//  "Time:" << timestamp << ' ' <<
							//	"BulletID:" << j << ' ' <<
							//	"AsteroidID:" << i << ' ' <<
							//	"PlayerScore:" << gameData.sScore;
							NetworkClient::Instance().CreateMessage(pck);
						}

					}
					break;

				case TYPE_SHIP:
					// If its colliding with the ship
					if (CollisionIntersection_RectRect(pInst_1->boundingBox, pInst_1->velCurr, pInst_2->boundingBox, pInst_2->velCurr, firstTimeOfCollision))
					{

						// Destroy the asteroid and update the ship live
						gameObjInstDestroy(pInst_1);

						if (pInst_2->serverID == gameData.currID)
						{
							ResetShip();

							{
								Packet pck(CMDID::SHIP_COLLIDE);
								pck << pInst_2->serverID << NetworkClient::Instance().GetTimeDiff() << i;
								//	"Time:" << timestamp << ' ' <<
								//	"AsteroidID:" << i;
								NetworkClient::Instance().CreateMessage(pck);

								Packet pck2(CMDID::SHIP_MOVE);
								pck2 << pInst_2->serverID  << NetworkClient::Instance().GetTimeDiff() << 0 <<
									gameData.spShip[pInst_2->serverID ]->posCurr.x << gameData.spShip[pInst_2->serverID ]->posCurr.y <<
									gameData.spShip[pInst_2->serverID ]->velCurr.x << gameData.spShip[pInst_2->serverID ]->velCurr.y <<
									gameData.spShip[pInst_2->serverID ]->dirCurr;
								NetworkClient::Instance().CreateMessage(pck2);
							}
						}

					}
					break;
				}
			}
		}
	}
}

/// <summary>
/// Update the AABB for game object instances
/// </summary>
void UpdateGOCollisionBoxes()
{
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst *pInst = gameData.sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;

		AEVec2 minBounding, maxBounding;
		AEVec2 scale;
		AEVec2Scale(&scale, &pInst->scale, -(BOUNDING_RECT_SIZE * 0.5f));
		AEVec2Add(&minBounding, &scale, &pInst->posPrev);

		AEVec2Scale(&scale, &pInst->scale, (BOUNDING_RECT_SIZE * 0.5f));
		AEVec2Add(&maxBounding, &scale, &pInst->posPrev);

		pInst->boundingBox.min = minBounding;
		pInst->boundingBox.max = maxBounding;

	}
}

/// <summary>
/// Create an asteroid with a pre calculated position, velocity and scale
/// </summary>
/// <param name="_pos">Position of the new GO</param>
/// <param name="_vel">Velocity of the new GO</param>
/// <param name="_scale">Scale of the new GO</param>
GameObjInst *CreateAsteroid(AEVec2 _pos, AEVec2 _vel, AEVec2 _scale, float dir)
{
	return gameObjInstCreate(TYPE_ASTEROID, &_scale, &_pos, &_vel, 0.0f);

}

/// <summary>
/// Spawn the wave of asteroid and power up
/// </summary>
//void NewAsteroidWave()
//{
//	// Create 4 asteroids every wave
//
//	// Spawn from bottom of screen
//	RandomiseAsteroid(AEGfxGetWinMinX(), AEGfxGetWinMaxX(), AEGfxGetWinMinY(), AEGfxGetWinMinY());
//	// Spawn from top of screen
//	RandomiseAsteroid(AEGfxGetWinMinX(), AEGfxGetWinMaxX(), AEGfxGetWinMaxY(), AEGfxGetWinMaxY());
//	//// Spawn from Left of screen
//	RandomiseAsteroid(AEGfxGetWinMinX(), AEGfxGetWinMinX(), AEGfxGetWinMinY(), AEGfxGetWinMaxY());
//	//// Spawn from Right of screen
//	RandomiseAsteroid(AEGfxGetWinMaxX(), AEGfxGetWinMaxX(), AEGfxGetWinMinY(), AEGfxGetWinMaxY());
//
//	// Create a power up when a wave spawns
//	//CreateRandPowerup();
//}

/// <summary>
/// Create a randomised asteroid within a fixed boundary
/// </summary>
/// <param name="min_xPos">The minimum AABB x pos</param>
/// <param name="max_xPos">The maximum AABB x pos</param>
/// <param name="min_yPos">The minimum AABB y</param>
/// <param name="max_yPos">The maximum AABB y</param>
//void RandomiseAsteroid(f32 min_xPos, f32 max_xPos, f32 min_yPos, f32 max_yPos)
//{
//	AEVec2 pos, vel, scale;
//	pos.x = AERandFloat() * (max_xPos - min_xPos) + min_xPos;
//	pos.y = AERandFloat() * (max_yPos - min_yPos) + min_yPos;
//	float randAng = AERandFloat() * 360;
//
//	AEVec2 dir;
//	AEVec2Set(&dir, cosf(randAng), sinf(randAng));
//	AEVec2Normalize(&dir, &dir);
//	AEVec2Zero(&vel);
//
//	AEVec2Scale(&dir, &dir, ASTEROID_ACCEL);
//	AEVec2Add(&vel, &vel, &dir);
//
//	float randScaleX = AERandFloat() * (ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X) + ASTEROID_MIN_SCALE_X;
//	float randScaleY = AERandFloat() * (ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y) + ASTEROID_MIN_SCALE_Y;
//	AEVec2Set(&scale, randScaleX, randScaleY);
//
//	// Create after randomising
//	CreateAsteroid(pos, vel, scale);
//}

/// <summary>
/// Reset the ship when it gets hit by an asteroid
/// </summary>
void ResetShip()
{
	AEVec2Set(&gameData.spShip[gameData.currID]->posCurr, 0, 0);
	AEVec2Zero(&gameData.spShip[gameData.currID]->velCurr);
	gameData.spShip[gameData.currID]->dirCurr = 0.0f;
}

/// <summary>
/// Renders a mesh object with a texture
/// </summary>
/// <param name="GO">The game object to render</param>
void RenderMeshObj(GameObjInst *GO)
{
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	AEGfxTextureSet(GO->pObject->pTexture, 0, 0);

	AEGfxSetTransform(GO->transform.m);

	AEGfxMeshDraw(GO->pObject->pMesh, AE_GFX_MDM_TRIANGLES);

}

void ProcessPacketMessages(Packet &msg, GameData &data)
{
	if (msg.id == PACKET_ERROR) return;

	//Packet newPacket(msg);

	//	char msgID = msg[0];

	int32_t clientID;

	switch (msg.id)
	{
	case REPLY_PLAYER_JOIN:
	{
		msg >> clientID;
		gameData.spShip[clientID]->active = true;
		gameData.spShip[clientID]->serverID = clientID;
		gameData.currID = clientID;

		int totalAsteroids;
		msg >> totalAsteroids;

		if (totalAsteroids > 0)
		{
			for (int i = 0; i < totalAsteroids; ++i)
			{
				AEVec2 pos;
				AEVec2 vel;
				AEVec2 scale;
				scale.x = 20.0f;
				scale.y = 20.0f;
				float dirCur;
				int id;
				msg >> id >> pos.x >> pos.y >> vel.x >> vel.y >> dirCur;


				GameObjInst *asteroid = CreateAsteroid(pos, vel, scale, dirCur);
				asteroid->active = true;
				asteroid->serverID = id;
			}
		}

		NetworkClient::Instance().SetShutdownPCK(clientID);
		break;

	}
	case NEW_PLAYER_JOIN:
	{
		// get how many player ids to pull
		int num;
		msg >> num; // get the num of players
		for (size_t i = 0; i < num; ++i)
		{
			msg >> clientID;
			//if (clientID == gameData.currID)
			//{
			//	
			//	continue;
			//}
			msg >> (float)gameData.spShip[clientID]->posCurr.x;
			msg >> (float)gameData.spShip[clientID]->posCurr.y;
			msg >> (float)gameData.spShip[clientID]->velCurr.x;
			msg >> (float)gameData.spShip[clientID]->velCurr.x;
			msg >> gameData.spShip[clientID]->dirCurr;
			gameData.spShip[clientID]->serverID = clientID;
			gameData.spShip[clientID]->active = true;
		}

		break;

	}
	case STATE_UPDATE:
	{
		int numOfShips;
		msg >> numOfShips;
		for (int i = 0; i < numOfShips; ++i)
		{
			msg >> clientID;

			// i cant continue cause i need to update readPos
			//if (clientID == gameData.currID)
			//{
			//	// nvm i can just update read pos fk it
			//	msg.readPos += 20; // 5 floats * 4 = 20 bytes to skip
			//	continue;
			//}
			msg >> (float)gameData.spShip[clientID]->posCurr.x;
			msg >> (float)gameData.spShip[clientID]->posCurr.y;
			msg >> (float)gameData.spShip[clientID]->velCurr.x;
			msg >> (float)gameData.spShip[clientID]->velCurr.x;
			msg >> gameData.spShip[clientID]->dirCurr;
		}

		//for (int i = 0; i < 4; ++i)
		//{
		//	if (!gameData.spShip[i]->active) continue;

		//	//ClientInfo& client = serverData.totalClients[i];

		//	std::cout << "Ship " << i << "\n"
		//		<< "Curr Pos : " << gameData.spShip[i]->posCurr.x << ", " << gameData.spShip[i]->posCurr.y << "\n"
		//		<< "Curr Vel : " << gameData.spShip[i]->velCurr.x << ", " << gameData.spShip[i]->velCurr.y << "\n";

		//}


		break;
	}
	// switch cases
	case ASTEROID_CREATED: // temporary

		int numOfAsteroids;
		msg >> numOfAsteroids;
		for (int i = 0; i < numOfAsteroids; ++i)
		{
			AEVec2 pos;
			AEVec2 vel;
			AEVec2 scale;
			scale.x = 20.0f;
			scale.y = 20.0f;
			float dirCur;
			int id;
			msg >> id >> pos.x >> pos.y >> vel.x >> vel.y >> dirCur;


			GameObjInst *asteroid = CreateAsteroid(pos, vel, scale, dirCur);
			asteroid->active = true;
			asteroid->serverID = id;
			gameData.asteroidMap[id] = asteroid;
		}
		//ProcessNewAsteroid(msg, data);
		break;
	case ASTEROID_UPDATE:
	{
		int numOfAsteroids;
		msg >> numOfAsteroids;
		for (int i = 0; i < numOfAsteroids; ++i)
		{
			int asteroidID;
			msg >> asteroidID;
			AEVec2 pos;
			AEVec2 vel;
			AEVec2 scale;
			scale.x = 20.0f;
			scale.y = 20.0f;
			float dirCur;
			msg >> pos.x >> pos.y >> vel.x >> vel.y >> dirCur;

			if (gameData.asteroidMap.count(asteroidID) > 0)
			{

				// exist
				gameData.asteroidMap[asteroidID]->posCurr = pos;
				gameData.asteroidMap[asteroidID]->velCurr = vel;
				gameData.asteroidMap[asteroidID]->dirCurr = dirCur;

			}
			else
			{
				// it hasn't been created yet
				// usually for new clients
				GameObjInst *asteroid = CreateAsteroid(pos, vel, scale, dirCur);
				asteroid->active = true;
				asteroid->serverID = asteroidID;
				gameData.asteroidMap[asteroidID] = asteroid;
			}
		}

		break;
	}
	case BULLET_CREATED:
	{
		// "Time:" << NetworkClient::Instance().GetTimeDiff() << ' ' <<
		// "ID:" << bulletID <<
		uint64_t timeDiff;
		uint32_t bulletID;
		msg >> clientID;

		msg >> timeDiff >> bulletID;

		AEVec2 pos, vel;
		float dir;
		// "Pos:" << pos.x << ' ' << pos.y << ' ' <<
		// "Vel:" << vel.x << ' ' << vel.y << ' ' <<
		// "Dir:" << gameData.spShip[gameData.currID]->dirCurr;
		msg >> pos.x >> pos.y;
		msg >> vel.x >> vel.y;
		msg >> dir;

		GameObjInst *pInst = bulletObjInstCreate(&pos, &vel, dir, bulletID);

		// Calculate timeDiff
	}
	break;
	case SHIP_MOVE:
		//  "Time:" << timestamp << ' ' <<
		//	"ShipNum:" << gameData.currID << ' ' <<
		//	"Pos:" << gameData.spShip->posCurr.x << ' ' << gameData.spShip->posCurr.y << ' ' <<
		//	"Vel:" << gameData.spShip->velCurr.x << ' ' << gameData.spShip->velCurr.y << ' ' <<
		//	"Dir:" << gameData.spShip->dirCurr;
	{

		msg >> clientID;
		if (clientID == gameData.currID) break;
		uint64_t timeDiff;
		int playerInput;
		msg >> timeDiff >> playerInput;
		msg >> gameData.spShip[clientID]->posCurr.x >> gameData.spShip[clientID]->posCurr.y >>
			gameData.spShip[clientID]->velCurr.x >> gameData.spShip[clientID]->velCurr.y >>
			gameData.spShip[clientID]->dirCurr;

		gameData.spShip[clientID]->posPrev = gameData.spShip[clientID]->posCurr;
	}
	break;
	case BULLET_COLLIDE:
		//  "Time:" << timestamp << ' ' <<
		//	"BulletID:" << j << ' ' <<
		//	"AsteroidID:" << i << ' ' <<
		//	"PlayerScore:" << gameData.sScore;
	{
		uint64_t timeDiff;
		uint32_t bulletID, asteroidID;
		msg >> clientID;

		msg >> timeDiff >> bulletID >> asteroidID;

		// if destroy happen before create (???)

		GameObjInst *pInst = gameData.sGameObjInstList + bulletID;
		pInst->active = false;
		pInst = gameData.sGameObjInstList + asteroidID;
		pInst->active = false;
	}
	break;
	case SHIP_COLLIDE:
		//	"Time:" << timestamp << ' ' <<
		//	"AsteroidID:" << i;
	{
		uint64_t timeDiff;
		uint32_t asteroidID;
		msg >> clientID;

		msg >> timeDiff >> asteroidID;

		GameObjInst *pInst = gameData.sGameObjInstList + asteroidID;
		pInst->active = false;
	}
	break;
	case CLIENT_REQ_HIGHSCORE:
	{
		msg >> clientID;
		uint64_t timeDiff;
		msg >> timeDiff;

		std::cout << "Player " << clientID << " pressed TAB at time " << timeDiff << std::endl;

		// Additional logic here if needed (e.g., triggering an event)
		break;
	}
	break;
	}
}

void ProcessNewAsteroid(Packet &packet, GameData &data)
{
	//float xPos;
	//packet >> xPos;





}
