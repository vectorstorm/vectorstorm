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

class vsDisplayList;
class vsFragment;
class vsMaterial;
class vsFile;

#include "VS/Math/VS_Vector.h"

enum JustificationType
{
	Justification_Left,
	Justification_Right,
	Justification_Center
};

enum FontContext
{
	FontContext_2D,		// in 2D, fonts assume y-down
	FontContext_3D		// in 3D, fonts assume y-up
};

struct vsGlyph
{
	int		glyph;

	vsVector3D	vertex[4];        // our vertices
	vsVector2D	texel[4];         // our texture coordinates

	vsRenderBuffer	tlBuffer;     // my triangle list, referencing the global font pt-format VBO

	vsRenderBuffer	ptBuffer;
	vsRenderBuffer	vertexBuffer; // my vbo of vertex data.
	vsRenderBuffer	texelBuffer;  // my vbo of texel data.

	vsVector2D	baseline;         // where do we start drawing from?
	float		xAdvance;         // how far do we need to move our cursor, after drawing this glyph?
};

class vsFont
{
	vsRenderBuffer * m_ptBuffer;
	vsMaterial *     m_material;
	vsGlyph *        m_glyph;
	int              m_glyphCount;
	float            m_size;
	float            m_lineSpacing;

	vsRenderBuffer   m_glyphTriangleList;

	vsGlyph *		FindGlyphForCharacter( char letter );

	void LoadOldFormat(vsFile *file);
	void LoadBMFont(vsFile *file);
public:

	vsFont( const vsString &filename );
	~vsFont();

	float			GetNativeSize() { return m_size; }

	float			GetCharacterWidth(char letter, float size);
	float			GetStringWidth(const vsString &string, float size);

	friend class vsFontRenderer;
};


#endif // VS_FONT_H
