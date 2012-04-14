/*
 *  VS_TextureInternal.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_TextureInternal.h"

#include "VS_Color.h"
#include "VS_Image.h"
#include "VS_RenderTarget.h"	// for vsSurface.  Should move into its own file.

#include "VS/Files/VS_File.h"

#include "VS_OpenGL.h"

#if TARGET_OS_IPHONE

#include "VS_TextureInternalIPhone.h"

/* Quick utility function for texture creation */
static int power_of_two(int input)
{
	int value = 1;

	while ( value < input ) {
		value <<= 1;
	}
	return value;
}

vsTextureInternal::vsTextureInternal( const vsString &filename_in ):
vsResource(filename_in),
m_texture(0),
m_refCount(0),
m_premultipliedAlpha(true)
{
	vsString filename = vsFile::GetFullFilename(filename_in);

	m_texture = ::loadTexture( filename.c_str() );
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsImage *maker ):
vsResource(name),
m_texture(0),
m_refCount(0),
m_premultipliedAlpha(true)
{
}


vsTextureInternal::vsTextureInternal( const vsString &name, vsSurface *surface ):
vsResource(name),
m_texture(surface->m_texture),
m_refCount(0),
m_premultipliedAlpha(true)
{
}

vsTextureInternal::~vsTextureInternal()
{
	vsAssert(m_refCount==0, "Error:  Not all references removed from texture!");
	GLuint t = m_texture;
	glDeleteTextures(1, &t);
	m_texture = 0;
}


#else
#if __APPLE_CC__
#include <SDL_image/SDL_image.h>
#else
#include "SDL_image.h"
#endif

/* Quick utility function for texture creation */
static int power_of_two(int input)
{
	int value = 1;

	while ( value < input ) {
		value <<= 1;
	}
	return value;
}

vsTextureInternal::vsTextureInternal( const vsString &filename_in ):
vsResource(filename_in),
m_texture(0),
m_refCount(0),
m_premultipliedAlpha(false)
{
	vsString filename = vsFile::GetFullFilename(filename_in);

	SDL_Surface *loadedImage = IMG_Load(filename.c_str());
	vsAssert(loadedImage != NULL, vsFormatString("Unable to load texture %s: %s", filename.c_str(), IMG_GetError()));
	ProcessSurface(loadedImage);
	SDL_FreeSurface(loadedImage);
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsImage *image ):
vsResource(name),
m_texture(0),
m_refCount(0),
m_premultipliedAlpha(false)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	SDL_Surface *surface = SDL_CreateRGBSurface(
											  SDL_SWSURFACE,
											  w, h,
											  32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
											  0x000000FF,
											  0x0000FF00,
											  0x00FF0000,
											  0xFF000000
#else
											  0xFF000000,
											  0x00FF0000,
											  0x0000FF00,
											  0x000000FF
#endif
											  );
	vsAssert(surface, "Error??");
	int err = SDL_LockSurface( surface );
	vsAssert(!err, "Couldn't lock surface??");
	vsAssert(surface->format->BytesPerPixel == 4, "Didn't get a 4-byte surface??");
	for ( int v = 0; v < h; v++ )
	{
		for ( int u = 0; u < w; u++ )
		{
			vsColor t = image->Pixel(u,v);
			char *pixel = (char *)surface->pixels;
			pixel += (v*surface->pitch);
			pixel += (u*surface->format->BytesPerPixel);

			uint32 color = SDL_MapRGBA( surface->format , (uint8)(t.r * 255) , (uint8)(t.g * 255) , (uint8)(t.b * 255), (uint8)(t.a * 255) );
			memcpy( pixel, &color, sizeof(color) );

		}
	}
	SDL_UnlockSurface( surface );

	ProcessSurface(surface);

	SDL_FreeSurface(surface);
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsSurface *surface ):
vsResource(name),
m_texture(surface->m_texture),
m_glTextureWidth(surface->m_width),
m_glTextureHeight(surface->m_height),
m_width(surface->m_width),
m_height(surface->m_height),
m_refCount(0),
m_premultipliedAlpha(true)
{
}

