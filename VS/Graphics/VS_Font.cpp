/*
 *  VS_Font.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 30/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Font.h"
#include "VS_FontRenderer.h"

#include "VS_DisplayList.h"
#include "VS_DynamicMaterial.h"
#include "VS_Fragment.h"
#include "VS_Vector.h"

#include "VS_RenderBuffer.h"
#include "VS_RenderTarget.h"
#include "VS_Screen.h"

#include "VS_Texture.h"
#include "VS_TextureManager.h"

#include "VS_File.h"
#include "VS_Record.h"

#include "Utils/utfcpp/utf8.h"

vsFontSize::vsFontSize( const vsString &filename ):
	m_glyph(nullptr),
	m_glyphCount(0),
	m_baseline(1.f),
	m_capHeight(1.f),
	m_kerning(nullptr),
	m_kerningCount(0)
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

vsFontSize::~vsFontSize()
{
	vsDeleteArray( m_glyph );
	vsDeleteArray( m_kerning );
	vsDelete( m_material );
	vsDelete( m_ptBuffer );
}

void
vsFontSize::LoadOldFormat( vsFile *font )
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
	return nullptr;
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
vsFontSize::LoadBMFont( vsFile *file )
{
	vsFile &fontData = *file;
	vsRecord r;
	int i = 0; // incrementer for glyphs
	int ki = 0; // incrementer for kerning information

	float width = 512;
	float height = 512;
	m_descenderHeight = 0.f;

	while( fontData.Record(&r) )
	{
		vsString string = r.ToString();
		vsString label = r.GetLabel().AsString();
		if ( r.GetLabel().AsString() == "info" )
		{
			m_size = (float)GetBMFontValue_Integer(&r, "size");
		}
		else if ( r.GetLabel().AsString() == "common" )
		{
			width = (float)GetBMFontValue_Integer(&r, "scaleW");
			height = (float)GetBMFontValue_Integer(&r, "scaleH");
			m_lineSpacing = (GetBMFontValue_Integer(&r, "lineHeight") - m_size) / m_size;
			m_baseline = (float)(GetBMFontValue_Integer(&r, "base")+0) / m_size;
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

			// EXAMPLE:
			//     glyph height 85
			//     yoffset 40
			//     baseline 120
			//
			//     bottom of glyph is at == glyph height + yoffset == 85+40 == 125
			//       This means that the glyph extends 5 pixels below the baseline!
			//
			//     height of glyph from baseline == baseline - yoffset == 120 - 40 == 80
			//       Only 80 of the glyph's 85 pixels are above the baseline!

			if ( m_glyph[i].glyph == 'H' ) // assume we always have an 'H'
				m_capHeight = m_baseline + m_glyph[i].baseline.y;

			if ( m_glyph[i].glyph == 'j' ||
					m_glyph[i].glyph == 'y' ||
					m_glyph[i].glyph == ',' ) // even all-cap fonts should have a comma
			{
				float glyphDescender = (-m_glyph[i].baseline.y + glyphHeight) - m_baseline;
				m_descenderHeight = vsMax( m_descenderHeight, glyphDescender );
			}

			{
				// I like to lay out text such that the baseline is at 0.
				// This raised glyph geometry compensates for how BMFont
				// wants to put the baseline at m_baseline pixels BELOW 0.
				// With this, we can render in a BMFont-approved way, and
				// still wind up with our glyphs visibly appearing above 0.
				float l = 0.f;
				float t = -m_baseline;//-1.f;//-1.f;
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
		else if ( r.GetLabel().AsString() == "kernings" )
		{
			m_kerningCount = GetBMFontValue(&r, "count")->AsInteger();
			m_kerning = new vsKerning[m_kerningCount];
		}
		else if ( r.GetLabel().AsString() == "kerning" )
		{
			m_kerning[ki].glyphA = GetBMFontValue_Integer(&r, "first");
			m_kerning[ki].glyphB = GetBMFontValue_Integer(&r, "second");
			m_kerning[ki].xAdvance = GetBMFontValue_Integer(&r, "amount") / m_size;
			ki++;
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
vsFontSize::FindGlyphForCharacter(uint32_t letter)
{
	for ( int i = 0; i < m_glyphCount; i++ )
	{
		if ( m_glyph[i].glyph == letter )
		{
			return &m_glyph[i];
		}
	}
	return nullptr;
}

float
vsFontSize::GetStringWidth( const vsString &string_in, float size )
{
	float width = 0.f;

	const char* string = string_in.c_str();
	const char* stringEnd = string + strlen(string);
	size_t len = utf8::distance(string, stringEnd);
	const char* w = string;

	for ( size_t i = 0; i < len; i++ )
	{
		uint32_t cp = utf8::next(w, stringEnd);

		if ( i == len-1 )
			width += GetCharacterWidth( cp, size ); // last character, use the full character width
		else
		{
			width += GetCharacterAdvance( cp, size ); // other characters, just use the distance we advance
			// width += GetCharacterKerning( string[i], string[i+1], size ); // also, adjust for kerning
		}
	}
	return width;
}

float
vsFontSize::GetCharacterWidth( uint32_t c, float size )
{
	float width = 0.f;

	vsGlyph *g = FindGlyphForCharacter(c);
	if ( g )
	{
		// this relies on knowing that vertex[1] is on the right, and vertex[0]
		// is on the left.
		width = g->vertex[1].x - g->vertex[0].x;
	}

	return width * size;
}

float
vsFontSize::GetCharacterAdvance( uint32_t c, float size )
{
	float width = 0.f;

	vsGlyph *g = FindGlyphForCharacter(c);
	if ( g )
	{
		width = g->xAdvance;
	}

	return width * size;
}

float
vsFontSize::GetCharacterKerning( uint32_t pChar, uint32_t nChar, float size )
{
	// TODO:  Make less absurd than this linear search
	for ( int i = 0; i < m_kerningCount; i++ )
	{
		if ( m_kerning[i].glyphA == pChar && m_kerning[i].glyphB == nChar )
		{
			return m_kerning[i].xAdvance * size;
		}
	}
	return 0.f;
}

vsFont::vsFont(const vsString& filename)
{
	vsFile file(filename);
	vsRecord r;
	while ( file.Record(&r) )
	{
		if ( r.GetLabel().AsString() == "Size" )
		{
			vsString name = r.GetToken(0).AsString();
			vsFontSize *fRecord = new vsFontSize(name);
			m_size.AddItem(fRecord);
		}
	}
}

vsFont::~vsFont()
{
	// iterate across all our fragments, cutting them loose so they don't try
	// to unregister themselves later.  (There shouldn't be any remaining,
	// actually, since they probably reference data held by the vsFontSize
	// instances?)

	int fragmentCount = m_fragment.ItemCount();
	for ( int i = 0; i < fragmentCount; i++ )
	{
		m_fragment[i]->Detach();
	}
}

vsFontSize *
vsFont::Size(float size)
{
	if ( vsScreen::Instance()->GetTrueWidth() <
			vsScreen::Instance()->GetMainRenderTarget()->GetWidth() )
	{
		float factor = vsScreen::Instance()->GetMainRenderTarget()->GetWidth() / (float)vsScreen::Instance()->GetTrueWidth();
		size *= factor;
	}

	int sizeCount = m_size.ItemCount();
	for ( int i = 0; i < sizeCount; i++ )
	{
		if ( m_size[i]->GetNativeSize() >= size )
		{
			return m_size[i];
		}
	}
	return m_size[sizeCount-1];
}

float
vsFont::MaxSize()
{
	return m_size[ m_size.ItemCount()-1 ]->GetNativeSize();
}

void
vsFont::RegisterFragment( vsFontFragment *fragment )
{
	m_fragment.AddItem(fragment);
}

void
vsFont::RemoveFragment( vsFontFragment *fragment )
{
	m_fragment.RemoveItem(fragment);
}

void
vsFont::RebuildFragments()
{
	int fragmentCount = m_fragment.ItemCount();
	for ( int i = 0; i < fragmentCount; i++ )
	{
		vsFontFragment *fragment = m_fragment[i];
		fragment->Rebuild();
	}
}

void
vsFont::RebuildLocFragments()
{
	int fragmentCount = m_fragment.ItemCount();
	for ( int i = 0; i < fragmentCount; i++ )
	{
		vsFontFragment *fragment = m_fragment[i];
		fragment->Rebuild_IfLocalised();
	}
}

