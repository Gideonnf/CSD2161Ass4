/******************************************************************************/
/*!
\file		Entity.cpp
\author 	Gideon Francis, g.francis, 2301207
\par    	email: g.francis@digipen.edu
\date   	February 09 2024
\brief	Entity cpp file. Holds function definitions to help render certain spcific objects.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/


#include "Main.h"


void RenderObj(f64 dt, FadeObject* obj)
{
	// Check if the object has to fade or not
	if (obj->toFade)
	{
		//std::cout << alpha << std::endl;
		// Lerp the alpha val
		obj->alpha = lerp(0.0f, 1.0f, ((f32)obj->fadeTimer / (f32)obj->fadeDuration));
		obj->fadeTimer += dt;
		// When its done, stop fading and reset some values
		if (obj->fadeTimer >= obj->fadeDuration)
		{
			obj->alpha = 1.0f;
			obj->toFade = false;
			obj->fadeTimer = 0.0f;
		}
	}
	// Fading into the scene
	else if (!obj->toFade && obj->alpha > 0)
	{
		// find out why this doesnt actually update alpha value
		obj->alpha = lerp(1.0f, 0.0f, ((f32)obj->fadeTimer / (f32)obj->fadeDuration));
		obj->fadeTimer += dt;
		if (obj->fadeTimer >= obj->fadeDuration)
		{
			obj->alpha = 0.0f;
			obj->fadeTimer = 0.0f;
		}
	}


	if (obj->mesh == nullptr) return;

	// Tell the engine rendering without textures
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	AEMtx33 scale{ 0 }, rotate{ 0 }, translate{ 0 }, transform{ 0 };

	AEMtx33Scale(&scale, obj->goScale.x, obj->goScale.y);
	AEMtx33Rot(&rotate, 0);
	AEMtx33Trans(&translate, 0, 0);

	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);
	AEGfxSetTransform(transform.m);
	//AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
	//AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 1.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(0.0f);

	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, obj->alpha);
	AEGfxMeshDraw(obj->mesh, AE_GFX_MDM_TRIANGLES);

	AEGfxSetTransparency(1.0f);

	AEGfxSetColorToAdd(0.f, 0.f, 0.f, 1.f);

}

void RenderMeshObj(BGObject* GO)
{
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	AEMtx33 scale{ 0 };
	AEMtx33 rotate{ 0 }; 
	AEMtx33 translate{ 0 };
	AEMtx33 transform{ 0 };

	AEMtx33Scale(&scale, GO->goScale.x, GO->goScale.y);
	AEMtx33Rot(&rotate, 0);
	AEMtx33Trans(&translate, GO->goPosition.x, GO->goPosition.y);
	AEMtx33Concat(&transform, &rotate, &scale);
	AEMtx33Concat(&transform, &translate, &transform);
	AEGfxTextureSet(GO->meshTex, 0, 0);

	AEGfxSetTransform(transform.m);

	AEGfxMeshDraw(GO->mesh, AE_GFX_MDM_TRIANGLES);
}


void RenderText(ButtonObj button, s8 fontSize)
{
	f32 textWidth, textHeight;
	AEVec2 textPos;
	AEVec2Zero(&textPos);
	AEGfxGetPrintSize(*button.Font, button.str, 1.f, &textWidth, &textHeight);
	AEVec2Set(&textPos, button.pos.x, button.pos.y - fontSize * 0.5f);
	textPos = GetUICoordinates(textPos);
	AEVec2Set(&textPos, textPos.x - (textWidth * 0.5f), textPos.y);
	AEGfxPrint(*button.Font, button.str, textPos.x, textPos.y, 1.f, 1.f, 1.f, 1.f, 1.f);
}

void RenderText(TextObj* GO, s8 _fontSize, char* str)
{
	f32 textWidth, textHeight;
	f32 textSize = (f32)GO->textSize /(f32) _fontSize;
	AEVec2 textPos;
	AEVec2Zero(&textPos);
	AEGfxGetPrintSize(*GO->Font, str, textSize, &textWidth, &textHeight);
	//textPos = { GO->pos.x,
	//		GO->pos.y - textSize * 0.5f };
	AEVec2Set(&textPos, GO->pos.x, GO->pos.y - textSize * 0.5f);
	textPos = GetUICoordinates(textPos);
	AEVec2Set(&textPos, textPos.x - (textWidth * 0.5f), textPos.y);

	//textPos = {  };
	AEGfxPrint(*GO->Font, str, textPos.x, textPos.y, textSize, 1.f, 1.f, 1.f, 1.f);
}