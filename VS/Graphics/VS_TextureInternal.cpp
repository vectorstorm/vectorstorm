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
#include "VS_FloatImage.h"
#include "VS_Image.h"
#include "VS_RenderTarget.h"	// for vsSurface.  Should move into its own file.
#include "VS_RenderBuffer.h"

#include "VS/Files/VS_File.h"
#include "VS/Memory/VS_Store.h"

#include "VS_OpenGL.h"

#if TARGET_OS_IPHONE

#include "VS_TextureInternalIPhone.h"

vsTextureInternal::vsTextureInternal( const vsString &filename_in ):
	vsResource(filename_in),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_tbo(NULL)
{
	vsString filename = vsFile::GetFullFilename(filename_in);

	m_texture = ::loadTexture( filename.c_str() );
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsImage *maker ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_tbo(NULL)
{
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsSurface *surface, bool depth ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_tbo(NULL)
{
	if ( surface )
		m_texture = (depth) ? surface->m_depth : surface->m_texture;
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsRenderBuffer *buffer ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(false),
	m_tbo(buffer)
{
	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;
	m_nearestSampling = false;
}

vsTextureInternal::~vsTextureInternal()
{
	GLuint t = m_texture;
	glDeleteTextures(1, &t);
	m_texture = 0;

	vsDelete( m_tbo );
}



#else
#include <SDL2/SDL_image.h>

/* Quick utility function for texture creation */
void
vsTextureInternal::SetSurface( vsSurface* surface, int surfaceBuffer, bool depth )
{
	// to be used for deferred texture creation  (We went through the vsSurface
	// constructor with a NULL argument;  now we're providing the surface)

	m_texture = (depth) ? surface->m_depth : surface->m_texture[surfaceBuffer];

	m_glTextureWidth = surface->m_width;
	m_glTextureHeight = surface->m_height;
	m_width = surface->m_width;
	m_height = surface->m_height;

	if ( m_nearestSampling )
		SetNearestSampling();
}

vsTextureInternal::vsTextureInternal( const vsString &filename_in ):
	vsResource(filename_in),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_tbo(NULL)
{
	vsFile img(filename_in, vsFile::MODE_Read);
	vsStore *s = new vsStore( img.GetLength() );
	img.Store(s);

	SDL_RWops* rwops = SDL_RWFromMem( s->GetReadHead(), s->BytesLeftForReading() );
	SDL_Surface *loadedImage = IMG_Load_RW( rwops, true );

	vsDelete(s);

	vsAssert(loadedImage != NULL, vsFormatString("Unable to load texture %s: %s", filename_in, IMG_GetError()));
	ProcessSurface(loadedImage);
	SDL_FreeSurface(loadedImage);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_tbo(NULL)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	m_width = w;
	m_height = w;

	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;

	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA,
			w, h,
			0,
			GL_RGBA,
			GL_UNSIGNED_INT_8_8_8_8_REV,
			image->RawData());
	glGenerateMipmap(GL_TEXTURE_2D);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_tbo(NULL)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	m_width = w;
	m_height = w;

	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;

	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA32F,
			w, h,
			0,
			GL_RGBA,
			GL_FLOAT,
			image->RawData());
	glGenerateMipmap(GL_TEXTURE_2D);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsRenderBuffer *buffer ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(false),
	m_tbo(buffer)
{
	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, uint32_t glTextureId ):
	vsResource(name),
	m_texture(glTextureId),
	m_premultipliedAlpha(false),
	m_tbo(NULL)
{
	m_nearestSampling = false;
}


void
vsTextureInternal::Blit( vsImage *image, const vsVector2D &where)
{
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexSubImage2D(GL_TEXTURE_2D,
			0,
			(int)where.x, (int)where.y,
			image->GetWidth(), image->GetHeight(),
			GL_RGBA,
			GL_UNSIGNED_INT_8_8_8_8_REV,
			image->RawData());
	glGenerateMipmap(GL_TEXTURE_2D);
}

void
vsTextureInternal::Blit( vsFloatImage *image, const vsVector2D &where)
{
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexSubImage2D(GL_TEXTURE_2D,
			0,
			(int)where.x, (int)where.y,
			image->GetWidth(), image->GetHeight(),
			GL_RGBA,
			GL_FLOAT,
			image->RawData());
	glGenerateMipmap(GL_TEXTURE_2D);
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsSurface *surface, int surfaceBuffer, bool depth ):
	vsResource(name),
	m_texture(0),
	m_glTextureWidth(0),
	m_glTextureHeight(0),
	m_width(0),
	m_height(0),
	m_depth(depth),
	m_premultipliedAlpha(true),
	m_tbo(NULL)
{
	if ( surface )
	{
		SetSurface(surface, surfaceBuffer, depth);
	}
	m_nearestSampling = false;
}

void
vsTextureInternal::ProcessSurface( SDL_Surface *source )
{
	//	SDL_Surface *screen = SDL_GetVideoSurface();
	SDL_Rect	area;

	//SDL_SetAlpha(source, 0, SDL_ALPHA_OPAQUE);

	m_width = source->w;
	m_height = source->h;

	int w = source->w;//vsNextPowerOfTwo(source->w);
	int h = source->h;//vsNextPowerOfTwo(source->h);

	m_glTextureWidth = w;
	m_glTextureHeight = h;

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

	SDL_BlendMode bm;
	SDL_GetSurfaceBlendMode(source, &bm);
	if ( bm != SDL_BLENDMODE_NONE )
	{
		SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_NONE);
	}

	/* Copy the surface into the GL texture image */
	area.x = 0;
	area.y = 0;
	area.w = source->w;
	area.h = source->h;
	SDL_BlitSurface(source, &area, image, &area);

	/* Restore the alpha blending attributes */
	if ( bm != SDL_BLENDMODE_NONE )
	{
		SDL_SetSurfaceBlendMode(source, bm);
	}

	/* Create an OpenGL texture for the image */
	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;

	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA,
			w, h,
			0,
			GL_RGBA,
			GL_UNSIGNED_INT_8_8_8_8_REV,
			image->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

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
	GLuint t = m_texture;
	glDeleteTextures(1, &t);
	m_texture = 0;


	vsDelete( m_tbo );
}

void
vsTextureInternal::SetNearestSampling()
{
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	m_nearestSampling = true;
}

void
vsTextureInternal::SetLinearSampling(bool linearMipmaps)
{
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if ( linearMipmaps )
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_nearestSampling = false;
}

#endif // TARGET_OS_IPHONE

uint32_t
vsTextureInternal::ScaleColour(uint32_t ini, float amt)
{
	char r = (ini & 0x000000FF);
	char g = (ini & 0x0000FF00) >> 8;
	char b = (ini & 0x00FF0000) >> 16;
	char a = (ini & 0xFF000000) >> 24;

	r = (char)(r * amt);
	g = (char)(g * amt);
	b = (char)(b * amt);
	a = (char)(a * amt);

	uint32_t result = (r & 0x000000FF) |
		((g << 8) & 0x0000FF00) |
		((b << 16) & 0x00FF0000) |
		((a << 24) & 0xFF000000);

	return result;

}

uint32_t
vsTextureInternal::SafeAddColour(uint32_t acolour, uint32_t bcolour)
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

	uint32_t result = (r & 0x000000FF) |
		((g << 8) & 0x0000FF00) |
		((b << 16) & 0x00FF0000) |
		((a << 24) & 0xFF000000);

	return result;

}

void
vsTextureInternal::ClampUV( bool u, bool v )
{
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, u ? GL_CLAMP_TO_EDGE : GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, v ? GL_CLAMP_TO_EDGE : GL_REPEAT );
}

