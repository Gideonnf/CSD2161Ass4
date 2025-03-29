/******************************************************************************/
/*!
\file		utils.cpp
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   February 09 2024
\brief	Holds the function definitons for util.h. Function handles getting UI Coordinates
and lerping 

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "Main.h"

// Function commenting in header files

float lerp(float start_val, float end_val, float t)
{
	return start_val + (end_val - start_val) * t;
}

AEVec2 GetUICoordinates(AEVec2 worldPos)
{
	// Convert world positions to UI coordinates
	AEVec2 uiPos;
	AEVec2Zero(&uiPos);

	uiPos.x = worldPos.x / ((AEGfxGetWindowWidth() * 0.5f));
	uiPos.y = worldPos.y / ((AEGfxGetWindowHeight() * 0.5f));

	return uiPos;
}

