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

#include "main.h"

/******************************************************************************/
/*!
	Define constant variables that we use in our game
*/
/******************************************************************************/
const unsigned int	GAME_OBJ_NUM_MAX		= 32;			// The total number of different objects (Shapes)
const unsigned int	GAME_OBJ_INST_NUM_MAX	= 2048;			// The total number of different game object instances


const unsigned int	SHIP_INITIAL_NUM		= 3;			// initial number of ship lives
const float			SHIP_SCALE_X			= 45.0f;		// ship scale x
const float			SHIP_SCALE_Y			= 45.0f;		// ship scale y
const float			BULLET_SCALE_X			= 20.0f;		// bullet scale x
const float			BULLET_SCALE_Y			= 3.0f;			// bullet scale y
const float			ASTEROID_MIN_SCALE_X	= 10.0f;		// asteroid minimum scale x
const float			ASTEROID_MAX_SCALE_X	= 60.0f;		// asteroid maximum scale x
const float			ASTEROID_MIN_SCALE_Y	= 10.0f;		// asteroid minimum scale y
const float			ASTEROID_MAX_SCALE_Y	= 60.0f;		// asteroid maximum scale y
const float           POWERUP_SCALE = 25.0f;

const float			WALL_SCALE_X			= 64.0f;		// wall scale x
const float			WALL_SCALE_Y			= 164.0f;		// wall scale y

const float			SHIP_ACCEL_FORWARD		= 100.0f;		// ship forward acceleration (in m/s^2)
const float			SHIP_ACCEL_BACKWARD		= 100.0f;		// ship backward acceleration (in m/s^2)
const float			SHIP_ROT_SPEED			= (2.0f * PI);	// ship rotation speed (degree/second)

const float			BULLET_SPEED			= 400.0f;		// bullet speed (m/s)

const float         BOUNDING_RECT_SIZE      = 1.0f;         // this is the normalized bounding rectangle (width and height) sizes - AABB collision data

const float			SCREEN_WIDTH = 1280.0f;
const float			SCREEN_HEIGHT = 720.0f;
const float         ASTEROID_ACCEL = 100.0f;
const float			POWERUP_ACCEL = 50.0f;
const float         POWERUP_TIME = 5.0f;
const float         WAVE_TIME = 4.0f;

// -----------------------------------------------------------------------------
enum TYPE
{
	// list of game object types
	TYPE_SHIP = 0,
	TYPE_BULLET,
	TYPE_ASTEROID,
	TYPE_WALL,
	TYPE_POWERUP,
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


// Structs shfited to entity.h

/******************************************************************************/
/*!
	Static Variables
*/
/******************************************************************************/

// list of original object
static GameObj				sGameObjList[GAME_OBJ_NUM_MAX];				// Each element in this array represents a unique game object (shape)
static unsigned long		sGameObjNum;								// The number of defined game objects

// list of object instances
static GameObjInst			sGameObjInstList[GAME_OBJ_INST_NUM_MAX];	// Each element in this array represents a unique game object instance (sprite)
static unsigned long		sGameObjInstNum;							// The number of used game object instances

// pointer to the ship object
static GameObjInst *		spShip;										// Pointer to the "Ship" game object instance

// pointer to the wall object
static GameObjInst *		spWall;										// Pointer to the "Wall" game object instance

// number of ship available (lives 0 = game over)
static long					sShipLives;									// The number of lives left

// the score = number of asteroid destroyed
static unsigned long		sScore;										// Current score

static bool onValueChange = true;

// ---------------------------------------------------------------------------

// functions to create/destroy a game object instance
GameObjInst *		gameObjInstCreate (unsigned long type, AEVec2* scale,
											   AEVec2 * pPos, AEVec2 * pVel, float dir);
void				gameObjInstDestroy(GameObjInst * pInst);

void				Helper_Wall_Collision();

// To help render game object instances that holds mesh textures
void RenderMeshObj(GameObjInst* GO);

// Local variables needed are all declared here
// Declared in namespace so it wont affect other files
namespace
{
	BGObject bgGO;
	AEGfxTexture* asteroidTexture;

