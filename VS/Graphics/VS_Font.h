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

struct vsKerning
{
	uint32_t glyphA;
	uint32_t glyphB;
	float xAdvance;
};

class vsFontSize
{
	vsRenderBuffer * m_ptBuffer;
	vsMaterial *     m_material;
	vsGlyph *        m_glyph;
	int              m_glyphCount;
	float            m_size;
	float            m_lineSpacing;
	float            m_baseline; // how far down the 'baseline' of this font is.
	float            m_capHeight; // how tall is a standard capital letter?

	vsKerning* m_kerning;
	int m_kerningCount;

	vsRenderBuffer   m_glyphTriangleList;

	vsGlyph *		FindGlyphForCharacter( uint32_t letter ); // in UTF8 codepoint format

	void LoadOldFormat(vsFile *file);
	void LoadBMFont(vsFile *file);
public:

	vsFontSize( const vsString &filename );
	~vsFontSize();

	float			GetNativeSize() { return m_size; }

	// GetCharacterWidth() returns how wide this character physically is.  For
	// example, a 'T' is quite wide.
	float			GetCharacterWidth(uint32_t letter, float size);
	// GetCharacterAdvance() returns the generic amount we'd move for this glyph.
	// This is usually pretty similar to the width value, above.
	float			GetCharacterAdvance(uint32_t letter, float size);
	// GetCharacterKerning() returns an adjustment to add to the generic amount
	// we'd move for this glyph, based upon the NEXT glyph.  For example, if we're
	// drawing 'To', then we might have a much lower offset, since the 'o' can
	// fit into the space underneath the cross-bar of the T.
	float			GetCharacterKerning( uint32_t pChar, uint32_t nChar, float size );
	float			GetStringWidth(const vsString &string, float size);
	float			GetCapHeight(float size) const { return m_capHeight * size; }

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
	float MaxSize();

	void RegisterFragment( vsFontFragment *fragment );
	void RemoveFragment( vsFontFragment *fragment );

	void RebuildFragments();
};


#endif // VS_FONT_H
