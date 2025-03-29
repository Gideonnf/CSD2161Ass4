/******************************************************************************/
/*!
\file		Collision.h
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	Collision file. Holds the function declaration of CollisionIntersection_RectRect.
This function checks for dynamic and static collision between two AABB boxes

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#ifndef CSD1130_COLLISION_H_
#define CSD1130_COLLISION_H_

#include "AEEngine.h"

/**************************************************************************/
/*!

	*/
/**************************************************************************/
struct AABB
{
	AEVec2	min;
	AEVec2	max;
};

/*!*************************************************************************
\brief Checks collision intersection between two rectangle AABB boxes. 
	

\param[in] aabb1 - First AABB to check
	

\param[in] vel1 - First Vel to check
	

\param[in] aabb2 - Second AABB t ocheck
	

\param[in] vel2 - Second Vel to check
	

\param[in] firstTimeOfCollision - Float of the time of collision. Holds the time of collision if it happens
	

\return true - If there was a collision
	
\return false - If there was no collision
	
***************************************************************************/
bool CollisionIntersection_RectRect(const AABB& aabb1,            //Input
									const AEVec2& vel1,           //Input 
									const AABB& aabb2,            //Input 
									const AEVec2& vel2,           //Input
									float& firstTimeOfCollision); //Output: the calculated value of tFirst, must be returned here


#endif // CSD1130_COLLISION_H_