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
		}
	}

	m_ptBuffer = new vsRenderBuffer;

	vsRenderBuffer::PT	*pt = new vsRenderBuffer::PT[m_glyphCount*4];

	for ( int i = 0; i < m_glyphCount; i++ )
	{
		vsGlyph *glyph = &m_glyph[i];

		uint16_t tsIndex[4];

		for ( int j = 0; j < 4; j++ )
		{
			pt[i*4+j].position = glyph->vertex[j];
			pt[i*4+j].texel = glyph->texel[i];

			tsIndex[j] = i*4+j;
		}

		glyph->tsBuffer.SetArray(tsIndex,4);
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
			i++;
		}
	}

	m_ptBuffer = new vsRenderBuffer;

	vsRenderBuffer::PT *pt = new vsRenderBuffer::PT[m_glyphCount*4];

	for ( int i = 0; i < m_glyphCount; i++ )
	{
		vsGlyph *glyph = &m_glyph[i];

		uint16_t tsIndex[4];

		for ( int j = 0; j < 4; j++ )
		{
			uint16_t index = i*4 + j;
			pt[index].position = glyph->vertex[j];
			pt[index].texel = glyph->texel[j];

			tsIndex[j] = index;
		}

		glyph->tsBuffer.SetArray(tsIndex,4);
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

float
vsFont::GetStringWidth( const vsString &string, float size )
{
	float width = 0.f;
	size_t length = string.size();
	for ( size_t i = 0; i < length; i++ )
	{
		width += GetCharacterWidth( string[i], size );
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

	return width * size;
}