	AEGfxTexture* shipTexture;
	AEGfxTexture* shipFireTexture;
	AEGfxTexture* powerUpTexture;


	s8 textFont;
	s8 fontSize;

	TextObj scoreText;
	TextObj liveText;
	TextObj endText;
	TextObj endText2;

	TextObj textList[7];

	// For seeding srand
	time_t timeVar;

	// Keep track of ship's abilities here
	int shipShield;
	int shipShotgun;
	f64 shieldTimer;
	f64 shotgunTimer;

	f64 waveTimer;

	bool gameOver;
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
	memset(sGameObjList, 0, sizeof(GameObj) * GAME_OBJ_NUM_MAX);
	// No game objects (shapes) at this point
	sGameObjNum = 0;

	// zero the game object instance array
	memset(sGameObjInstList, 0, sizeof(GameObjInst) * GAME_OBJ_INST_NUM_MAX);
	// No game object instances (sprites) at this point
	sGameObjInstNum = 0;

	// The ship object instance hasn't been created yet, so this "spShip" pointer is initialized to 0
	spShip = nullptr;

	// Load the textures
	asteroidTexture = AEGfxTextureLoad("../Resources/Textures/asteroid.png");
	shipTexture = AEGfxTextureLoad("../Resources/Textures/myship.png");
	shipFireTexture = AEGfxTextureLoad("../Resources/Textures/myship_fire.png");
	powerUpTexture = AEGfxTextureLoad("../Resources/Textures/powerup.png");


	// load/create the mesh data (game objects / Shapes)
	GameObj * pObj;

	// =====================
	// create the ship shape
	// =====================

	pObj		= sGameObjList + sGameObjNum++;
	pObj->type	= TYPE_SHIP;

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

	pObj = sGameObjList + sGameObjNum++;
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

	pObj = sGameObjList + sGameObjNum++;
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

