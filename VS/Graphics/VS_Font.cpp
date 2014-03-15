/*
 *  VS_Font.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 30/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Font.h"

#include "VS_DisplayList.h"
#include "VS_DynamicMaterial.h"
#include "VS_Fragment.h"
#include "VS_Vector.h"

#include "VS_RenderBuffer.h"

#include "VS_Texture.h"
#include "VS_TextureManager.h"

#include "VS_File.h"
#include "VS_Record.h"

// #define USE_EXPLICIT_ARRAY	// if set, we'll use vertex/texel arrays, instead of VBOs.

// #define FONTS_DRAW_FROM_BASE	// if set, fonts get drawn with their baseline at 0.  If not, the middle of the font goes there.

static vsDisplayList s_tempFontList(1024*10);

static float GetDisplayListWidth( vsDisplayList *list )
{
	vsVector2D topLeft, bottomRight;

	list->GetBoundingBox(topLeft, bottomRight);
	topLeft.x = 0.f;
	float width = bottomRight.x - topLeft.x;
	return width;
}

vsFont::vsFont( const vsString &filename ):
	m_glyph(NULL),
	m_glyphCount(0)
{
	uint16_t indices[6] = { 0, 2, 1, 1, 2, 3 };
	m_glyphTriangleList.SetArray( indices, 6 );

	vsFile fontData(filename);

	if ( filename.find(".fnt") == vsString::npos )	// This is awful.
	{
		LoadOldFormat(&fontData);
	}
	else
	{
		LoadBMFont(&fontData);
	}
}

vsFont::~vsFont()
{
	vsDeleteArray( m_glyph );
	vsDelete( m_material );
	vsDelete( m_ptBuffer );
}

void
vsFont::LoadOldFormat( vsFile *font )
{
	m_size = 1.f;
	m_lineSpacing = 0.4f;
	vsFile &fontData = *font;
	vsRecord r;
	int i = 0;

	while( fontData.Record(&r) )
	{
		if ( r.GetLabel().AsString() == "Texture" )
		{
			vsString textureName = r.GetToken(0).AsString();
			vsDynamicMaterial *mat = new vsDynamicMaterial();
			mat->SetTexture(0, textureName);
			mat->SetDrawMode(DrawMode_Normal);
			mat->SetLayer(101);
			mat->SetPostGlow(true);
			m_material = mat;
		}
		else if ( r.GetLabel().AsString() == "Material" )
		{
			vsString materialName = r.GetToken(0).AsString();
			m_material = new vsMaterial(materialName);
		}
		else if ( r.GetLabel().AsString() == "GlyphCount" )
		{
			m_glyphCount = r.GetToken(0).AsInteger();
			m_glyph = new vsGlyph[m_glyphCount];
		}
		else if ( r.GetLabel().AsString() == "Glyph" )
		{
			vsAssert(i < m_glyphCount, "ERROR:  Too many glyphs!");
			vsGlyph *g = &m_glyph[i++];

			g->glyph = r.GetToken(0).AsInteger();
			g->xAdvance = 0.f;

			bool chomping = true;

			while ( chomping && fontData.PeekRecord(&r) )
			{
				if ( r.GetLabel().AsString() == "Bounds" )
				{
					float l = r.GetToken(0).AsFloat();
					float t = r.GetToken(1).AsFloat();
					float w = r.GetToken(2).AsFloat();
					float h = r.GetToken(3).AsFloat();

					g->vertex[0].Set(l,t,0.f);
					g->vertex[1].Set(l+w,t,0.f);
					g->vertex[2].Set(l,t+h,0.f);
					g->vertex[3].Set(l+w,t+h,0.f);

					fontData.Record(&r);
				}
				else if ( r.GetLabel().AsString() == "Texels" )
				{
					float l = r.GetToken(0).AsFloat();
					float t = r.GetToken(1).AsFloat();
					float w = r.GetToken(2).AsFloat();
					float h = r.GetToken(3).AsFloat();

					g->texel[0].Set(l,t);
					g->texel[1].Set(l+w,t);
					g->texel[2].Set(l,t+h);
					g->texel[3].Set(l+w,t+h);

					fontData.Record(&r);
				}
				else if ( r.GetLabel().AsString() == "Origin" )
				{
					g->baseline.Set( r.GetToken(0).AsFloat(), r.GetToken(1).AsFloat() );
					fontData.Record(&r);
				}
				else if ( r.GetLabel().AsString() == "Kern" )
				{
					g->xAdvance = r.GetToken(0).AsFloat();
					fontData.Record(&r);
				}
				else
				{
					chomping = false;
				}
			}

			g->vertexBuffer.SetArray(g->vertex,4);
			g->texelBuffer.SetArray(g->texel,4);

			vsRenderBuffer::PT	pt[4];
			for ( int i = 0; i < 4; i++ )
			{
				pt[i].position = g->vertex[i];
				pt[i].texel = g->texel[i];
			}
			vsAssert(sizeof(pt) == 128,"Something's gone wrong??");
			g->ptBuffer.SetArray(pt,4);
		}
	}

	m_ptBuffer = new vsRenderBuffer;

	vsRenderBuffer::PT	*pt = new vsRenderBuffer::PT[m_glyphCount*4];

	for ( int i = 0; i < m_glyphCount; i++ )
	{
		vsGlyph *glyph = &m_glyph[i];

		uint16_t tlIndex[4];

		for ( int j = 0; j < 4; j++ )
		{
			pt[i*4+j].position = glyph->vertex[j];
			pt[i*4+j].texel = glyph->texel[i];

			tlIndex[j] = i*4+j;
		}

		glyph->tlBuffer.SetArray(tlIndex,4);
	}
	m_ptBuffer->SetArray(pt, m_glyphCount*4);

	vsDeleteArray(pt);


}

vsToken * GetBMFontValue( vsRecord *r, const vsString& label )
{
	for ( int i = 0; i < r->GetTokenCount()-2; i++ )
	{
		if ( r->GetToken(i).GetType() == vsToken::Type_Label &&
				r->GetToken(i).AsString() == label )
		{
			return &r->GetToken(i+2);
		}
	}
	return NULL;
}

vsString GetBMFontValue_String( vsRecord *r, const vsString& label )
{
	for ( int i = 0; i < r->GetTokenCount()-2; i++ )
	{
		if ( r->GetToken(i).GetType() == vsToken::Type_Label &&
				r->GetToken(i).AsString() == label )
		{
			return r->GetToken(i+2).AsString();
		}
	}
	return "";
}

int GetBMFontValue_Integer( vsRecord *r, const vsString& label )
{
	for ( int i = 0; i < r->GetTokenCount()-2; i++ )
	{
		if ( r->GetToken(i).GetType() == vsToken::Type_Label &&
				r->GetToken(i).AsString() == label )
		{
			return r->GetToken(i+2).AsInteger();
		}
	}
	return 0;
}

void
vsFont::LoadBMFont( vsFile *file )
{
	vsFile &fontData = *file;
	vsRecord r;
	int i = 0;

	float width = 512;
	float height = 512;

	while( fontData.Record(&r) )
	{
		vsString string = r.ToString();
		vsString label = r.GetLabel().AsString();
		if ( r.GetLabel().AsString() == "info" )
		{
			m_size = GetBMFontValue_Integer(&r, "size");
		}
		else if ( r.GetLabel().AsString() == "common" )
		{
			width = (float)GetBMFontValue_Integer(&r, "scaleW");
			height = (float)GetBMFontValue_Integer(&r, "scaleH");
			m_lineSpacing = (GetBMFontValue_Integer(&r, "lineHeight") - m_size) / m_size;
		}
		else if ( r.GetLabel().AsString() == "page" )
		{
			vsString filename = GetBMFontValue(&r, "file")->AsString();
			m_material = new vsMaterial(filename);
			// vsDynamicMaterial *m = new vsDynamicMaterial;
			// m->SetTexture(0, filename);
		}
		else if ( r.GetLabel().AsString() == "chars" )
		{
			m_glyphCount = GetBMFontValue(&r, "count")->AsInteger();
			m_glyph = new vsGlyph[m_glyphCount];
		}
		else if ( r.GetLabel().AsString() == "char" )
		{
			float glyphWidth = GetBMFontValue_Integer(&r, "width") / m_size;
			float glyphHeight = GetBMFontValue_Integer(&r, "height") / m_size;
			m_glyph[i].glyph = GetBMFontValue(&r, "id")->AsInteger();
			// m_glyph[i].baseline = vsVector2D::Zero;
			m_glyph[i].baseline.Set(
					-GetBMFontValue(&r, "xoffset")->AsInteger() / m_size,
					-GetBMFontValue(&r, "yoffset")->AsInteger() / m_size
					);

			m_glyph[i].xAdvance = GetBMFontValue_Integer(&r, "xadvance") / m_size;

			{
				// my original code was measuring from the bottom --
				// BMFont format measures from the top.  So we need to
				// lift our characters up by one character-height in order
				// to draw in the same position as the old font code.
				float l = 0.f;
				float t = -1.f;
				float w = glyphWidth;
				float h = glyphHeight;

				m_glyph[i].vertex[0].Set(l,t,0.f);
				m_glyph[i].vertex[1].Set(l+w,t,0.f);
				m_glyph[i].vertex[2].Set(l,t+h,0.f);
				m_glyph[i].vertex[3].Set(l+w,t+h,0.f);
			}
			{
				float l = GetBMFontValue_Integer(&r, "x") / width;
				float t = GetBMFontValue_Integer(&r, "y") / height;
				float w = GetBMFontValue_Integer(&r, "width") / width;
				float h = GetBMFontValue_Integer(&r, "height") / height;

				m_glyph[i].texel[0].Set(l,t);
				m_glyph[i].texel[1].Set(l+w,t);
				m_glyph[i].texel[2].Set(l,t+h);
				m_glyph[i].texel[3].Set(l+w,t+h);
			}
			vsGlyph *g = &m_glyph[i];
			g->vertexBuffer.SetArray(g->vertex,4);
			g->texelBuffer.SetArray(g->texel,4);

			vsRenderBuffer::PT	pt[4];
			for ( int j = 0; j < 4; j++ )
			{
				pt[j].position = g->vertex[j];
				pt[j].texel = g->texel[j];
			}
			vsAssert(sizeof(pt) == 128,"Something's gone wrong??");
			g->ptBuffer.SetArray(pt,4);

			i++;
		}
	}

	m_ptBuffer = new vsRenderBuffer;

	vsRenderBuffer::PT	*pt = new vsRenderBuffer::PT[m_glyphCount*4];

	for ( int i = 0; i < m_glyphCount; i++ )
	{
		vsGlyph *glyph = &m_glyph[i];

		uint16_t tlIndex[4];

		for ( int j = 0; j < 4; j++ )
		{
			pt[i*4+j].position = glyph->vertex[j];
			pt[i*4+j].texel = glyph->texel[i];

			tlIndex[j] = i*4+j;
		}

		glyph->tlBuffer.SetArray(tlIndex,4);
	}
	m_ptBuffer->SetArray(pt, m_glyphCount*4);

	vsDeleteArray(pt);
}

vsGlyph *
vsFont::FindGlyphForCharacter(char letter)
{
	for ( int i = 0; i < m_glyphCount; i++ )
	{
		if ( m_glyph[i].glyph == letter )
		{
			return &m_glyph[i];
		}
	}
	return NULL;
}

vsDisplayList *
vsFont::CreateString_NoColor( FontContext context, const vsString &string, float size, JustificationType j, float maxWidth )
{
	vsDisplayList *result = NULL;
	vsDisplayList loader(1024 * 10);

	CreateStringInDisplayList_NoClear( context, &loader, string, size, j, maxWidth );

	size_t displayListSize = loader.GetSize();
	if ( displayListSize )
	{
		result = new vsDisplayList( displayListSize );
		result->Append(loader);
	}

	return result;
}

vsDisplayList *
vsFont::CreateString( FontContext context, const vsString &string, float size, JustificationType j, float maxWidth, const vsColor &color )
{
	vsDisplayList *result = NULL;
	vsDisplayList loader(1024 * 10);

	CreateStringInDisplayList_NoClear( context, &loader, string, size, j, maxWidth, color );

	size_t displayListSize = loader.GetSize();
	if ( displayListSize )
	{
		result = new vsDisplayList( displayListSize );
		result->Append(loader);
	}

	return result;
}

// vsFragment *
// vsFont::CreateString_Fragment(FontContext context, const vsString &string, float size, JustificationType j, float maxWidth, const vsColor &color, const vsTransform3D &transform )
// {
// 	return CreateString_Fragment( context, string, size, j, vsBox2D::CenteredBox( vsVector2D(maxWidth, -1) ), color, transform );
// }

vsFragment *
vsFont::CreateString_Fragment(FontContext context, const vsString &string, float size, JustificationType j, const vsBox2D &bounds, const vsColor &color, const vsTransform3D &transform )
{
	size_t stringLength = string.length();

	if ( stringLength == 0 )
		return NULL;

	size_t requiredSize = stringLength * 4;		// we need a maximum of four verts for each character in the string.
	size_t requiredTriangles = stringLength * 6;	// three indices per triangle, two triangles per character, max.

	vsRenderBuffer::PT *ptArray = new vsRenderBuffer::PT[ requiredSize ];
	uint16_t *tlArray = new uint16_t[ requiredTriangles ];

	vsRenderBuffer *ptBuffer = new vsRenderBuffer;
	vsRenderBuffer *tlBuffer = new vsRenderBuffer;

	FragmentConstructor constructor;
	constructor.ptArray = ptArray;
	constructor.tlArray = tlArray;
	constructor.ptIndex = constructor.tlIndex = 0;

	bool fits = false;

	float lineHeight = 1.f;
	float lineMargin = m_lineSpacing;

	while ( !fits )
	{
		WrapLine(string, size, j, bounds.Width());
		fits = true;
		if ( bounds.Height() != -1.f )
		{
			float totalScaledHeight = size * ((lineHeight * m_wrappedLineCount) + (lineMargin * (m_wrappedLineCount-1)));
			if ( totalScaledHeight > bounds.Height() )
			{
				fits = false;
				// try a smaller font.
				size *= 0.95f;
			}
		}
	}

	vsVector2D size_vec(size, size);
	if ( context == FontContext_3D )
	{
		size_vec.y *= -1.f;	// upside down, in 3D context!
	}

	float totalHeight = (lineHeight * m_wrappedLineCount) + (lineMargin * (m_wrappedLineCount-1));
	float baseOffsetDown = totalHeight * 0.5f;
	float topLinePosition = baseOffsetDown - totalHeight + lineHeight;

	vsVector2D offset(0.f,topLinePosition);
#ifdef FONTS_DRAW_FROM_BASE
	// TODO:  This seems sensible, but it gets complicated when strings wrap --
	// do we move the top line in that case?  Maybe this needs to be (yet) more
	// configurable settings for the font drawing code?
	float totalHeight = lineHeight * m_size;
	if ( m_wrappedLineCount > 1 )
	{
		totalHeight += (lineHeight + lineMargin) * (m_wrappedLineCount-1);
	}
	float totalExtraHeight = totalHeight - (lineHeight * m_size);
	float baseOffsetDown = vsFloor(totalExtraHeight * 0.5f);
	float topLinePosition = baseOffsetDown - totalExtraHeight;
	vsVector2D offset(0.f,topLinePosition / m_size);
#endif

	for ( int i = 0; i < m_wrappedLineCount; i++ )
	{
		offset.y = topLinePosition + (i*(lineHeight+lineMargin));

		AppendStringToArrays( &constructor, m_wrappedLine[i].c_str(), size_vec, j, offset );
	}

	ptBuffer->SetArray( constructor.ptArray, constructor.ptIndex );
	tlBuffer->SetArray( constructor.tlArray, constructor.tlIndex );

	vsFragment *fragment = new vsFragment;

	fragment->SetMaterial( m_material );
	fragment->AddBuffer( ptBuffer );
	fragment->AddBuffer( tlBuffer );

	vsDisplayList *list = new vsDisplayList(60);

	if ( transform != vsTransform3D::Identity )
		list->PushTransform( transform );
	list->SnapMatrix();
	list->SetColor( color );
	list->BindBuffer( ptBuffer );
	list->TriangleListBuffer( tlBuffer );
	list->ClearArrays();
	list->PopTransform();
	if ( transform != vsTransform3D::Identity )
		list->PopTransform();

	fragment->SetDisplayList( list );

	vsDeleteArray( ptArray );
	vsDeleteArray( tlArray );

	return fragment;
}

vsFragment *
vsFont::CreateString_NoColor_Fragment(FontContext context, const vsString &string, float size, JustificationType j, float maxWidth)
{
	size_t stringLength = string.length();

	if ( stringLength == 0 )
		return NULL;

	int requiredSize = stringLength * 4;		// we need a maximum of four verts for each character in the string.
	int requiredTriangles = stringLength * 6;	// three indices per triangle, two triangles per character, max.

	vsRenderBuffer::PT *ptArray = new vsRenderBuffer::PT[ requiredSize ];
	uint16_t *tlArray = new uint16_t[ requiredTriangles ];

	vsRenderBuffer *ptBuffer = new vsRenderBuffer;
	vsRenderBuffer *tlBuffer = new vsRenderBuffer;

	FragmentConstructor constructor;
	constructor.ptArray = ptArray;
	constructor.tlArray = tlArray;
	constructor.ptIndex = constructor.tlIndex = 0;

	vsVector2D size_vec(size, size);
	if ( context == FontContext_3D )
	{
		size_vec.y *= -1.f;	// upside down, in 3D context!
	}

	if ( 1 )//maxWidth > 0.f )
	{
		WrapLine(string, size, j, maxWidth);

		float lineHeight = 1.f;
		float lineMargin = m_lineSpacing;

		float totalHeight = (lineHeight * m_wrappedLineCount) + (lineMargin * (m_wrappedLineCount-1));
		float baseOffsetDown = totalHeight * 0.5f;
		float topLinePosition = baseOffsetDown - totalHeight + lineHeight;

		vsVector2D offset(0.f,topLinePosition);

		for ( int i = 0; i < m_wrappedLineCount; i++ )
		{
			offset.y = topLinePosition + (i*(lineHeight+lineMargin));

			AppendStringToArrays( &constructor, m_wrappedLine[i].c_str(), size_vec, j, offset );
		}
	}
	else
	{
		vsVector2D offset(0.f,0.5f);

		AppendStringToArrays( &constructor, string.c_str(), size_vec, j, offset );
	}

	ptBuffer->SetArray( constructor.ptArray, constructor.ptIndex );
	tlBuffer->SetArray( constructor.tlArray, constructor.tlIndex );

	vsFragment *fragment = new vsFragment;

	fragment->SetMaterial( m_material );
	fragment->AddBuffer( ptBuffer );
	fragment->AddBuffer( tlBuffer );

	vsDisplayList *list = new vsDisplayList(30);

	list->BindBuffer( ptBuffer );
	list->TriangleListBuffer( tlBuffer );
	list->ClearArrays();

	fragment->SetDisplayList( list );

	vsDeleteArray( ptArray );
	vsDeleteArray( tlArray );

	return fragment;
}

void
vsFont::CreateStringInDisplayList_NoClear( FontContext context, vsDisplayList *list, const vsString &string, float size, JustificationType j, float maxWidth, const vsColor &color )
{
	list->SetMaterial( m_material );
	list->SetColor( color );

	if ( 1 )//maxWidth > 0.f )
	{
		WrapLine(string, size, j, maxWidth);

		float lineHeight = 1.f;
		float lineMargin = m_lineSpacing;

		float totalHeight = (lineHeight * m_wrappedLineCount) + (lineMargin * (m_wrappedLineCount-1));
		float baseOffsetDown = totalHeight * 0.5f;
		float topLinePosition = baseOffsetDown - totalHeight + lineHeight;

		vsVector2D offset(0.f,topLinePosition);

		for ( int i = 0; i < m_wrappedLineCount; i++ )
		{
			offset.y = topLinePosition + (i*(lineHeight+lineMargin));
			s_tempFontList.Clear();
			BuildDisplayListFromString( context, &s_tempFontList, m_wrappedLine[i].c_str(), size, j, offset, color );
			list->Append(s_tempFontList);
		}
	}
	else
	{
		vsVector2D offset(0.f,0.5f);

		BuildDisplayListFromString( context, list, string.c_str(), size, j, offset, color );
	}
}

void
vsFont::AppendStringToArrays( vsFont::FragmentConstructor *constructor, const char* string, const vsVector2D &size, JustificationType j, const vsVector2D &offset_in)
{
	vsVector2D offset = offset_in;
	size_t len = strlen(string);

	if ( j != Justification_Left )
	{
		s_tempFontList.Clear();

		BuildDisplayListFromString( FontContext_2D, &s_tempFontList, string, size.x, Justification_Left);
		float width = GetDisplayListWidth(&s_tempFontList);

		if ( j == Justification_Right )
			offset.x = -width;
		if ( j == Justification_Center )
			offset.x = -(width*0.5f);

		offset.x *= (1.f / size.x);

		s_tempFontList.Clear();
	}

	uint16_t glyphIndices[6] = { 0, 2, 1, 1, 2, 3 };

	for ( size_t i = 0; i < len; i++ )
	{
		vsGlyph *g = FindGlyphForCharacter( string[i] );

		if ( !g )
		{
			vsLog("Missing character in font: %c", string[i]);
		}
		else
		{
			vsVector2D characterOffset = offset - g->baseline;
			vsVector2D scaledPosition;

			// now, add our four verts and two triangles onto the arrays.

			for ( int i = 0; i < 4; i++ )
			{
				scaledPosition = g->vertex[i] + characterOffset;
				scaledPosition.x *= size.x;
				scaledPosition.y *= size.y;

				constructor->ptArray[ constructor->ptIndex+i ].position = scaledPosition;
				constructor->ptArray[ constructor->ptIndex+i ].texel = g->texel[i];
			}
			for ( int i = 0; i < 6; i++ )
			{
				constructor->tlArray[ constructor->tlIndex+i ] = constructor->ptIndex + glyphIndices[i];
			}

			constructor->ptIndex += 4;
			constructor->tlIndex += 6;


			offset.x += g->xAdvance;
		}
	}
}

void
vsFont::CreateStringInDisplayList( FontContext context, vsDisplayList *list, const vsString &string, float size, JustificationType j, float maxWidth, const vsColor &color )
{
	list->Clear();
	CreateStringInDisplayList_NoClear( context, list, string, size, j, maxWidth, color);
}

void
vsFont::BuildDisplayListFromString( FontContext context, vsDisplayList * list, const char* string, float size, JustificationType j, const vsVector2D &offset_in, const vsColor &color)
{
	vsVector2D offset = offset_in;
	size_t len = strlen(string);

	if ( j != Justification_Left )
	{
		BuildDisplayListFromString( context, list, string, size, Justification_Left);
		float width = GetDisplayListWidth(list);

		if ( j == Justification_Right )
			offset.x = -width;
		if ( j == Justification_Center )
			offset.x = -(width*0.5f);

		offset.x *= (1.f / size);

		list->Clear();
	}

	list->SetMaterial( m_material );
	list->SetColor( color );
	vsTransform2D transform;
	float ysize = size;
	if ( context == FontContext_3D )
	{
		ysize *= -1.f;	// upside down, in 3D context!
	}
	transform.SetScale( vsVector2D(size,ysize) );
	list->PushTransform(transform);

#if defined(USE_EXPLICIT_ARRAY)
	vsVector3D *vertexArray = new vsVector3D[len*4];
	vsVector2D *texelArray = new vsVector2D[len*4];
	int *triangleListArray = new int[len*6];
	int vertexCount = 0;
	int triangleIndexCount = 0;
#endif

	for ( size_t i = 0; i < len; i++ )
	{
		vsGlyph *g = FindGlyphForCharacter( string[i] );

		if ( !g )
		{
			vsLog("Missing character in font: %c", string[i]);
		}
		else
		{
#if defined(USE_EXPLICIT_ARRAY)
			triangleListArray[triangleIndexCount++] = vertexCount;
			triangleListArray[triangleIndexCount++] = vertexCount + 2;
			triangleListArray[triangleIndexCount++] = vertexCount + 1;
			triangleListArray[triangleIndexCount++] = vertexCount + 1;
			triangleListArray[triangleIndexCount++] = vertexCount + 2;
			triangleListArray[triangleIndexCount++] = vertexCount + 3;

			for ( int vi = 0; vi < 4; vi++ )
			{
				vertexArray[vertexCount] = g->vertex[vi] + offset - g->baseline;
				texelArray[vertexCount] = g->texel[vi];
				vertexCount++;
			}
#else
			list->PushTranslation(offset - g->baseline);
			list->BindBuffer( &g->ptBuffer );
			list->TriangleListBuffer( &m_glyphTriangleList );

			list->PopTransform();
#endif
			offset.x += g->xAdvance;
		}
	}
#if defined(USE_EXPLICIT_ARRAY)
	list->VertexArray(vertexArray, vertexCount);
	list->TexelArray(texelArray, vertexCount);
	list->TriangleList(triangleListArray, triangleIndexCount);

	vsDeleteArray(vertexArray);
	vsDeleteArray(texelArray);
	vsDeleteArray(triangleListArray);
#endif

	list->PopTransform();
	list->SetMaterial( vsMaterial::White );
	list->ClearArrays();
}

void
vsFont::AppendCharacterToList( FontContext context, char c, vsDisplayList *list, vsVector2D &offset, float size )
{
	vsGlyph *g = FindGlyphForCharacter(c);

	if ( !g )
	{
		vsLog("Missing character in font: %c", c);
	}
	else
	{
		vsVector3D v[4];
		for ( int i = 0; i < 4; i++ )
		{
			v[i] = size * (g->vertex[i] + offset - g->baseline);
		}

		int strip[4] = {0,2,1,3};

		list->VertexArray(v,4);
		list->TexelArray(g->texel,4);
		list->TriangleStrip(strip,4);

		offset.x += g->xAdvance;
	}
}


void
vsFont::WrapLine(const vsString &string, float size, JustificationType j, float maxWidth)
{
	vsString remainingString = string;

	// okay, check where we need to wordwrap
	size_t lineEnd = 0;
	size_t seekPosition = 0;
	//bool done = false;

	m_wrappedLineCount = 0;

	while ( remainingString != vsEmptyString )
	{
		vsString line = remainingString;
		seekPosition = remainingString.find_first_of(" \n",seekPosition+1);
//		newlinePosition = remainingString.find('\n',newlinePosition+1);

		if ( seekPosition != vsString::npos )
		{
			line.erase(seekPosition);
		}

		// check if we want to do a line break here!
		bool outOfSpace = (maxWidth > 0.f && GetStringWidth(line, size) > maxWidth && lineEnd > 0) ;
		bool wrapping = outOfSpace;
		if ( seekPosition != vsString::npos )
		{
			if ( remainingString[seekPosition] == '\n' )	// newline?
			{
				lineEnd = seekPosition;
				wrapping = true;
			}
		}

		if ( wrapping )
		{
			// time to wrap!
			line.erase(lineEnd);
			remainingString.erase(0,lineEnd+1);
			// we've gone too far, and need to wrap!  Wrap to the last safe wrap point we found.

			m_wrappedLine[m_wrappedLineCount++] = line;

			lineEnd = 0;
			seekPosition = 0;
		}
		else if ( seekPosition == vsString::npos )
		{
			m_wrappedLine[m_wrappedLineCount] = remainingString;
			m_wrappedLineCount++;
			remainingString = vsEmptyString;
		}
		else
		{
			lineEnd = seekPosition;
		}
	}
}

float
vsFont::GetStringWidth( const vsString &string, float size )
{
	float width = 0.f;

	s_tempFontList.Clear();
	BuildDisplayListFromString( FontContext_2D, &s_tempFontList, string.c_str(), size, Justification_Left );

	{
		vsVector2D topLeft, bottomRight;

		s_tempFontList.GetBoundingBox(topLeft, bottomRight);
		topLeft.x = 0.f;
		width = bottomRight.x - topLeft.x;
	}

	return width;
}

float
vsFont::GetCharacterWidth( char c, float size )
{
	float width = 0.f;

	vsGlyph *g = FindGlyphForCharacter(c);
	if ( g )
	{
		width = g->xAdvance;
	}

	return width;
}