void
vsTextureInternal::ProcessSurface( SDL_Surface *source )
{
	//	SDL_Surface *screen = SDL_GetVideoSurface();
	SDL_Rect	area;
    uint32 saved_flags;
    Uint8  saved_alpha;

	//SDL_SetAlpha(source, 0, SDL_ALPHA_OPAQUE);

	m_width = (float)source->w;
	m_height = (float)source->h;

	int w = power_of_two(source->w);
	int h = power_of_two(source->h);

	m_glTextureWidth = (float)w;
	m_glTextureHeight = (float)h;

	SDL_Surface *image = SDL_CreateRGBSurface(
											  SDL_SWSURFACE,
											  w, h,
											  32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
											  0x000000FF,
											  0x0000FF00,
											  0x00FF0000,
											  0xFF000000
#else
											  0xFF000000,
											  0x00FF0000,
											  0x0000FF00,
											  0x000000FF
#endif
											  );
	vsAssert(image, "Error??");

    saved_flags = source->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
    saved_alpha = source->format->alpha;
    if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
        SDL_SetAlpha(source, 0, SDL_ALPHA_OPAQUE);
    }

    /* Copy the surface into the GL texture image */
    area.x = 0;
    area.y = 0;
    area.w = source->w;
    area.h = source->h;
    SDL_BlitSurface(source, &area, image, &area);

    /* Restore the alpha blending attributes */
    if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
        SDL_SetAlpha(source, saved_flags, saved_alpha);
    }

	/* Create an OpenGL texture for the image */
	GLuint t;
    glGenTextures(1, &t);
	m_texture = t;

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 w, h,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_INT_8_8_8_8_REV,
				 image->pixels);

#if 0
/*	if ( 0 )
	{
		int wid = w;
		int hei = h;
		int mapMapId = 1;

		while ( wid > 32 || hei > 32 )
		{
			wid = wid >> 1;
			hei = hei >> 1;
			glTexImage2D(GL_TEXTURE_2D, mapMapId++, GL_RGBA, wid, hei, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		}
		glGenerateMipmapEXT(GL_TEXTURE_2D);
	}*/


	// now lets read it back.
#endif // 0

	bool doIt = false;

	if ( doIt )
	{
		unsigned char *imageData = new unsigned char[w*h*4];
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, imageData);

		for ( int i = 0; i < w*h; i++ )
		{
			int ri = i*4;
			int gi = ri+1;
			int bi = ri+2;
			int ai = ri+3;

			unsigned char r = imageData[ri];
			unsigned char g = imageData[gi];
			unsigned char b = imageData[bi];
			unsigned char a = imageData[ai];

			unsigned char origr = ((unsigned char*)image->pixels)[ri];
			unsigned char og = ((unsigned char*)image->pixels)[gi];
			unsigned char ob = ((unsigned char*)image->pixels)[bi];
			unsigned char oa = ((unsigned char*)image->pixels)[ai];

			if ( r != origr || g != og || b != ob || a != oa )
			{
				vsLog("Pixel %d,%d,%d,%d != %d,%d,%d,%d", r,g,b,a,origr,og,ob,oa);
			}
		}
		delete [] imageData;
	}
//#endif //0
	SDL_FreeSurface(image); /* No longer needed */

}

vsTextureInternal::~vsTextureInternal()
{
	vsAssert(m_refCount==0, "Error:  Not all references removed from texture!");
	GLuint t = m_texture;
	glDeleteTextures(1, &t);
	m_texture = 0;

}

#endif // TARGET_OS_IPHONE

uint32
vsTextureInternal::ScaleColour(uint32 ini, float amt)
{
	char r = (ini & 0x000000FF);
	char g = (ini & 0x0000FF00) >> 8;
	char b = (ini & 0x00FF0000) >> 16;
	char a = (ini & 0xFF000000) >> 24;

	r = (char)(r * amt);
	g = (char)(g * amt);
	b = (char)(b * amt);
	a = (char)(a * amt);

	uint32 result = (r & 0x000000FF) |
	((g << 8) & 0x0000FF00) |
	((b << 16) & 0x00FF0000) |
	((a << 24) & 0xFF000000);

	return result;

}

uint32
vsTextureInternal::SafeAddColour(uint32 acolour, uint32 bcolour)
{
	int ra = (acolour & 0x000000FF);
	int ga = (acolour & 0x0000FF00) >> 8;
	int ba = (acolour & 0x00FF0000) >> 16;
	int aa = (acolour & 0xFF000000) >> 24;

	int rb = (bcolour & 0x000000FF);
	int gb = (bcolour & 0x0000FF00) >> 8;
	int bb = (bcolour & 0x00FF0000) >> 16;
	int ab = (bcolour & 0xFF000000) >> 24;

	int r = vsMin( ra + rb, 0xFF );
	int g = vsMin( ga + gb, 0xFF );
	int b = vsMin( ba + bb, 0xFF );
	int a = vsMin( aa + ab, 0xFF );

	uint32 result = (r & 0x000000FF) |
	((g << 8) & 0x0000FF00) |
	((b << 16) & 0x00FF0000) |
	((a << 24) & 0xFF000000);

	return result;

}