	// =========================
	// create the wall shape
	// =========================

	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_WALL;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0x6600FF00, 0.0f, 0.0f,
		0.5f, 0.5f, 0x6600FF00, 0.0f, 0.0f,
		-0.5f, 0.5f, 0x6600FF00, 0.0f, 0.0f);
	AEGfxTriAdd(
		-0.5f, -0.5f, 0x6600FF00, 0.0f, 0.0f,
		0.5f, -0.5f, 0x6600FF00, 0.0f, 0.0f,
		0.5f, 0.5f, 0x6600FF00, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");	

	// =========================
	// create power up shape
	// =========================

	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_POWERUP;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,  // bottom-left
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left

	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,   // bottom-right
		0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,    // top-right
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top-left

	pObj->pMesh = AEGfxMeshEnd();
	pObj->pTexture = powerUpTexture;
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

	shieldTimer = 0.0f;
	shotgunTimer = 0.0f;
}

/******************************************************************************/
/*!
	"Initialize" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsInit(void)
{
	waveTimer = 0.0f;

	AEVec2Set(&bgGO.goScale, SCREEN_WIDTH, SCREEN_HEIGHT);
	AEVec2Set(&bgGO.goPosition, 0, 0);

	// Initalizes all the text objects

	AEVec2Set(&textList[0].pos, (0), (SCREEN_HEIGHT * 0.45f));
	textList[0].textSize = 20;
	textList[0].Font = &textFont;
	textList[0].str = "Score: ";

	AEVec2Set(&textList[1].pos, (SCREEN_WIDTH * 0.45f * -1), (SCREEN_HEIGHT * 0.45f));
	textList[1].textSize = 20;
	textList[1].Font = &textFont;
	textList[1].str = "Ships Left: ";

	AEVec2Set(&textList[2].pos, 0, 0);
	textList[2].textSize = 25;
	textList[2].Font = &textFont;
	textList[2].str = "GAME OVER";

	AEVec2Set(&textList[3].pos, 0, -20);
	textList[3].textSize = 12;
	textList[3].Font = &textFont;
	textList[3].str = "Press R to restart";

	AEVec2Set(&textList[4].pos, (SCREEN_WIDTH * 0.445f * -1), (SCREEN_HEIGHT * 0.42f));
	textList[4].textSize = 20;
	textList[4].Font = &textFont;
	textList[4].str = "Shield is Active";

	AEVec2Set(&textList[5].pos, (SCREEN_WIDTH * 0.44f * -1), (SCREEN_HEIGHT * 0.39f));
	textList[5].textSize = 20;
	textList[5].Font = &textFont;
	textList[5].str = "Shotgun is Active";

	AEVec2Set(&textList[6].pos, 0, -40);
	textList[6].textSize = 12;
	textList[6].Font = &textFont;
	textList[6].str = "Press B to go back to menu";


	// create the main ship
	AEVec2 scale;
	AEVec2 pos, vel;
	AEVec2Set(&scale, SHIP_SCALE_X, SHIP_SCALE_Y);
	AEVec2Set(&pos, 0, 0);
	AEVec2Set(&vel, 0, 0);

	spShip = gameObjInstCreate(TYPE_SHIP, &scale, &pos, &vel, 0.0f);
	AE_ASSERT(spShip);
	spShip->posPrev.x = spShip->posCurr.x;
	spShip->posPrev.y = spShip->posCurr.y;

	// Create 4 random asteroids
	NewAsteroidWave();

	// Testing Powerups
	for(int i = 0; i < 2; ++i)
		CreateRandPowerup();

	 //create the static wall
	AEVec2Set(&scale, WALL_SCALE_X, WALL_SCALE_Y);
	AEVec2 position;
	AEVec2Set(&position, 300.0f, 150.0f);
	spWall = gameObjInstCreate(TYPE_WALL, &scale, &position, nullptr, 0.0f);
	AE_ASSERT(spWall);

	// reset the score and the number of ships
	sScore      = 0;
	sShipLives  = SHIP_INITIAL_NUM;

	gameOver = false;
	onValueChange = true; // To reprint the console if player choose to restart
}

/******************************************************************************/
/*!
	"Update" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsUpdate(void)
{
	// =========================================================
	// update according to input
	// =========================================================
	spShip->pObject->pTexture = shipTexture;
	if (AEInputCheckCurr(AEVK_UP))
	{
		// Get the direction and calculate the velocity and apply it to the current velocity.
		AEVec2 dir;
		AEVec2Set(&dir, cosf(spShip->dirCurr), sinf(spShip->dirCurr));
		AEVec2Normalize(&dir, &dir);

		AEVec2Scale(&dir, &dir, SHIP_ACCEL_FORWARD * (float)(AEFrameRateControllerGetFrameTime()) * 0.99f);
		AEVec2Add(&spShip->velCurr, &spShip->velCurr, &dir);

		spShip->pObject->pTexture = shipFireTexture;
	}

	if (AEInputCheckCurr(AEVK_DOWN))
	{
		// Get the direction and calculate the velocity and apply it to the current velocity.
		// Apply speed negatively since its going backward
		AEVec2 dir;
		AEVec2Set(&dir, cosf(spShip->dirCurr), sinf(spShip->dirCurr));
		AEVec2Normalize(&dir, &dir);
		AEVec2Scale(&dir, &dir, -SHIP_ACCEL_BACKWARD * (float)(AEFrameRateControllerGetFrameTime()) * 0.99f);
		AEVec2Add(&spShip->velCurr, &spShip->velCurr, &dir);
		spShip->pObject->pTexture = shipFireTexture;
	}

	if (AEInputCheckCurr(AEVK_LEFT))
	{
		// Rotate the ship, wrap the angle
		spShip->dirCurr += SHIP_ROT_SPEED * (float)(AEFrameRateControllerGetFrameTime ());
		spShip->dirCurr =  AEWrap(spShip->dirCurr, -PI, PI);
	}

	if (AEInputCheckCurr(AEVK_RIGHT))
	{
		// Rotate the ship, wrap the angle
		spShip->dirCurr -= SHIP_ROT_SPEED * (float)(AEFrameRateControllerGetFrameTime ());
		spShip->dirCurr =  AEWrap(spShip->dirCurr, -PI, PI);
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
		pos.x = spShip->posCurr.x;	pos.y = spShip->posCurr.y;
		vel.x = BULLET_SPEED * cosf(spShip->dirCurr);
		vel.y = BULLET_SPEED * sinf(spShip->dirCurr);
		AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
		gameObjInstCreate(TYPE_BULLET, &scale, &pos, &vel, spShip->dirCurr);

		if (shipShotgun == 1)
		{
			float bulletOffset = 0.1f;
			{
				pos.x = spShip->posCurr.x;	pos.y = spShip->posCurr.y;
				vel.x = BULLET_SPEED * cosf(spShip->dirCurr + bulletOffset);
				vel.y = BULLET_SPEED * sinf(spShip->dirCurr + bulletOffset);
				AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
				gameObjInstCreate(TYPE_BULLET, &scale, &pos, &vel, spShip->dirCurr + bulletOffset);

				pos.x = spShip->posCurr.x;	pos.y = spShip->posCurr.y;
				vel.x = BULLET_SPEED * cosf(spShip->dirCurr + bulletOffset * 2);
				vel.y = BULLET_SPEED * sinf(spShip->dirCurr + bulletOffset * 2);
				AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
				gameObjInstCreate(TYPE_BULLET, &scale, &pos, &vel, spShip->dirCurr + bulletOffset);

				pos.x = spShip->posCurr.x;	pos.y = spShip->posCurr.y;
				vel.x = BULLET_SPEED * cosf(spShip->dirCurr - bulletOffset);
				vel.y = BULLET_SPEED * sinf(spShip->dirCurr - bulletOffset);
				AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
				gameObjInstCreate(TYPE_BULLET, &scale, &pos, &vel, spShip->dirCurr + bulletOffset);

				pos.x = spShip->posCurr.x;	pos.y = spShip->posCurr.y;
				vel.x = BULLET_SPEED * cosf(spShip->dirCurr - bulletOffset * 2);
				vel.y = BULLET_SPEED * sinf(spShip->dirCurr - bulletOffset * 2);
				AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
				gameObjInstCreate(TYPE_BULLET, &scale, &pos, &vel, spShip->dirCurr + bulletOffset);
			}
		}
	}

	// Check wave timer
	waveTimer += AEFrameRateControllerGetFrameTime();
	if (waveTimer >= WAVE_TIME)
	{
		// Spawn a new wave if it reaches wave time
		NewAsteroidWave();
		waveTimer = 0.0f;
	}


	// ======================================================================
	// Save previous positions
	//  -- For all instances
	// [DO NOT UPDATE THIS PARAGRAPH'S CODE]
	// ======================================================================
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		// Update all the GO Instances's prev positions here
		// Before doing any update in collision and movement
		GameObjInst* pInst = sGameObjInstList + i;

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
	Helper_Wall_Collision();

	// ======================================================================
	// check for dynamic-dynamic collisions
	// ======================================================================
	// Loop through GO lists and check collisions between objects
	CheckGOCollision();

	// Update the GOs (i.e movement, etc)
	UpdateGO();

	// Update powerups timers here
	UpdatePowerups();
		
	// =====================================================================
	// calculate the matrix for all objects
	// =====================================================================

	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst * pInst = sGameObjInstList + i;
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
		GameObjInst * pInst = sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;
		
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
	for (int i = 0; i < 7; ++i)
	{
		// Specific checks so  certain text dont render until required
		if (!gameOver && (i == 2 || i == 3 || i == 6)) continue;
		if (shipShield == 0 && i == 4) continue;
		if (shipShotgun == 0 && i == 5) continue;
		
		if (gameOver)
		{
			if (sScore >= 5000)
			{
				textList[2].str = "GAME OVER! YOU WON";

			}
			else if (sShipLives <= 0)
			{
				textList[2].str = "GAME OVER! YOU LOST";
			}
		}

		// 0 and 1 is for score and lives
		// 2 3 and 6 is for game over texts
		// 4 and 5 is for power up shield and shotgun
		// They need to pass in the values when rendering
		if (i == 0)
			snprintf(strBuffer, sizeof(strBuffer), "%s %d", textList[i].str, sScore);
		else if (i == 1)
			snprintf(strBuffer, sizeof(strBuffer), "%s %d", textList[i].str, sShipLives >= 0 ? sShipLives : 0);
		else
			snprintf(strBuffer, sizeof(strBuffer), "%s", textList[i].str);

		// Render the text object after assigning the string val
		RenderText(&textList[i], fontSize, strBuffer);
	}


	// Updates the console print of the score and lives when value is changed
	if(onValueChange)
	{
		sprintf_s(strBuffer, "Score: %d", sScore);
		//AEGfxPrint(10, 10, (u32)-1, strBuffer);
		printf("%s \n", strBuffer);

		sprintf_s(strBuffer, "Ship Left: %d", sShipLives >= 0 ? sShipLives : 0);
		//AEGfxPrint(600, 10, (u32)-1, strBuffer);
		printf("%s \n", strBuffer);

		// display the game over message
		if (sShipLives <= 0)
		{
			//AEGfxPrint(280, 260, 0xFFFFFFFF, "       GAME OVER       ");
			printf("       GAME OVER       \n");
		}

		// Set to false so that it doesn't flood the console with print messages
		onValueChange = false;
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
		GameObjInst* pInst = sGameObjInstList + i;

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
		if (sGameObjList[i].pMesh != nullptr)
		{
			// Free the mesh
			AEGfxMeshFree(sGameObjList[i].pMesh);
		}

		// Check if there is any texture currently pointing to
		if (sGameObjList[i].pTexture != nullptr)
		{
			// Dont delete the texture here as it is a shared texture
			sGameObjList[i].pTexture = nullptr;
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
	Create a game object
*/
/******************************************************************************/
GameObjInst * gameObjInstCreate(unsigned long type, 
							   AEVec2 * scale,
							   AEVec2 * pPos, 
							   AEVec2 * pVel, 
							   float dir)
{
	AEVec2 zero;
	AEVec2Zero(&zero);

	AE_ASSERT_PARM(type < sGameObjNum);
	
	// loop through the object instance list to find a non-used object instance
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst * pInst = sGameObjInstList + i;

		// check if current instance is not used
		if (pInst->flag == 0)
		{
			// it is not used => use it to create the new instance
			pInst->pObject	= sGameObjList + type;
			
			pInst->flag		= FLAG_ACTIVE;
			pInst->scale	= *scale;
			pInst->posCurr	= pPos ? *pPos : zero;
			pInst->posPrev = pInst->posCurr;
			pInst->velCurr	= pVel ? *pVel : zero;
			pInst->dirCurr	= dir;
			
			// return the newly created instance
			return pInst;
		}
	}

