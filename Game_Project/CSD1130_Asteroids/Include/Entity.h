/******************************************************************************/
/*!
\file		Entity.h
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	Entity header file. Holds function declarations to help render certain spcific objects.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#ifndef ENTITY_H
#define ENTITY_H

#include "AEEngine.h"
#include "AEMath.h"
#include "Collision.h"

struct BGObject
{
	AEGfxVertexList* mesh;
	AEGfxTexture* meshTex;
	AEVec2 goScale;
	AEVec2 goPosition;
};

struct ButtonObj
{
	char* str;
	AEVec2 scale;
	AEVec2 pos;
	s8* Font;
	f64 delay;
	f64 timer;
	int bounce;
};

struct TextObj
{
	char* str;
	s8 textSize;
	AEVec2 pos;
	s8* Font;
};


struct FadeObject
{
	AEGfxVertexList* mesh;
	AEVec2 goScale;
	AEVec2 goPosition;
	bool toFade;
	float alpha;
	f64 fadeDuration;
	f64 fadeTimer;
	bool goActive;
};

/*!

	Data for game

*/
const unsigned int	GAME_OBJ_NUM_MAX = 32;			// The total number of different objects (Shapes)
const unsigned int	GAME_OBJ_INST_NUM_MAX = 2048;			// The total number of different game object instances
const unsigned int  BULLET_ID_MAX = 100;

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
	bool active = false;

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
	GameObjInst* spShip[4];										// Pointer to the "Ship" game object instance

	uint32_t bulletIDCount{0};

	// pointer to the wall object
	//static GameObjInst *		spWall;										// Pointer to the "Wall" game object instance

	// the score = number of asteroid destroyed
	uint32_t		sScore;										// Current score

	bool onValueChange = true;

	TextObj scoreText;
	TextObj endText;
	TextObj endText2;

	TextObj textList[4];
	
	int currID{};
};


/*!*************************************************************************
\brief Renders the fade object which is only used for the fading effect
	

\param[in] dt - the delta time value or time elapsed between frames
	

\param[in] obj - the object to fade
	
***************************************************************************/
void RenderObj(f64 dt, FadeObject* obj);

/*!*************************************************************************
\brief Renders the BG mesh object for scenes. A simplified GameObjectInstance.
	

\param[in] GO - the BG Object to render
	

***************************************************************************/
void RenderMeshObj(BGObject* GO);
/*!*************************************************************************
\brief Renders text objects for game UI. Used in game scenes
	

\param[in] GO - The text object to render
	

\param[in] fontSize - The size of the font
	

\param[in] str - The string to render
	
***************************************************************************/
void RenderText(TextObj* GO, s8 fontSize, char* str);

/*!*************************************************************************
\brief Renders a button object for game menu screens. Used in main menu screen
	

\param[in] button - A button obj 
	

\param[in] fontSize - The font size
	
***************************************************************************/
void RenderText(ButtonObj button, s8 fontSize);

#endif