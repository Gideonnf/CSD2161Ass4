/******************************************************************************/
/*!
\file		GameState_Menu.cpp
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	The menu game state file. Holds the 6 main state functions. This file will 
Update input in the main menu and render any objects needed for the main menu

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/


#include "main.h"

namespace
{
	const unsigned int MAX_BUTTON = 20;

	ButtonObj menuButtons[MAX_BUTTON];
	//AEGfxTexture* bgTex;
	BGObject bgGO;

	s8 textFont;
	s8 fontSize;
}

void GameStateMenuLoad()
{
	fontSize = 36;
	textFont = AEGfxCreateFont("../Resources/Fonts/Arial-Italic.ttf", fontSize);
	for (int i = 0; i < MAX_BUTTON; ++i)
	{
		menuButtons[i].Font = &textFont;
	}

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

	
}

void GameStateMenuInit()
{
	AEVec2Set(&bgGO.goScale, 1280, 720);
	AEVec2Set(&bgGO.goPosition, 0, 0);

	char text[MAX_BUTTON] = "PRESS SPACE TO PLAY";
	//menuButtons[0].str = "PRESS SPACE TO PLAY";
	for (int i = 0; i < MAX_BUTTON; ++i)
	{
		char temp[2] = "";
		temp[0] = text[i];
		menuButtons[i].str = (char*)malloc(sizeof(char*) * 2);
		//menuButtons[i].str = "";
		if (menuButtons[i].str != NULL)
			strcpy_s(menuButtons[i].str, 2, temp);

		//menuButtons[i].str = string[i];
		AEVec2Set(&menuButtons[i].scale, 1280, 720);
		AEVec2Set(&menuButtons[i].pos, -200 + (i * 25.0f), 0);
		//menuButtons[i].button = nullptr; // No button needed
		menuButtons[i].delay = i * 0.1f;
		menuButtons[i].timer = 0.0f;
		menuButtons[i].bounce = 0;
	}
	
}

void GameStateMenuUpdate()
{
	f64 dt = AEFrameRateControllerGetFrameTime();

	if (AEInputCheckTriggered(AEVK_SPACE))
	{
		gGameStateNext = GS_ASTEROIDS;
	}

	for (int i = 0; i < MAX_BUTTON; ++i)
	{
		// skip if its delay isnt 0 yet
		if (menuButtons[i].delay > 0)
		{
			menuButtons[i].delay -= dt;
			continue;
		}

		menuButtons[i].timer += dt;
		//printf("%f \n", sinf(2.0f * ((float)menuButtons[i].timer / 2.0f)));
		menuButtons[i].pos.y = 30.0f * sinf(2.0f * ((float)menuButtons[i].timer / 2.0f));
	}
	//AEVec2Lerp()
}

void GameStateMenuDraw()
{
	// Set the background to this color
	//AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

	RenderMeshObj(&bgGO);


	for (int i = 0; i < MAX_BUTTON; ++i)
	{
		RenderText(menuButtons[i], fontSize);
	}


}

void GameStateMenuFree()
{
	for (int i = 0; i < MAX_BUTTON; ++i)
	{
		if (menuButtons[i].str == NULL) continue;

	// VS Studio will keep flagging this as a warning
	// Even after i check for null
#pragma warning(push)
#pragma warning(disable: 6001)
			free(menuButtons[i].str);
#pragma warning(pop)
	}
}

void GameStateMenuUnload()
{
	for (int i = 0; i < MAX_BUTTON; ++i)
	{
		menuButtons[i].Font = NULL;
	}

	AEGfxDestroyFont(textFont);
	AEGfxMeshFree(bgGO.mesh);
	AEGfxTextureUnload(bgGO.meshTex);
}

void GameStateMenuProcessMessage()
{

}

