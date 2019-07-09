/*
 *  VS_FontRenderer.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15/03/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_FontRenderer.h"
#include "VS_DisplayList.h"
#include "VS_Fragment.h"
#include "Utils/utfcpp/utf8.h"

static float s_globalFontScale = 1.f;
static vsDisplayList s_tempFontList(1024*10);

void
vsFontRenderer::SetGlobalFontScale( float scale )
{
	s_globalFontScale = scale;
}

vsFontRenderer::vsFontRenderer( vsFont *font, float size, JustificationType type ):
	m_font(font),
	m_size(size),
	m_texSize(size),
	m_bounds(-1.f,-1.f),
	m_justification(type),
	m_color(c_white),
	m_hasColor(false),
	m_hasDropShadow(false),
	m_snap(false),
	m_hasSnap(false),
	m_buildMapping(false)
{
	vsAssert( m_font, "No font set??" );
}

void
vsFontRenderer::SetJustificationType(JustificationType type)
{
	m_justification = type;
}

void
vsFontRenderer::SetSize(float size)
{
	m_size = s_globalFontScale * size;
}

void
vsFontRenderer::SetMaxWidthAndHeight(float maxWidth, float maxHeight)
{
	m_bounds.Set( maxWidth, maxHeight );
}

void
vsFontRenderer::SetMaxWidthAndHeight(const vsVector2D &bounds)
{
	m_bounds = bounds;
}

void
vsFontRenderer::SetMaxWidth(float maxWidth)
{
	m_bounds.Set(maxWidth, -1.f);
}

void
vsFontRenderer::SetTransform( const vsTransform3D& transform )
{
	m_transform = transform;
}

void
vsFontRenderer::SetColor( const vsColor& color )
{
	m_hasColor = true;
	m_color = color;
}

void
vsFontRenderer::SetDropShadow( const vsColor& color, int xOff, int yOff )
{
	m_hasColor = true;
	m_hasDropShadow = true;
	m_dropShadowColor = color;
	m_dropShadowOffset.Set((float)xOff,(float)yOff);
}

void
vsFontRenderer::SetNoColor()
{
	m_hasColor = false;
}

void
vsFontRenderer::WrapStringSizeTop(const vsString &string, float *size_out, float *top_out)
{
	vsAssert( m_font, "No font set??" );
	float size = m_size;
	bool fits = false;

	float lineHeight = 1.f;
	float lineMargin = m_font->Size(size)->m_lineSpacing;

	while ( !fits )
	{
		WrapLine(string, size);
		fits = true;
		if ( m_bounds.y > 0.f )
		{
			float totalScaledHeight = size * ((lineHeight * m_wrappedLine.ItemCount()) + (lineMargin * (m_wrappedLine.ItemCount()-1)));
			if ( totalScaledHeight > m_bounds.y )
			{
				fits = false;
				// try a smaller font.
				size *= 0.95f;
			}
		}
		if ( fits && m_bounds.x > 0.f )
		{
			for ( int i = 0; i < m_wrappedLine.ItemCount(); i++ )
			{
				float totalScaledWidth = m_font->Size(size)->GetStringWidth( m_wrappedLine[i], size );
				if ( totalScaledWidth > m_bounds.x )
				{
					fits = false;
					size -= 1.0f;
				}
			}
		}
	}
	m_texSize = size;

	float totalHeight = (lineHeight * m_wrappedLine.ItemCount()) + (lineMargin * (m_wrappedLine.ItemCount()-1));
	float baseOffsetDown = 0.f;

	if ( m_justification == Justification_BottomLeft || m_justification == Justification_BottomRight || m_justification == Justification_BottomCenter )
	{
		// If we're justifying the bottom of our text to this point, we need to
		// move our text up far enough so that the BASELINE of the BOTTOM LINE
		// of the wrapped text sits here.
		baseOffsetDown = 0.f;
	}
	else if ( m_justification == Justification_TopLeft || m_justification == Justification_TopRight || m_justification == Justification_TopCenter )
	{
		// If we're justifying the top of our text to this point, we need to
		// move our text up far enough so that the TOP of the FIRST LINE
		// of the wrapped text sits here.
		if ( m_wrappedLine.ItemCount() == 1 ) // only one line, just offset down by cap height
			baseOffsetDown = m_font->Size(size)->GetCapHeight(1.f);
		else
			baseOffsetDown = totalHeight;

	}
	else
	{
		// We're centering the MIDDLE of our text to this point.
		if ( m_wrappedLine.ItemCount() == 1 ) // only one line, just offset down by cap height
			baseOffsetDown = m_font->Size(size)->GetCapHeight(1.f) * 0.5f - (1.f/size);
		else
			baseOffsetDown = totalHeight * 0.5f;
	}
	float topLinePosition = baseOffsetDown - totalHeight + lineHeight;

	*size_out = size;
	*top_out = topLinePosition;
}

void
vsFontRenderer::CreateString_InFragment( FontContext context, vsFontFragment *fragment, const vsString& string )
{
	fragment->Clear();

	size_t stringLength = string.length();
	float size = m_size;

	if ( stringLength == 0 )
		return;

	size_t requiredSize = stringLength * 4;      // we need a maximum of four verts for each character in the string.
	size_t requiredTriangles = stringLength * 6; // three indices per triangle, two triangles per character, max.

	if ( m_hasDropShadow )
	{
		requiredSize *= 2;
		requiredTriangles *= 2;
	}

	vsRenderBuffer::PCT *ptArray = new vsRenderBuffer::PCT[ requiredSize ];
	uint16_t *tlArray = new uint16_t[ requiredTriangles ];

	vsRenderBuffer *ptBuffer = new vsRenderBuffer;
	vsRenderBuffer *tlBuffer = new vsRenderBuffer;

	FragmentConstructor constructor;
	constructor.ptArray = ptArray;
	constructor.tlArray = tlArray;
	constructor.ptIndex = constructor.tlIndex = 0;

	constructor.glyphBox = NULL;
	constructor.glyphCount = 0;
	constructor.lineBox = NULL;
	constructor.lineFirstGlyph = NULL;
	constructor.lineLastGlyph = NULL;
	constructor.lineCount = 0;

	// figure out our wrapping and final render size
	float topLinePosition;
	WrapStringSizeTop(string, &size, &topLinePosition);
	if ( ShouldSnap( context ) )
	{
		int positionInPixels = (int)(topLinePosition * m_size);
		topLinePosition = positionInPixels / m_size;
	}

	vsVector2D size_vec(size, size);
	if ( context == FontContext_3D )
	{
		size_vec.y *= -1.f;	// upside down, in 3D context!
	}

	if ( m_buildMapping )
	{
		for ( int i = 0; i < m_wrappedLine.ItemCount(); i++ )
		{
			ptrdiff_t glyphs = utf8::distance( m_wrappedLine[i].c_str(), m_wrappedLine[i].c_str() + m_wrappedLine[i].size() );
			constructor.glyphCount += glyphs;
		}
		constructor.lineCount = m_wrappedLine.ItemCount();
		if ( constructor.glyphCount > 0 )
		{
			constructor.glyphBox = new vsBox2D[constructor.glyphCount];
			constructor.lineBox = new vsBox2D[constructor.lineCount];
			constructor.lineFirstGlyph = new int[constructor.lineCount];
			constructor.lineLastGlyph = new int[constructor.lineCount];
		}
	}


	vsVector2D baseOffset(0.f,topLinePosition);
	vsVector2D offset(0.f,topLinePosition);
	int nextGlyph = 0;
	float lineHeight = 1.f;
	float lineMargin = lineHeight * m_font->Size(size)->m_lineSpacing;

	bool doSnap = ShouldSnap( context );
	if ( doSnap )
	{
		vsVector4D t = m_transform.GetTranslation();
		t.x = (float)vsFloor(t.x + 0.5f);
		t.y = (float)vsFloor(t.y + 0.5f);
		t.z = (float)vsFloor(t.z + 0.5f);
		m_transform.SetTranslation(t);
	}

	if ( m_hasDropShadow )
	{
		// BLAH.  Seems like our x offset is in pixels, and our y offset is scaled by font size?
		// That's terrible;  must fix!
		offset = baseOffset;
		for ( int i = 0; i < m_wrappedLine.ItemCount(); i++ )
		{
			offset.y = topLinePosition + (i*(lineHeight+lineMargin));

			AppendStringToArrays( &constructor, context, m_wrappedLine[i].c_str(), size_vec, m_justification, offset, nextGlyph, i, true );

			ptrdiff_t glyphsInThisLine = utf8::distance( m_wrappedLine[i].c_str(), m_wrappedLine[i].c_str() + m_wrappedLine[i].size() );
			nextGlyph += (int)glyphsInThisLine;
		}
	}
	offset = baseOffset;
	nextGlyph = 0;
	for ( int i = 0; i < m_wrappedLine.ItemCount(); i++ )
	{
		offset.y = topLinePosition + (i*(lineHeight+lineMargin));

		AppendStringToArrays( &constructor, context, m_wrappedLine[i].c_str(), size_vec, m_justification, offset, nextGlyph, i, false );

		ptrdiff_t glyphsInThisLine = utf8::distance( m_wrappedLine[i].c_str(), m_wrappedLine[i].c_str() + m_wrappedLine[i].size() );
		nextGlyph += (int)glyphsInThisLine;
	}


	ptBuffer->SetArray( constructor.ptArray, constructor.ptIndex );
	tlBuffer->SetArray( constructor.tlArray, constructor.tlIndex );

	fragment->SetMaterial( m_font->Size(m_texSize)->m_material );
	fragment->AddBuffer( ptBuffer );
	fragment->AddBuffer( tlBuffer );

	vsDisplayList *list = new vsDisplayList(256);

	list->BindBuffer( ptBuffer );
	list->TriangleListBuffer( tlBuffer );
	list->ClearArrays();

	fragment->SetDisplayList( list );
	vsFontFragment* ff = dynamic_cast<vsFontFragment*>(fragment);
	if ( ff )
	{
		ff->m_glyphBox = constructor.glyphBox;
		ff->m_glyphCount = constructor.glyphCount;
		ff->m_lineBox = constructor.lineBox;
		ff->m_lineFirstGlyph = constructor.lineFirstGlyph;
		ff->m_lineLastGlyph = constructor.lineLastGlyph;
		ff->m_lineCount = constructor.lineCount;
	}
	else
	{
		vsDeleteArray( constructor.glyphBox );
		vsDeleteArray( constructor.lineBox );
		vsDeleteArray( constructor.lineFirstGlyph );
		vsDeleteArray( constructor.lineLastGlyph );
	}

	vsDeleteArray( ptArray );
	vsDeleteArray( tlArray );
}

bool
vsFontRenderer::ShouldSnap( FontContext context )
{
	return (m_hasSnap && m_snap) ||
		(!m_hasSnap && context == FontContext_2D);
}

void
vsFontRenderer::CreateString_InDisplayList( FontContext context, vsDisplayList *list, const vsString &string )
{
	bool doSnap = ShouldSnap( context );

	list->SetMaterial( m_font->Size(m_texSize)->m_material );
	if ( m_hasColor )
		list->SetColor( m_color );

	if ( m_transform != vsTransform3D::Identity )
		list->PushTransform( m_transform );
	if ( doSnap )
		list->SnapMatrix();

	float size, topLinePosition;
	WrapStringSizeTop(string, &size, &topLinePosition);

	float lineHeight = 1.f;
	float lineMargin = m_font->Size(size)->m_lineSpacing;

	vsVector2D offset(0.f,topLinePosition);

	for ( int i = 0; i < m_wrappedLine.ItemCount(); i++ )
	{
		offset.y = topLinePosition + (i*(lineHeight+lineMargin));
		s_tempFontList.Clear();
		BuildDisplayListGeometryFromString( context, &s_tempFontList, m_wrappedLine[i].c_str(), m_size, m_justification, offset );
		list->Append(s_tempFontList);
	}

	if ( m_hasColor )
		list->SetColor( c_white );

	if ( doSnap )
		list->PopTransform();
	if ( m_transform != vsTransform3D::Identity )
		list->PopTransform();
}

vsFontFragment*
vsFontRenderer::Fragment2D( const vsString& string )
{
	m_texSize = m_size;
	vsFontFragment *fragment = new vsFontFragment(*this, FontContext_2D, string);
	CreateString_InFragment(FontContext_2D, fragment, string);
	m_font->RegisterFragment(fragment);
	if ( fragment->GetDisplayList() == NULL )
	{
		vsDelete(fragment);
		return NULL;
	}
	return fragment;
}

vsFontFragment*
vsFontRenderer::Fragment3D( const vsString& string )
{
	m_texSize = m_font->MaxSize();
	vsFontFragment *fragment = new vsFontFragment(*this, FontContext_3D, string);
	CreateString_InFragment(FontContext_3D, fragment, string);
	m_font->RegisterFragment(fragment);
	if ( fragment->GetDisplayList() == NULL )
	{
		vsDelete(fragment);
		return NULL;
	}
	return fragment;
}

vsDisplayList*
vsFontRenderer::DisplayList2D( const vsString& string )
{
	m_texSize = m_size;
	vsDisplayList *loader = new vsDisplayList(1024 * 10);
	CreateString_InDisplayList(FontContext_2D, loader, string);

	vsDisplayList *result = new vsDisplayList( loader->GetSize() );
	result->Append(*loader);
	delete loader;

	return result;
}

vsDisplayList*
vsFontRenderer::DisplayList3D( const vsString& string )
{
	m_texSize = m_font->MaxSize();
	vsDisplayList *loader = new vsDisplayList(1024 * 10);
	CreateString_InDisplayList(FontContext_3D, loader, string);

	vsDisplayList *result = new vsDisplayList( loader->GetSize() );
	result->Append(*loader);
	delete loader;

	return result;
}

void
vsFontRenderer::DisplayList2D( vsDisplayList *list, const vsString& string )
{
	m_texSize = m_size;
	vsDisplayList *loader = new vsDisplayList(1024 * 10);
	CreateString_InDisplayList(FontContext_3D, loader, string);

	list->Append(*loader);
	delete loader;
}

void
vsFontRenderer::DisplayList3D( vsDisplayList *list, const vsString& string )
{
	m_texSize = m_font->MaxSize();
	vsDisplayList *loader = new vsDisplayList(1024 * 10);
	CreateString_InDisplayList(FontContext_3D, loader, string);

	list->Append(*loader);
	delete loader;
}

vsVector2D
vsFontRenderer::GetStringDimensions( const vsString& string )
{
// #define BOUNDING_BOX_METHOD
#ifdef BOUNDING_BOX_METHOD
	vsDisplayList *loader = new vsDisplayList(1024 * 10);
	CreateString_InDisplayList(FontContext_2D, loader, string);
	vsVector2D topLeft, bottomRight;
	loader->GetBoundingBox( topLeft, bottomRight );
	return bottomRight - topLeft;
#else
	float size;
	float topLinePosition;
	WrapStringSizeTop(string, &size, &topLinePosition);

	vsVector2D result;
	for ( int i = 0; i < m_wrappedLine.ItemCount(); i++ )
	{
		float width = m_font->Size(size)->GetStringWidth(m_wrappedLine[i], size);
		result.x = vsMax( width, result.x );
	}
	float lineHeight = 1.0;
	float lineMargin = m_font->Size(size)->m_lineSpacing;
	float totalScaledHeight = size * ((lineHeight * m_wrappedLine.ItemCount()) + (lineMargin * (m_wrappedLine.ItemCount()-1)));
	result.y = totalScaledHeight;
	return result;
#endif
}

int
vsFontRenderer::GetLineCount( const vsString& string )
{
	float size;
	float topLinePosition;
	WrapStringSizeTop(string, &size, &topLinePosition);
	return m_wrappedLine.ItemCount();
}


void
vsFontRenderer::WrapLine(const vsString &string, float size)
{
	vsString remainingString = string;

	// okay, check where we need to wordwrap
	size_t lineEnd = 0;
	size_t seekPosition = 0;

	float maxWidth = m_bounds.x;

	m_wrappedLine.Clear();

	while ( remainingString != vsEmptyString )
	{
		vsString line = remainingString;
		seekPosition = remainingString.find_first_of(" \n",seekPosition);
//		newlinePosition = remainingString.find('\n',newlinePosition+1);

		if ( seekPosition != vsString::npos )
		{
			line.erase(seekPosition);
		}

		// check if we want to do a line break here!
		bool outOfSpace = (maxWidth > 0.f && m_font->Size(size)->GetStringWidth(line, size) > maxWidth && lineEnd > 0) ;
		bool wrapping = outOfSpace;
		if ( seekPosition != vsString::npos )
		{
			if ( !outOfSpace && remainingString[seekPosition] == '\n' )	// newline?
			{
				lineEnd = seekPosition;
				wrapping = true;
			}
		}

		if ( wrapping )
		{
			// time to wrap!
			if ( line.size() > lineEnd )
				line.erase(lineEnd+1);
			remainingString.erase(0,lineEnd+1);
			// we've gone too far, and need to wrap!  Wrap to the last safe wrap point we found.

			m_wrappedLine.AddItem( line );

			lineEnd = 0;
			seekPosition = 0;
		}
		else if ( seekPosition == vsString::npos )
		{
			m_wrappedLine.AddItem( remainingString );
			remainingString = vsEmptyString;
		}
		else
		{
			lineEnd = seekPosition;
			seekPosition++;
		}
	}
}

void
vsFontRenderer::AppendStringToArrays( vsFontRenderer::FragmentConstructor *constructor, FontContext context, const char* string, const vsVector2D &size, JustificationType j, const vsVector2D &offset_in, int nextGlyphId, int lineId, bool dropShadow)
{
	if ( m_buildMapping )
	{
		constructor->lineFirstGlyph[lineId] = nextGlyphId;
	}

	vsVector2D offset = offset_in;
	vsBox2D lineBox;
	int glyphCount = 0;

	if ( j != Justification_Left && j != Justification_TopLeft && j != Justification_BottomLeft )
	{
		s_tempFontList.Clear();

		float width = m_font->Size(m_size)->GetStringWidth(string, size.x);

		if ( j == Justification_Right || j == Justification_TopRight || j == Justification_BottomRight )
			offset.x = -width;
		else if ( j == Justification_Center || j == Justification_TopCenter || j == Justification_BottomCenter )
			offset.x = -(width*0.5f);

		if ( ShouldSnap( context ) )
		{
			// snap our offsets!
			offset.x = (float)(int)(offset.x);
		}

		offset.x *= (1.f / size.x);

		s_tempFontList.Clear();
	}

	if ( dropShadow )
	{
		offset.x += m_dropShadowOffset.x / size.x;
		offset.y += m_dropShadowOffset.y / size.y;
	}


	uint16_t glyphIndices[6] = { 0, 2, 1, 1, 2, 3 };

	size_t len = utf8::distance(string, string + strlen(string));
	const char* w = string;
	size_t startGlyphId = nextGlyphId;
	size_t endGlyphId = nextGlyphId + len;
	vsFontSize *fontSize = m_font->Size(m_texSize);
	float sizeScale = m_texSize / fontSize->GetNativeSize();

	for ( size_t glyphId = startGlyphId; glyphId < endGlyphId; glyphId++ )
	{
		uint32_t cp = utf8::next(w, string + strlen(string));
		vsGlyph *g = fontSize->FindGlyphForCharacter( cp );

		if ( cp == '\r' )
			continue;
		if ( cp == '\t' )
			continue;

		if ( !g )
		{
			char glyphString[5];
			utf8::append(cp, glyphString);
			vsLog("Missing character in font: %d (%s)", cp, glyphString);

			const char* missingGlyphString = u8"□";
			g = fontSize->FindGlyphForCharacter(utf8::next(missingGlyphString, missingGlyphString + strlen(missingGlyphString)));

			if ( !g )
				g = fontSize->FindGlyphForCharacter( '?' );
		}
		if ( g )
		{
			vsVector2D characterOffset = offset - g->baseline;
			vsVector2D scaledPosition;

			// now, add our four verts and two triangles onto the arrays.


			for ( int i = 0; i < 4; i++ )
			{
				vsVector3D v = g->vertex[i];
				vsColor color = c_white;

				if ( glyphId < (size_t)m_glyphTransform.ItemCount() )
				{
					// Now, I just so happen to know that vertices (0,1) are
					// on the left and right sides of this quad.  And (1,2) are
					// at the top and bottom.  So let's offset by half, before we
					// transform the vertex, then offset back.
					v.x -= (g->vertex[0].x + g->vertex[1].x) * 0.5f;
					v.y -= (g->vertex[1].y + g->vertex[2].y) * 0.5f;
					v = m_glyphTransform[glyphId].ApplyTo( v );
					v.x += (g->vertex[0].x + g->vertex[1].x) * 0.5f;
					v.y += (g->vertex[1].y + g->vertex[2].y) * 0.5f;
				}
				if ( glyphId < (size_t)m_glyphColor.ItemCount() )
				{
					color = m_glyphColor[glyphId];
				}
				if ( !dropShadow && m_hasColor )
				{
					color *= m_color;
				}
				if ( dropShadow )
				{
					color *= m_dropShadowColor;
				}

				scaledPosition = v + characterOffset;
				scaledPosition.x *= size.x;
				scaledPosition.y *= size.y;

				vsVector3D transformedPosition = m_transform.ApplyTo(scaledPosition);

				constructor->ptArray[ constructor->ptIndex+i ].position = transformedPosition;
				constructor->ptArray[ constructor->ptIndex+i ].color = color;
				constructor->ptArray[ constructor->ptIndex+i ].texel = g->texel[i];

				if ( m_buildMapping )
				{
					vsVector2D mappingPosition = scaledPosition;
					if ( cp == ' ' && i == 1) // let's map spaces better
					{
						mappingPosition.x += g->xAdvance * size.x;
					}
					constructor->glyphBox[nextGlyphId].ExpandToInclude(mappingPosition);
					lineBox.ExpandToInclude(mappingPosition);
				}
			}

			for ( int i = 0; i < 6; i++ )
			{
				constructor->tlArray[ constructor->tlIndex+i ] = constructor->ptIndex + glyphIndices[i];
			}

			constructor->ptIndex += 4;
			constructor->tlIndex += 6;

			offset.x += g->xAdvance;

			if ( glyphId < endGlyphId-1 )
			{
				const char* stringEnd = string + strlen(string);
				uint32_t ncp = utf8::peek_next(w, stringEnd);
				offset.x += fontSize->GetCharacterKerning( cp, ncp, sizeScale );
			}

			nextGlyphId++;
			glyphCount++;
		}
	}

	if ( m_buildMapping )
	{
		// fix top and bottom of lineBox.
		float top = (offset.y - 1.0f) * m_size;
		float bottom = (offset.y) * m_size;
		vsVector2D tl = lineBox.GetMin();
		vsVector2D br = lineBox.GetMax();
		tl.y = top;
		br.y = bottom;
		lineBox.Set(tl,br);
		constructor->lineBox[lineId] = lineBox;
		if ( glyphCount == 0 )
			constructor->lineFirstGlyph[lineId] = nextGlyphId-1;
		constructor->lineLastGlyph[lineId] = nextGlyphId-1;
	}
}

void
vsFontRenderer::BuildDisplayListGeometryFromString( FontContext context, vsDisplayList * list, const char* string, float size, JustificationType j, const vsVector2D &offset_in)
{
	vsFontSize *fontSize = m_font->Size(size);
	float sizeScale = size / fontSize->GetNativeSize();
	vsVector2D offset = offset_in;

	if ( j != Justification_Left && j != Justification_TopLeft && j != Justification_BottomLeft )
	{
		float width = fontSize->GetStringWidth(string, size);

		if ( j == Justification_Right || j == Justification_TopRight || j == Justification_BottomRight )
			offset.x = -width;
		else if ( j == Justification_Center || j == Justification_TopCenter || j == Justification_BottomCenter )
			offset.x = -(width*0.5f);

		offset.x *= (1.f / size);

		list->Clear();
	}

	vsTransform2D transform;
	float ysize = size;
	if ( context == FontContext_3D )
	{
		ysize *= -1.f;	// upside down, in 3D context!
	}
	transform.SetScale( vsVector2D(size,ysize) );
	list->PushTransform(transform);
	list->BindBuffer( fontSize->m_ptBuffer );

	const char* stringEnd = string + strlen(string);
	size_t len = utf8::distance(string, stringEnd);
	const char* w = string;
	for ( size_t i = 0; i < len; i++ )
	{
		uint32_t cp = utf8::next(w, stringEnd);
		vsGlyph *g = fontSize->FindGlyphForCharacter( cp );

		if ( !g )
		{
			char glyphString[5];
			utf8::append(cp, glyphString);
			vsLog("Missing character in font: %d (%s)", cp, glyphString);
			// check whether we have a "missing symbol" glyph.

			const char* missingGlyphString = u8"□";
			g = fontSize->FindGlyphForCharacter( utf8::next(missingGlyphString, missingGlyphString + strlen(missingGlyphString)) );

			if ( !g )
				g = fontSize->FindGlyphForCharacter( '?' );
		}

		if ( g )
		{
			list->PushTranslation(offset - g->baseline);
			list->TriangleStripBuffer( &g->tsBuffer );
			list->PopTransform();

			offset.x += g->xAdvance;

			if ( i < len-1 )
			{
				uint32_t ncp = utf8::peek_next(w, stringEnd);
				offset.x += fontSize->GetCharacterKerning( cp, ncp, sizeScale );
			}
		}
	}

	list->PopTransform();
	list->ClearArrays();
}

void
vsFontRenderer::SetGlyphTransforms( const vsArray<vsTransform3D> &transforms )
{
	m_glyphTransform = transforms;
}

void
vsFontRenderer::SetGlyphColors( const vsArray<vsColor> &colors )
{
	m_glyphColor = colors;
}

int
vsFontRenderer::GetGlyphCount( const vsString& string )
{
	int glyphs = utf8::distance( string.c_str(), string.c_str() + string.size() );
	return glyphs;
}

vsFontFragment::vsFontFragment( vsFontRenderer& renderer, FontContext fc, const vsString& string ):
	m_renderer(renderer),
	m_context(fc),
	m_string(string),
	m_lineBox(NULL),
	m_lineFirstGlyph(NULL),
	m_lineLastGlyph(NULL),
	m_lineCount(0),
	m_glyphBox(NULL),
	m_attached(true)
{
}

vsFontFragment::~vsFontFragment()
{
	if ( m_attached )
		m_renderer.m_font->RemoveFragment(this);
	vsDeleteArray(m_glyphBox);
	vsDeleteArray(m_lineBox);
	vsDeleteArray(m_lineFirstGlyph);
	vsDeleteArray(m_lineLastGlyph);
}

void
vsFontFragment::Rebuild()
{
	if ( m_attached )
	{
		Clear();
		vsDeleteArray(m_glyphBox);
		vsDeleteArray(m_lineBox);
		vsDeleteArray(m_lineFirstGlyph);
		vsDeleteArray(m_lineLastGlyph);
		m_renderer.CreateString_InFragment( m_context, this, m_string );
	}
}

void
vsFontFragment::Detach()
{
	m_attached = false;
}

