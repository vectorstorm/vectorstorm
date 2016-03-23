/*
 *  VS_Font.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 30/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_FONT_H
#define VS_FONT_H

#include "VS_Texture.h"
#include "VS/Math/VS_Transform.h"
#include "VS_RenderBuffer.h"
#include "VS/Math/VS_Box.h"
#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_ArrayStore.h"

class vsDisplayList;
class vsFragment;
class vsMaterial;
class vsFile;

#include "VS/Math/VS_Vector.h"

class vsFontFragment;

enum JustificationType
{
	Justification_TopLeft,
	Justification_TopRight,
	Justification_TopCenter,

	Justification_Left,
	Justification_Right,
	Justification_Center,

	Justification_BottomLeft,
	Justification_BottomRight,
	Justification_BottomCenter
};

enum FontContext
{
	FontContext_2D,		// in 2D, fonts assume y-down
	FontContext_3D		// in 3D, fonts assume y-up
};

struct vsGlyph
{
	uint32_t	glyph;

	vsVector3D	vertex[4];        // our vertices
	vsVector2D	texel[4];         // our texture coordinates

	vsRenderBuffer	tsBuffer;     // my triangle list, referencing the global font pt-format VBO

	vsVector2D	baseline;         // where do we start drawing from?
	float		xAdvance;         // how far do we need to move our cursor, after drawing this glyph?
};

class vsFontSize
{
	vsRenderBuffer * m_ptBuffer;
	vsMaterial *     m_material;
	vsGlyph *        m_glyph;
	int              m_glyphCount;
	float            m_size;
	float            m_lineSpacing;

	vsRenderBuffer   m_glyphTriangleList;

	vsGlyph *		FindGlyphForCharacter( uint32_t letter ); // in UTF8 codepoint format

	void LoadOldFormat(vsFile *file);
	void LoadBMFont(vsFile *file);
public:

	vsFontSize( const vsString &filename );
	~vsFontSize();

	float			GetNativeSize() { return m_size; }

	float			GetCharacterWidth(uint32_t letter, float size);
	float			GetStringWidth(const vsString &string, float size);

	friend class vsFontRenderer;
};

class vsFont
{
	vsArrayStore<vsFontSize> m_size;
	vsArray<vsFontFragment*> m_fragment;
public:
	vsFont( const vsString &filename );
	~vsFont();

	vsFontSize* Size(float size);

	void RegisterFragment( vsFontFragment *fragment );
	void RemoveFragment( vsFontFragment *fragment );

	void RebuildFragments();
};


#endif // VS_FONT_H
