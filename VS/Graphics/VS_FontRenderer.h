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
#include "VS/Utils/VS_LocString.h"

class vsFontRenderer
{
	struct FragmentConstructor
	{
		vsRenderBuffer::PCT *ptArray;
		uint16_t *tlArray;

		int ptIndex;
		int tlIndex;

		// if 'm_buildMapping' is set, these contain boxes for each generated glyph
		vsBox2D *glyphBox;
		size_t glyphCount;
		// if 'm_buildMapping' is set, these contain boxes around each
		// generated wrapped line.  Note that these are NOT "exact" boxes;
		// they cover top to bottom of the main portion of the line, they do
		// not necessarily cover descenders, and they DO contain the top of
		// where capitals would be in the line, if there were any capitals.
		vsBox2D *lineBox;
		int *lineFirstGlyph;
		int *lineLastGlyph;
		size_t lineCount;
	};

	// reference only -- do not deallocate!
	vsFont *m_font;

	float m_size;    // actual size
	float m_texSize; // size of the texture we want to use.  For 2D, this should equal m_size.  For 3D, will determine quality of font bitmaps that are used.
	float m_sizeBias;
	vsVector2D m_bounds;
	JustificationType m_justification;
	vsTransform3D m_transform;
	vsArray<vsTransform3D> m_glyphTransform;
	vsArray<vsColor> m_glyphColor;
	vsColor m_color;
	vsColor m_dropShadowColor;
	vsVector3D m_dropShadowOffset;
	bool m_hasColor;
	bool m_hasDropShadow;
	bool m_snap;
	bool m_hasSnap;
	bool m_buildMapping;

// #define MAX_WRAPPED_LINES (5000)
	vsArray<vsString> m_wrappedLine;

	void		WrapStringSizeTop(const vsString &string, float *size_out, float *top_out);
	void		WrapLine(const vsString &string, float size);
	// vsFragment* CreateString_Fragment( FontContext context, const vsString& string );
	void		CreateString_InFragment( FontContext context, vsFontFragment *fragment, const vsString& string );
	void		CreateString_InDisplayList( FontContext context, vsDisplayList *list, const vsString &string );
	void		AppendStringToArrays( vsFontRenderer::FragmentConstructor *constructor, FontContext context, const char* string, const vsVector2D &size, JustificationType type, const vsVector2D &offset, int nextGlyphId, int lineId, bool dropShadow);
	void		BuildDisplayListGeometryFromString( FontContext context, vsDisplayList * list, const char* string, float size, JustificationType type, const vsVector2D &offset);

	bool		ShouldSnap( FontContext context );

	void		UpdateSize();

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

	void SetGlyphTransforms( const vsArray<vsTransform3D> &transforms );
	void SetGlyphColors( const vsArray<vsColor> &colors );

	// If set, this color will be applied to the drawn text.  If not, whatever
	// color is in the sprite font will show through unmodified.
	void SetColor( const vsColor& color );
	void SetNoColor();

	// if set, we'll draw a second copy of the text offset and behind the main text.
	void SetDropShadow( const vsColor& color, int xOff=1, int yOff=1 );
	void SetDropShadow( const vsColor& color, const vsVector3D& offset );

	// if true, the text will snap to integer coordinates.  If false, it won't.
	// If not set either way, the text will snap in a 2D context, and won't snap
	// in a 3D context.
	void SetSnap( bool snap ) { m_hasSnap = true; m_snap = snap; }

	// Font size bias adjusts our selection of what font size to use.
	//
	// By default, we assume that 2D fonts are being drawn in a way that
	// 1 unit equals 1 pixel on screen.
	//
	// If that's not the case, you can set the size bias.  With the size
	// bias, you can do this:
	//
	// Size 12, Bias 1:  Draw using font size 12, 12 units tall.
	// Size 12, Bias 2:  Draw using font size 24, 12 units tall.
	void SetSizeBias( float bias );

	// If true, we'll produce a set of boxes around glyphs, which can be used
	// to figure out where individual glyphs are.  These will be stored on
	// any vsFontFragments produced.  There's no way to get at them from
	// display lists, so this function really doesn't do anything in that case.
	// You really shouldn't be building display lists any more anyway;
	// fragments are better in almost every way!
	void BuildMapping( bool mapping = true ) { m_buildMapping = mapping; }

	// the 'fragment' approach is ideal for long-lived strings, as it produces a
	// single renderable chunk of geometry which can be drawn in a single draw call,
	// but which requires its own blob of data on the GPU.
	vsFontFragment* Fragment2D( const vsString& string );
	vsFontFragment* Fragment3D( const vsString& string );

	vsFontFragment* Fragment2D( const vsLocString& string ); //
	vsFontFragment* Fragment3D( const vsLocString& string );

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
	int GetLineCount( const vsString& string );
	static int GetGlyphCount( const vsString& string );

	friend class vsFontFragment;
};

class vsFontFragment: public vsFragment
{
	vsFontRenderer m_renderer;
	FontContext m_context;
	vsString m_string;
	vsLocString m_locString;

	vsBox2D *m_lineBox; // contains a box around each wrapped line.
	int *m_lineFirstGlyph; // the first glyph on each line.
	int *m_lineLastGlyph; // the last glyph on each line.
	size_t m_lineCount;

	vsBox2D *m_glyphBox; // contains a box around each glyph.
	size_t m_glyphCount;

	bool m_attached;

public:
	vsFontFragment( vsFontRenderer& renderer, FontContext fc, const vsString& string );
	virtual ~vsFontFragment();

	// If our font renderer had mapping enabled, these functions provide access
	// to the generated glyph mapping.
	size_t GetGlyphMappingCount() const { return m_glyphCount; }
	const vsBox2D& GetGlyphMapping(int glyph) const { return m_glyphBox[glyph]; }

	// If our font renderer had mapping enabled, these functions provide access
	// to the generated line mapping.
	size_t GetLineMappingCount() const { return m_lineCount; }
	const vsBox2D& GetLineMapping(int line) const { return m_lineBox[line]; }
	int GetLineFirstGlyph(int line) const { return m_lineFirstGlyph[line]; }
	int GetLineLastGlyph(int line) const { return m_lineLastGlyph[line]; }

	// SetLocString() informs us of the composed localisation string which
	// resulted in our text.  If set, we'll re-localise our text if we rebuild
	// (in case localisation language changed)
	void SetLocString( const vsLocString& str ) { m_locString = str; }

	// Detach() makes the fragment aware that its font no longer exists.  Should
	// only be called by the Font.  Calling it means that this fragment will no
	// no longer attempt to rebuild itself in response to DPI changes, or otherwise
	// communicate with the vsFont.
	void Detach();
	//
	// Rebuild is called in the event of a DPI change (for example, if the window
	// gets dragged from a regular screen to a Retina screen on OSX, or if the
	// 'HighDPI' setting is changed on a HighDPI screen).  This is called
	// automatically by the font when this happens;  end-users should never
	// call it manually.
	void Rebuild();

	// If we have localisation data, rebuild (because the localisation language
	// just changed, generally)
	void Rebuild_IfLocalised();

	friend class vsFontRenderer;
};


#endif // VS_FONTRENDERER_H