	// cannot find empty slot => return 0
	return 0;
}

/******************************************************************************/
/*!
	Destroy game object instance
*/
/******************************************************************************/
void gameObjInstDestroy(GameObjInst * pInst)
{
	// if instance is destroyed before, just return
	if (pInst->flag == 0)
		return;

	// zero out the flag
	pInst->flag = 0;
}

/******************************************************************************/
/*!
    check for collision between Ship and Wall and apply physics response on the Ship
		-- Apply collision response only on the "Ship" as we consider the "Wall" object is always stationary
		-- We'll check collision only when the ship is moving towards the wall!
	[DO NOT UPDATE THIS PARAGRAPH'S CODE]
*/
/******************************************************************************/
void Helper_Wall_Collision()
{
	//calculate the vectors between the previous position of the ship and the boundary of wall
	AEVec2 vec1;
	vec1.x = spShip->posPrev.x - spWall->boundingBox.min.x;
	vec1.y = spShip->posPrev.y - spWall->boundingBox.min.y;
	AEVec2 vec2;
	vec2.x = 0.0f;
	vec2.y = -1.0f;
	AEVec2 vec3;
	vec3.x = spShip->posPrev.x - spWall->boundingBox.max.x;
	vec3.y = spShip->posPrev.y - spWall->boundingBox.max.y;
	AEVec2 vec4;
	vec4.x = 1.0f;
	vec4.y = 0.0f;
	AEVec2 vec5;
	vec5.x = spShip->posPrev.x - spWall->boundingBox.max.x;
	vec5.y = spShip->posPrev.y - spWall->boundingBox.max.y;
	AEVec2 vec6;
	vec6.x = 0.0f;
	vec6.y = 1.0f;
	AEVec2 vec7;
	vec7.x = spShip->posPrev.x - spWall->boundingBox.min.x;
	vec7.y = spShip->posPrev.y - spWall->boundingBox.min.y;
	AEVec2 vec8;
	vec8.x = -1.0f;
	vec8.y = 0.0f;
	if (
		(AEVec2DotProduct(&vec1, &vec2) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec2) <= 0.0f) ||
		(AEVec2DotProduct(&vec3, &vec4) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec4) <= 0.0f) ||
		(AEVec2DotProduct(&vec5, &vec6) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec6) <= 0.0f) ||
		(AEVec2DotProduct(&vec7, &vec8) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec8) <= 0.0f)
		)
	{
		float firstTimeOfCollision = 0.0f;
		if (CollisionIntersection_RectRect(spShip->boundingBox,
			spShip->velCurr,
			spWall->boundingBox,
			spWall->velCurr,
			firstTimeOfCollision))
		{
			//re-calculating the new position based on the collision's intersection time
			spShip->posCurr.x = spShip->velCurr.x * (float)firstTimeOfCollision + spShip->posPrev.x;
			spShip->posCurr.y = spShip->velCurr.y * (float)firstTimeOfCollision + spShip->posPrev.y;

			//reset ship velocity
			spShip->velCurr.x = 0.0f;
			spShip->velCurr.y = 0.0f;
		}
	}
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
		GameObjInst* pInst = sGameObjInstList + i;

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
		case TYPE_POWERUP:
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
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst* pInst_1 = sGameObjInstList + i;

		// skip non-active object
		if ((pInst_1->flag & FLAG_ACTIVE) == 0)
			continue;

		// If its an asteroid, then start checking for collision against ship or bullet
		if (pInst_1->pObject->type == TYPE_ASTEROID)
		{
			for (unsigned long j = 0; j <  GAME_OBJ_INST_NUM_MAX; j++)
			{
				GameObjInst* pInst_2 = sGameObjInstList + j;
				if ((pInst_2->flag & FLAG_ACTIVE) == 0) continue;
				if ((pInst_2->pObject->type == TYPE_ASTEROID)) continue;
				float firstTimeOfCollision = 0;

				switch (pInst_2->pObject->type)
				{
				case TYPE_BULLET:
					if (CollisionIntersection_RectRect(pInst_1->boundingBox, pInst_1->velCurr, pInst_2->boundingBox, pInst_2->velCurr, firstTimeOfCollision))
					{
						if (pInst_1->scale.x >= (ASTEROID_MIN_SCALE_X * 2.0f) && pInst_1->scale.y >= (ASTEROID_MIN_SCALE_Y * 2.0f))
						{
							// This part here is causing my ship to take damage when i destroy asteroids 
							for (unsigned long k = 0; k < 3; ++k)
							{
								AEVec2 pos, vel, scale;
								AEVec2Set(&pos, pInst_1->posCurr.x, pInst_1->posCurr.y);
								float randAng = AERandFloat() * 360;

								AEVec2 dir;
								AEVec2Set(&dir, cosf(randAng), sinf(randAng));
								AEVec2Normalize(&dir, &dir);
								AEVec2Zero(&vel);

								AEVec2Scale(&dir, &dir, ASTEROID_ACCEL);
								AEVec2Add(&vel, &vel, &dir);

								// Create more asteroids randomised from the size of the asteroid destroyed to min size of an asteroid
								float randScaleX = AERandFloat() * (pInst_1->scale.x - ASTEROID_MIN_SCALE_X) + ASTEROID_MIN_SCALE_X;
								float randScaleY = AERandFloat() * (pInst_1->scale.y - ASTEROID_MIN_SCALE_Y) + ASTEROID_MIN_SCALE_Y;

								AEVec2Set(&scale, randScaleX, randScaleY);
								CreateAsteroid(pos, vel, scale);
							}

							//  Update the hitboxes because new asteroids were made
							UpdateGOCollisionBoxes();
						}

						// Destroy both the bullet and asteroid after
						gameObjInstDestroy(pInst_1);
						gameObjInstDestroy(pInst_2);

						if (!gameOver)
						{
							// increment score
							sScore += 100;
							onValueChange = true;
							if (sScore >= 5000)
								gameOver = true;
						}
					}
					break;

				case TYPE_SHIP:
					// If its colliding with the ship
					if (CollisionIntersection_RectRect(pInst_1->boundingBox, pInst_1->velCurr, pInst_2->boundingBox, pInst_2->velCurr, firstTimeOfCollision))
					{
						// Use up the shield power first
						if (shipShield == 1)
						{
							// Dont reduce health
							shipShield = 0;
							ResetShip();
							break;
						}

						// Destroy the asteroid and update the ship live
						gameObjInstDestroy(pInst_1);
						if (!gameOver)
						{
							sShipLives--;
							onValueChange = true;
							if (sShipLives <= 0)
								gameOver = true;
						}

						ResetShip();
					}
					break;
				}
			}
		}
		if (pInst_1->pObject->type == TYPE_POWERUP)
		{
			for (unsigned long j = 0; j < GAME_OBJ_INST_NUM_MAX; j++)
			{
				GameObjInst* pInst_2 = sGameObjInstList + j;
				// Power up ignores other power up, asteroids and bullets
				// Only collideable with the ship
				if ((pInst_2->flag & FLAG_ACTIVE) == 0) continue;
				if ((pInst_2->pObject->type == TYPE_POWERUP)) continue;
				if ((pInst_2->pObject->type == TYPE_ASTEROID)) continue;
				if ((pInst_2->pObject->type == TYPE_BULLET)) continue;
				float firstTimeOfCollision = 0;

				if (pInst_2->pObject->type == TYPE_SHIP)
				{
					if (CollisionIntersection_RectRect(pInst_1->boundingBox, pInst_1->velCurr, pInst_2->boundingBox, pInst_2->velCurr, firstTimeOfCollision))
					{
						// Randomise which power up to get when picked up
						int randNum = rand() % (POWERUP_TYPE_NUM);
						// For power up collision
						switch (randNum)
						{
						case POWERUP_TYPE_SHIELD:
							shipShield = 1;
							shieldTimer = POWERUP_TIME;
							break;
						case POWERUP_TYPE_SHOTGUN:
							shipShotgun = 1;
							shotgunTimer = POWERUP_TIME;
							break;
						}

						// Destroy power up aft touching
						gameObjInstDestroy(pInst_1);
					}
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
		GameObjInst* pInst = sGameObjInstList + i;

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
void CreateAsteroid(AEVec2 _pos, AEVec2 _vel, AEVec2 _scale)
{
	gameObjInstCreate(TYPE_ASTEROID, &_scale, &_pos, &_vel, 0.0f);

}

/// <summary>
/// Spawn the wave of asteroid and power up
/// </summary>
void NewAsteroidWave()
{
	// Create 4 asteroids every wave

	// Spawn from bottom of screen
	RandomiseAsteroid(AEGfxGetWinMinX(), AEGfxGetWinMaxX(), AEGfxGetWinMinY(), AEGfxGetWinMinY());
	// Spawn from top of screen
	RandomiseAsteroid(AEGfxGetWinMinX(), AEGfxGetWinMaxX(), AEGfxGetWinMaxY(), AEGfxGetWinMaxY());
	//// Spawn from Left of screen
	RandomiseAsteroid(AEGfxGetWinMinX(), AEGfxGetWinMinX(), AEGfxGetWinMinY(), AEGfxGetWinMaxY());
	//// Spawn from Right of screen
	RandomiseAsteroid(AEGfxGetWinMaxX(), AEGfxGetWinMaxX(), AEGfxGetWinMinY(), AEGfxGetWinMaxY());

	// Create a power up when a wave spawns
	CreateRandPowerup();
}

/// <summary>
/// Create a randomised asteroid within a fixed boundary
/// </summary>
/// <param name="min_xPos">The minimum AABB x pos</param>
/// <param name="max_xPos">The maximum AABB x pos</param>
/// <param name="min_yPos">The minimum AABB y</param>
/// <param name="max_yPos">The maximum AABB y</param>
void RandomiseAsteroid(f32 min_xPos, f32 max_xPos, f32 min_yPos, f32 max_yPos)
{
	AEVec2 pos, vel, scale;
	pos.x = AERandFloat() * (max_xPos - min_xPos) + min_xPos;
	pos.y = AERandFloat() * (max_yPos - min_yPos) + min_yPos;
	float randAng = AERandFloat() * 360;

	AEVec2 dir;
	AEVec2Set(&dir, cosf(randAng), sinf(randAng));
	AEVec2Normalize(&dir, &dir);
	AEVec2Zero(&vel);

	AEVec2Scale(&dir, &dir, ASTEROID_ACCEL);
	AEVec2Add(&vel, &vel, &dir);

	float randScaleX = AERandFloat() * (ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X) + ASTEROID_MIN_SCALE_X;
	float randScaleY = AERandFloat() * (ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y) + ASTEROID_MIN_SCALE_Y;
	AEVec2Set(&scale, randScaleX, randScaleY);

	// Create after randomising
	CreateAsteroid(pos, vel, scale);
}

/// <summary>
/// Create a random powerup in either top, bottom, left or right side of the screen
/// </summary>
void CreateRandPowerup()
{
	AEVec2 pos, vel, scale;
	int randNum = rand() % 4;
	switch (randNum)
	{
	case 0:
		// Bottom of the screen
		pos.x = AERandFloat() * (AEGfxGetWinMaxX() - AEGfxGetWinMinX()) + AEGfxGetWinMinX();
		pos.y = AERandFloat() * (AEGfxGetWinMinY() - AEGfxGetWinMinY()) + AEGfxGetWinMinY();
		break;
	case 1:
		// Top of the screen
		pos.x = AERandFloat() * (AEGfxGetWinMaxX() - AEGfxGetWinMinX()) + AEGfxGetWinMinX();
		pos.y = AERandFloat() * (AEGfxGetWinMaxY() - AEGfxGetWinMaxY()) + AEGfxGetWinMaxY();
		break;
	case 2:
		// Left side of the screen
		pos.x = AERandFloat() * (AEGfxGetWinMinX() - AEGfxGetWinMinX()) + AEGfxGetWinMinX();
		pos.y = AERandFloat() * (AEGfxGetWinMaxY() - AEGfxGetWinMinY()) + AEGfxGetWinMinY();
		break;
	case 3:
		// Right side of the screen
		pos.x = AERandFloat() * (AEGfxGetWinMaxX() - AEGfxGetWinMaxX()) + AEGfxGetWinMaxX();
		pos.y = AERandFloat() * (AEGfxGetWinMaxY() - AEGfxGetWinMinY()) + AEGfxGetWinMinY();
		break;
	}
	float randAng = AERandFloat() * 360;

	AEVec2 dir;
	AEVec2Set(&dir, cosf(randAng), sinf(randAng));
	AEVec2Normalize(&dir, &dir);
	AEVec2Zero(&vel);

	AEVec2Scale(&dir, &dir, POWERUP_ACCEL);
	AEVec2Add(&vel, &vel, &dir);
	AEVec2Set(&scale, POWERUP_SCALE, POWERUP_SCALE);

	gameObjInstCreate(TYPE_POWERUP, &scale, &pos, &vel, randAng);
}

/// <summary>
/// Reset the ship when it gets hit by an asteroid
/// </summary>
void ResetShip()
{
	AEVec2Set(&spShip->posCurr, 0, 0);
	AEVec2Zero(&spShip->velCurr);
	spShip->dirCurr = 0.0f;
}

/// <summary>
/// Update the power up timers so they can turn off after a set amount of time
/// </summary>
void UpdatePowerups()
{
	f64 dt = AEFrameRateControllerGetFrameTime();

	if (shipShield == 1) // If active
	{
		//printf("%f\n", shieldTimer);
		shieldTimer -= dt;
		if (shieldTimer <= 0)
		{
			// Its over
			shipShield = 0;
		}
	}

	if (shipShotgun == 1)
	{
		shotgunTimer -= dt;
		if (shotgunTimer <= 0)
		{
			shipShotgun = 0;
		}
	}
}

/// <summary>
/// Renders a mesh object with a texture
/// </summary>
/// <param name="GO">The game object to render</param>
void RenderMeshObj(GameObjInst* GO)
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
