/******************************************************************************/
/*!
\file		Collision.cpp
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	Collision file. Holds the function definition of CollisionIntersection_RectRect.
This function checks for dynamic and static collision between two AABB boxes

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/


#include "main.h"

bool CollisionIntersection_RectRect(const AABB & aabb1,          //Input
									const AEVec2 & vel1,         //Input 
									const AABB & aabb2,          //Input 
									const AEVec2 & vel2,         //Input
									float& firstTimeOfCollision) //Output: the calculated value of tFirst, below, must be returned here
{
	UNREFERENCED_PARAMETER(aabb1);
	UNREFERENCED_PARAMETER(vel1);
	UNREFERENCED_PARAMETER(aabb2);
	UNREFERENCED_PARAMETER(vel2);
	UNREFERENCED_PARAMETER(firstTimeOfCollision);

	if (aabb1.max.x > aabb2.min.x && aabb1.min.x < aabb2.max.x)
	{
		if (aabb1.max.y > aabb2.min.y && aabb1.min.y < aabb2.max.y)
		{
			return true;
		}
	}

	// If no static collision then check dynamic
	AEVec2 vB;
	float dFirst = 0;
	float dLast = 0;
	AABB _aabb1 = aabb1;
	AABB _aabb2 = aabb2;
	AEVec2 _vel1 = vel1;
	AEVec2 _vel2 = vel2;

	AEVec2Sub(&vB, &_vel2, &_vel1);

	float tFirst = 0;
	float tLast = (float)AEFrameRateControllerGetFrameTime();
	// aabb 2 is the one moving

	if (vB.x < 0)
	{
		// aabb1 is to the right of aabb2, and aabb2 is going in the other direction
		if (aabb1.min.x > aabb2.max.x)
		{
			// case 1
			return false;
		}
		// aabb1 is to the left of aabb2, and aabb2 is going in that direction
		else if (aabb1.max.x < aabb2.min.x)
		{
			// case 4 part 1
			// if dfirst is more, then tfirst = dlast, if not then tfirst = tfirst
			dFirst = (aabb1.max.x - aabb2.min.x) / vB.x;
			tFirst = max(dFirst, tFirst);
			
		}
		else if (aabb1.min.x < aabb2.max.x)
		{
			// Repeat the same thing as tfirst for tlast
			dLast = (aabb1.min.x - aabb2.max.x) / vB.x;
			tLast = min(dLast, tLast);

		}
	}
	else if (vB.x > 0)
	{
		// aabb2 is moving to the right, but aabb1 is on the left side so its safe
		if (aabb1.max.x < aabb2.min.x)
		{
			// case 3
			return false;
		}
		// aabb2 is moving to the right, and aabb1 is on the right side so its in the direction
		if (aabb1.min.x > aabb2.max.x)
		{
			 // case 2 part 1
			dFirst = (aabb1.min.x - aabb2.max.x) / vB.x;
			tFirst = max(dFirst, tFirst);
		}

		if (aabb1.max.x > aabb2.min.x)
		{
			// case 2 part 2
			dLast = (aabb1.max.x - aabb2.min.x) / vB.x;
			tLast = min(dLast, tLast);
		}
	}
	// vB == 0
	else
	{
		if (aabb1.max.x < aabb2.min.x)
			return false;
		else if (aabb1.min.x > aabb2.max.x)
			return false;
	}

	if (vB.y < 0)
	{
		// aabb1 is to the right of aabb2, and aabb2 is going in the other direction
		if (aabb1.min.y > aabb2.max.y)
		{
			// case 1
			return false;
		}
		// aabb1 is to the left of aabb2, and aabb2 is going in that direction
		else if (aabb1.max.y < aabb2.min.y)
		{
			// case 4 part 1
			// if dfirst is more, then tfirst = dlast, if not then tfirst = tfirst
			dFirst = (aabb1.max.y - aabb2.min.y) / vB.y;
			tFirst = max(dFirst, tFirst);

		}
		else if (aabb1.min.y < aabb2.max.y)
		{
			// Repeat the same thing as tfirst for tlast
			dLast = (aabb1.min.y - aabb2.max.y) / vB.y;
			tLast = min(dLast, tLast);

		}
	}
	else if (vB.y > 0)
	{
		// aabb2 is moving to the right, but aabb1 is on the left side so its safe
		if (aabb1.max.y < aabb2.min.y)
		{
			// case 3
			return false;
		}
		// aabb2 is moving to the right, and aabb1 is on the right side so its in the direction
		if (aabb1.min.y > aabb2.max.y)
		{
			// case 2 part 1
			dFirst = (aabb1.min.y - aabb2.max.y) / vB.y;
			tFirst = max(dFirst, tFirst);
		}

		if (aabb1.max.y > aabb2.min.y)
		{
			// case 2 part 2
			dLast = (aabb1.max.y - aabb2.min.y) / vB.y;
			tLast = min(dLast, tLast);
		}
	}
	// vB == 0
	else
	{
		if (aabb1.max.y < aabb2.min.y)
			return false;
		else if (aabb1.min.y > aabb2.max.y)
			return false;
	}


	if (tFirst > tLast)
		return false;

	firstTimeOfCollision = tFirst;

	return true;
}