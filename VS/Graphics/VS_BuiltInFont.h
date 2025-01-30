/*
 *  VS_BuiltInFont.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15/03/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_BUILTINFONT_H
#define VS_BUILTINFONT_H

#include "VS/Graphics/VS_Font.h"

class vsBuiltInFont
{
	static void AppendCharacterToList( char c, vsDisplayList *list, const vsVector2D &offset, float size );
	static vsDisplayList * CreateString_Internal(const char* string, float size, float capSize, JustificationType j, float maxWidth = -1.f);

	static void		BuildDisplayListFromString(vsDisplayList * list, const char* string, float size, float capSize, JustificationType j, const vsVector2D &offset = vsVector2D::Zero);
	static void		BuildDisplayListFromCharacter( vsDisplayList *list, char c, float size, float capSize);

	static void		WrapLine(const vsString &string, float size, float capSize, JustificationType j, float maxWidth);

public:

	static void				Init();	// called internally
	static void				Deinit();	// called internally

	static void				CreateStringInDisplayList(vsDisplayList *list, const vsString &string, float size, float capSize=-1.f, JustificationType j = Justification_Left, float maxWidth = -1.f);

	static vsDisplayList *	CreateString(const vsString &string, float size, float capSize=-1.f, JustificationType j = Justification_Left, float maxWidth = -1.f);
	static vsDisplayList *	CreateCharacter(char letter, float size, float capSize=-1.f);

	static float			GetCharacterWidth(char letter, float size, float capSize=-1.f);
	static float			GetStringWidth(const vsString &string, float size, float capSize=-1.f);

	static float			GetKerningForSize(float size);
};

#endif // VS_BUILTINFONT_H

