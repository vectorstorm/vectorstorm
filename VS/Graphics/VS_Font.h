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


// TO ME:  CONSIDER THE FOLLOWING STRUCT BEFORE COMMITTING!

struct vsFontDrawParameters		// font drawing should take one of these!!!
{
	FontContext			context;
	JustificationType	justification;

	vsColor				color;
	bool				setColor;

	float				maxWidth;

	vsFontDrawParameters():
		context(FontContext_2D),
		justification(Justification_Left),
		color(c_white),
		setColor(true),
		maxWidth(-1)
	{
	}
};

class vsBuiltInFont
{
	static void AppendCharacterToList( char c, vsDisplayList *list, const vsVector2D &offset, float size );
	static vsDisplayList * CreateString_Internal(const char* string, float size, float capSize, JustificationType j, float maxWidth = -1.f);

	static void		BuildDisplayListFromString(vsDisplayList * list, const char* string, float size, float capSize, JustificationType j, const vsVector2D &offset = vsVector2D::Zero);
	static void		BuildDisplayListFromCharacter( vsDisplayList *list, char c, float size, float capSize);

	static void		WrapLine(const vsString &string, float size, float capSize, JustificationType j, float maxWidth);

public:

	static void				Init();	// called internally

	static void				CreateStringInDisplayList(vsDisplayList *list, const vsString &string, float size, float capSize=-1.f, JustificationType j = Justification_Left, float maxWidth = -1.f);

	static vsDisplayList *	CreateString(const vsString &string, float size, float capSize=-1.f, JustificationType j = Justification_Left, float maxWidth = -1.f);
	static vsDisplayList *	CreateCharacter(char letter, float size, float capSize=-1.f);

	static float			GetCharacterWidth(char letter, float size, float capSize=-1.f);
	static float			GetStringWidth(const vsString &string, float size, float capSize=-1.f);

	static float			GetKerningForSize(float size);
};

struct vsGlyph
{
	int		glyph;

	vsVector3D	vertex[4];		// our vertices
	vsVector2D	texel[4];		// our texture coordinates

	vsRenderBuffer	tlBuffer;				// my triangle list, referencing the global font pt-format VBO

	vsRenderBuffer	ptBuffer;
	vsRenderBuffer	vertexBuffer;			// my vbo of vertex data.
	vsRenderBuffer	texelBuffer;			// my vbo of texel data.

	vsVector2D	baseline;		// where do we start drawing from?
	float		xAdvance;		// how far do we need to move our cursor, after drawing this glyph?
};

class vsFont
{
	struct FragmentConstructor
	{
		vsRenderBuffer::PT *		ptArray;
		uint16_t *			tlArray;

		int					ptIndex;
		int					tlIndex;
	};

	void AppendCharacterToList( FontContext context, char c, vsDisplayList *list, vsVector2D &offset, float size );
//	void			AppendCharacterToArrays( FontContext context, char c, vsVector3D *vertex, vsVector3D *texel, vsVector2D &offset, float size );
	vsDisplayList * CreateString_Internal( FontContext context, const char* string, float size, JustificationType j, float maxWidth = -1.f);
	vsFragment * CreateString_Fragment_Internal( FontContext context, const char* string, float size, JustificationType j, float maxWidth = -1.f);

	void		BuildDisplayListFromString( FontContext context, vsDisplayList * list, const char* string, float size, JustificationType j, const vsVector2D &offset = vsVector2D::Zero, const vsColor &color = c_white);
	void		BuildDisplayListFromCharacter( FontContext context, vsDisplayList *list, char c, float size);

	void		AppendStringToArrays( vsFont::FragmentConstructor *constructor, const char* string, const vsVector2D &size, JustificationType j, const vsVector2D &offset = vsVector2D::Zero);

	void		WrapLine(const vsString &string, float size, JustificationType j, float maxWidth);

	vsRenderBuffer *m_ptBuffer;
	vsMaterial *	m_material;
	vsGlyph *		m_glyph;
	int				m_glyphCount;
	float			m_size;
	float			m_lineSpacing;

	vsRenderBuffer		m_glyphTriangleList;

#define MAX_WRAPPED_LINES (50)

	vsString	m_wrappedLine[MAX_WRAPPED_LINES];
	int			m_wrappedLineCount;



	vsGlyph *		FindGlyphForCharacter( char letter );

	void LoadOldFormat(vsFile *file);
	void LoadBMFont(vsFile *file);
public:

	// Note:  old-format files will have a native size of "1.0".  new-format
	// BMFont files will have a native size as specified in the file.  The 'size'
	// parameter of font creation functions is a multiplier on top of the default
	// font size.
	vsFont( const vsString &filename );
	~vsFont();

	float			GetNativeSize() { return m_size; }

	void			CreateStringInDisplayList( FontContext context, vsDisplayList *list, const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f, const vsColor &color = c_white);
	void			CreateStringInDisplayList_NoClear( FontContext context, vsDisplayList *list, const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f, const vsColor &color = c_white);


	vsDisplayList *	CreateString2D( const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f) { return CreateString( FontContext_2D, string, size, j, maxWidth ); }
	vsDisplayList *	CreateCharacter2D( char letter, float size) { return CreateCharacter( FontContext_2D, letter, size ); }

	vsDisplayList *	CreateString3D( const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f) { return CreateString( FontContext_3D, string, size, j, maxWidth ); }
	vsDisplayList *	CreateCharacter3D( char letter, float size) { return CreateCharacter( FontContext_3D, letter, size ); }

	vsDisplayList *	CreateString( FontContext context, const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f, const vsColor &color = c_white);

	vsFragment *	CreateString_Fragment( FontContext context, const vsString &string, float size, JustificationType j, const vsBox2D& bounds, const vsColor &color = c_white, const vsTransform3D &transform = vsTransform3D::Identity );
//	vsFragment *	CreateString_NoColor_Fragment( FontContext context, const vsString &string, float size, JustificationType j = Justification_Left, const vsBox2D& bounds);

	vsFragment *	CreateString_Fragment( FontContext context, const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f, const vsColor &color = c_white, const vsTransform3D &transform = vsTransform3D::Identity );
	vsFragment *	CreateString_NoColor_Fragment( FontContext context, const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f);

	vsDisplayList *	CreateString_NoColor( FontContext context, const vsString &string, float size, JustificationType j = Justification_Left, float maxWidth = -1.f);
	vsDisplayList *	CreateCharacter( FontContext context, char letter, float size);

	float			GetCharacterWidth(char letter, float size);
	float			GetStringWidth(const vsString &string, float size);

	float			GetKerningForSize(float size);
};


#endif // VS_FONT_H
