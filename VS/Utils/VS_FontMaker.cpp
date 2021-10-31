/*
 *  VS_FontMaker.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 28/12/08.
 *  Copyright 2008 PanicKitten Softworks. All rights reserved.
 *
 */

#if 0
#if !TARGET_OS_IPHONE
#if defined(_DEBUG)

#include "VS_FontMaker.h"

#include "FS_File.h"
#include "FS_Record.h"

#include <SDL_ttf.h>

struct Glyph
{
	char	glyph;
	int		minx;	// define the box where do I draw, relative to the origin
	int		maxx;
	int		miny;
	int		maxy;
	int		advance;	// how far the origin moves in X after we draw.

	SDL_Surface *	surface;
};

void
vsFontMaker::MakeFont( const vsString &name, float pointSize )
{
	if ( 0 != TTF_Init() )
	{
		HandleTTFError("TTF_Init");
		return;
	}

	vsString fontFileName = name + ".ttf";
	TTF_Font *font = TTF_OpenFont( vsFile::GetFullFilename(fontFileName).c_str(), pointSize );
	if ( !font )
	{
		HandleTTFError("TTF_OpenFont");
	}
	else
	{
		SDL_Color bg={0,0,0};
		SDL_Color color={255,255,255};

		const char *glyphsToRender = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ `1234567890-=~!@#$%^&*()_+,./;'[]\\<>?:\"{}|";
		int glyphCount = strlen(glyphsToRender);

		float baseline = 0.f;//TTF_FontAscent(font);

		Glyph *gcache = new Glyph[glyphCount];

		for ( int i = 0; i < glyphCount; i++ )
		{
			gcache[i].surface = nullptr;
			if(TTF_GlyphMetrics(font,glyphsToRender[i],&gcache[i].minx,&gcache[i].maxx,&gcache[i].miny,&gcache[i].maxy,&gcache[i].advance)==-1)
			{
				HandleTTFError("TTF_GlyphMetrics");
			}
			else
			{
				gcache[i].glyph = glyphsToRender[i];
				gcache[i].surface = TTF_RenderGlyph_Shaded(font,glyphsToRender[i],color,bg);

				baseline = vsMax( baseline, gcache[i].maxy );
			}
		}

		//int descent = TTF_FontDescent(font);

		TTF_CloseFont(font);
		font = nullptr;

		// now at this point, let's try to fit into a 512x512

		/* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
		 as expected by OpenGL for textures */
		SDL_Surface *fontPage;
		vsFile fontData(vsFormatString("%s.txt", name.c_str()), vsFile::MODE_Write);
		vsRecord r;
		r.SetLabel("Texture");
		r.SetTokenCount(1);
		r.GetToken(0).SetString(name + ".bmp");
		fontData.Record(&r);

		r.SetLabel("GlyphCount");
		r.SetTokenCount(1);
		r.GetToken(0).SetInteger(glyphCount);
		fontData.Record(&r);


		Uint32_t rmask, gmask, bmask, amask;

		/* SDL interprets each pixel as a 32-bit number, so our masks must depend
		 on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
#endif

		float width = 512;
		float height = 512;

		fontPage = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32,
									   rmask, gmask, bmask, amask);
		vsAssert(fontPage != nullptr, vsFormatString("CreateRGBSurface failed: %s\n", SDL_GetError()));

		int x = 1;	// start one pixel in, for a safe border
		int y = 1;	// start one pixel down, for a safe border.

		//int lineHeight = (baseline - descent) + 2;
		int lowestPointThisLine = y;


		for ( int i = 0; i < glyphCount; i++ )
		{
			Glyph *g = &gcache[i];

			int actualWidth = g->maxx - g->minx;
			int actualHeight = g->maxy - g->miny;

			if ( x + actualWidth > width )	// go to the next line!
			{
				x = 1;
				y = lowestPointThisLine + 1;
			}
			if ( y + actualHeight > height )
			{
				vsLog("Ran out of space in font page!");
				break;	// ran out of space!
			}

			lowestPointThisLine = vsMax( lowestPointThisLine, y + actualHeight );

			SDL_Rect srcRect = {0,0,actualWidth,actualHeight};
			SDL_Rect dstRect = {x,y,actualWidth,actualHeight};
			//dstRect.y += baseline - g->maxy;

			SDL_BlitSurface( g->surface, &srcRect, fontPage, &dstRect );

			vsRecord r;
			r.SetLabel("Glyph");
			r.SetTokenCount(2);
			r.GetToken(0).SetInteger(g->glyph);
			r.GetToken(1).SetString(vsFormatString("# %c",g->glyph).c_str());
			fontData.Record(&r);

			r.SetLabel("Bounds");	// geometry bounds, normalised by baseline
			r.SetRect(0.f, 0.f, actualWidth / baseline, actualHeight / baseline);
			fontData.Record(&r);

			r.SetLabel("Texels");	// texture bounds
			r.SetRect(x / width, y / height, actualWidth / width, actualHeight / height);
			fontData.Record(&r);

			r.SetLabel("Origin");	// origin where we're going to be drawing from when we draw this texture later
			r.SetTokenCount(2);
			r.GetToken(0).SetFloat( -g->minx / baseline );	// here, just specify where the origin is relative to our texture rect's left
			r.GetToken(1).SetFloat( g->maxy / baseline );	// store where the baseline is, relative to our texture rect's top
			fontData.Record(&r);

			r.SetLabel("Kern");	// origin where we're going to be drawing from when we draw this texture later
			r.SetFloat( g->advance / baseline );
			fontData.Record(&r);

			x += actualWidth + 2;

			//SDL_SaveBMP( gcache[i].surface, vsFile::GetFullFilename(name + ".bmp").c_str() );
			//vsString filename = vsFile::GetFullFilename(vsFormatString("%c.bmp", gcache[i].glyph).c_str());
			SDL_FreeSurface( gcache[i].surface );
		}

		SDL_SaveBMP( fontPage, vsFile::GetFullFilename(name + ".bmp").c_str() );

		SDL_FreeSurface( fontPage );

		vsDeleteArray(gcache);
	}

	TTF_Quit();
}

void
vsFontMaker::HandleTTFError( const vsString &context )
{
	vsLog("%s: %s", context.c_str(), TTF_GetError());
}
#endif // _DEBUG

#endif // TARGET_OS_IPHONE
#endif // 0
