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


#include "Main.h"


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