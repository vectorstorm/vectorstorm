/*
 *  VS_FontRenderer.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15/03/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_FONTRENDERER_H
#define VS_FONTRENDERER_H

#include "VS/Graphics/VS_Font.h"
#include "VS/Graphics/VS_Fragment.h"

class vsFontRenderer
{
	struct FragmentConstructor
	{
		vsRenderBuffer::PT *ptArray;
		uint16_t *			tlArray;

		int					ptIndex;
		int					tlIndex;
	};

	// reference only -- do not deallocate!
	vsFont *m_font;

	float m_size;    // actual size
	float m_texSize; // size of the texture we want to use.  For 2D, this should equal m_size.  For 3D, will determine quality of font bitmaps that are used.
	vsVector2D m_bounds;
	JustificationType m_justification;
	vsTransform3D m_transform;
	vsColor m_color;
	vsColor m_dropShadowColor;
	vsVector2D m_dropShadowOffset;
	bool m_hasColor;
	bool m_hasDropShadow;
	bool m_snap;
	bool m_hasSnap;

#define MAX_WRAPPED_LINES (5000)
	vsString	m_wrappedLine[MAX_WRAPPED_LINES];
	int m_wrappedLineCount;

	void		WrapStringSizeTop(const vsString &string, float *size_out, float *top_out);
	void		WrapLine(const vsString &string, float size);
	// vsFragment* CreateString_Fragment( FontContext context, const vsString& string );
	void		CreateString_InFragment( FontContext context, vsFontFragment *fragment, const vsString& string );
	void		CreateString_InDisplayList( FontContext context, vsDisplayList *list, const vsString &string );
	void		AppendStringToArrays( vsFontRenderer::FragmentConstructor *constructor, FontContext context, const char* string, const vsVector2D &size, JustificationType type, const vsVector2D &offset);
	void		BuildDisplayListGeometryFromString( FontContext context, vsDisplayList * list, const char* string, float size, JustificationType type, const vsVector2D &offset);

	bool		ShouldSnap( FontContext context );

public:
	vsFontRenderer( vsFont *font, float size, JustificationType type = Justification_Left );

	// useful for supporting HighDPI displays, where you may want to inform
	// VectorStorm to automatically set font sizes at 50% the exported font size.
	static void SetGlobalFontScale( float scale );

	void SetJustificationType(JustificationType type);
	void SetSize(float size);
	void SetMaxWidthAndHeight(float maxWidth, float maxHeight);
	void SetMaxWidthAndHeight(const vsVector2D &bounds);
	void SetMaxWidth(float maxWidth);

	void SetTransform( const vsTransform3D& transform );

	// If set, this color will be applied to the drawn text.  If not, whatever
	// color is in the sprite font will show through unmodified.
	void SetColor( const vsColor& color );
	void SetNoColor();

	// if set, we'll draw a second copy of the text offset and behind the main text.
	void SetDropShadow( const vsColor& color, int xOff=1, int yOff=1 );

	// if true, the text will snap to integer coordinates.  If false, it won't.
	// If not set either way, the text will snap in a 2D context, and won't snap
	// in a 3D context.
	void SetSnap( bool snap ) { m_hasSnap = true; m_snap = snap; }

	// the 'fragment' approach is ideal for long-lived strings, as it produces a
	// single renderable chunk of geometry which can be drawn in a single draw call,
	// but which requires its own blob of data on the GPU.
	vsFragment* Fragment2D( const vsString& string );
	vsFragment* Fragment3D( const vsString& string );

	// the non-fragment approach avoids creating its own data on the GPU, and instead
	// uses a single shared blob of data which is owned by the font itself.  The
	// downside of this is that it requires a separate draw call for each glyph
	// in the text, with transforms inserted between them to offset each glyph into
	// its own position.  (This approach is necessary because a vsDisplayList
	// cannot own GPU data).  In general, the Fragment approach is preferred in
	// virtually all cases!
	vsDisplayList* DisplayList2D( const vsString& string );
	vsDisplayList* DisplayList3D( const vsString& string );

	void DisplayList2D( vsDisplayList *list, const vsString& string );
	void DisplayList3D( vsDisplayList *list, const vsString& string );

	vsVector2D GetStringDimensions( const vsString& string );

	friend class vsFontFragment;
};

class vsFontFragment: public vsFragment
{
	vsFontRenderer m_renderer;
	FontContext m_context;
	vsString m_string;
	bool m_attached;
public:
	vsFontFragment( vsFontRenderer& renderer, FontContext fc, const vsString& string );
	virtual ~vsFontFragment();

	// detach makes the fragment aware that its font no longer exists.  It will
	// no longer attempt to rebuild or to alert its font that it's being destroyed.
	void Detach();
	void Rebuild();
};


#endif // VS_FONTRENDERER_H

