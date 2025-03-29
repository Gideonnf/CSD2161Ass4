/******************************************************************************/
/*!
\file		utils.h
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   February 09 2024
\brief	Holds the function declarations for util.cpp. Function handles getting UI Coordinates
and lerping

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "Main.h"

/*!*************************************************************************
\brief Lerps a foat value between two floats (start_val and end_val) over time
	

\param[in] start_val - The starting value to float from
	

\param[in] end_val - The ending value when it reaches the end time
	

\param[in] t - the time variable 
	

\return float the lerped float values
	
***************************************************************************/
float lerp(float start_val, float end_val, float t);

AEVec2 GetUICoordinates(AEVec2 worldPos);
