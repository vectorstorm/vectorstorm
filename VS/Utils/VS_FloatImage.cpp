/*
 *  VS_FloatImage.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 24/12/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_FloatImage.h"

#include "VS_Color.h"
#include "VS_Texture.h"
#include "VS_TextureManager.h"
#include "VS_TextureInternal.h"

#include "VS_File.h"
#include "VS_Store.h"

#if !TARGET_OS_IPHONE
#include <SDL2/SDL_image.h>
#include <png.h>
#include "VS_OpenGL.h"

#ifndef _WIN32
#include <zlib.h>
#endif // _WIN32
#endif // TARGET_OS_IPHONE

int vsFloatImage::m_textureMakerCount = 0;

vsFloatImage::vsFloatImage(unsigned int width, unsigned int height):
	m_pixel(NULL),
	m_pixelCount(0),
	m_width(width),
	m_height(height)
{
	m_pixelCount = width * height;

	m_pixel = new vsColor[m_pixelCount];
}

vsFloatImage::vsFloatImage( const vsString &filename_in )
{
#if !TARGET_OS_IPHONE
	vsString filename = vsFile::GetFullFilename(filename_in);
	SDL_Surface *loadedImage = IMG_Load(filename.c_str());
	vsAssert(loadedImage != NULL, vsFormatString("Unable to load texture %s: %s", filename.c_str(), IMG_GetError()));
	LoadFromSurface(loadedImage);
	SDL_FreeSurface(loadedImage);
#endif
}

vsFloatImage::vsFloatImage( vsTexture * texture )
{
	GL_CHECK_SCOPED("vsFloatImage");
	m_width = texture->GetResource()->GetWidth();
	m_height = texture->GetResource()->GetHeight();

	m_pixelCount = m_width * m_height;
	m_pixel = new vsColor[m_pixelCount];

	bool depthTexture = texture->GetResource()->IsDepth();

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, texture->GetResource()->GetTexture() );
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	if ( depthTexture )
	{
		size_t imageSizeInFloats = size_t(m_width) * size_t(m_height);

		float* pixels = new float[imageSizeInFloats];

		glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
		glBindTexture( GL_TEXTURE_2D, 0 );

		for ( unsigned int y = 0; y < m_height; y++ )
		{
			int rowStart = y * m_width;

			for ( unsigned int x = 0; x < m_width; x++ )
			{
				int rInd = rowStart + (x);
				float rVal = pixels[rInd];

				SetPixel(x,y, vsColor(rVal, rVal, rVal, 1.0f) );
			}
		}
		vsDeleteArray( pixels );
	}
	else
	{
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, m_pixel);
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
}

vsFloatImage::~vsFloatImage()
{
	vsDeleteArray( m_pixel );
}

vsColor
vsFloatImage::GetPixel(unsigned int u, unsigned int v) const
{
	vsAssert(u >= 0 && u < m_width && v >= 0 && v < m_height, "Texel out of bounds!");
	return m_pixel[ PixelIndex(u,v) ];
}

void
vsFloatImage::SetPixel(unsigned int u, unsigned int v, const vsColor &c)
{
	vsAssert(u >= 0 && u < m_width && v >= 0 && v < m_height, "Texel out of bounds!");
	m_pixel[ PixelIndex(u,v) ] = c;
}

void
vsFloatImage::Clear( const vsColor &clearColor )
{
	for ( int i = 0; i < m_pixelCount; i++ )
	{
		m_pixel[i] = clearColor;
	}
}

vsTexture *
vsFloatImage::Bake()
{
	vsString name = vsFormatString("FloatMakerTexture%d", m_textureMakerCount++);

	vsTextureInternal *ti = new vsTextureInternal(name, this);
	vsTextureManager::Instance()->Add(ti);

	return new vsTexture(name);
}

void
vsFloatImage::LoadFromSurface( SDL_Surface *source )
{
#if !TARGET_OS_IPHONE
	//	SDL_Surface *screen = SDL_GetVideoSurface();
	SDL_Rect	area;

	SDL_BlendMode bm;
	SDL_GetSurfaceBlendMode(source, &bm);
	SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_NONE);

	m_width = source->w;
	m_height = source->h;

	int w = source->w;
	int h = source->h;

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


	/* Copy the surface into the GL-format texture image, to make loading easier */
	area.x = 0;
	area.y = 0;
	area.w = source->w;
	area.h = source->h;
	SDL_BlitSurface(source, &area, image, &area);

	SDL_SetSurfaceBlendMode(source, bm);

	// now lets copy our image data

	m_pixelCount = w*h;
	m_pixel = new vsColor[m_pixelCount];

	for ( int v = 0; v < h; v++ )
	{
		for ( int u = 0; u < w; u++ )
		{
			int i = v*image->pitch + u*4;
			int ri = i;
			int gi = ri+1;
			int bi = ri+2;
			int ai = ri+3;

			unsigned char r = ((unsigned char*)image->pixels)[ri];
			unsigned char g = ((unsigned char*)image->pixels)[gi];
			unsigned char b = ((unsigned char*)image->pixels)[bi];
			unsigned char a = ((unsigned char*)image->pixels)[ai];

			// flip our image.  Our image is stored upside-down, relative to a standard SDL Surface.
			SetPixel(u,(w-1)-v, vsColor( r / 255.f, g / 255.f, b / 255.f, a / 255.f ) );
		}
	}

	SDL_FreeSurface(image); /* No longer needed */
#endif // TARGET_OS_IPHONE
}

vsStore *
vsFloatImage::BakePNG(int compression)
{
	// first, create an SDL_Surface from our raw pixel data.
	SDL_Surface *image = SDL_CreateRGBSurface(
			SDL_SWSURFACE,
			m_width, m_height,
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
	int err = SDL_LockSurface( image );
	vsAssert(!err, "Couldn't lock surface??");
	vsAssert(image->format->BytesPerPixel == 4, "Didn't get a 4-byte surface??");
	for ( size_t v = 0; v < m_height; v++ )
	{
		for ( size_t u = 0; u < m_width; u++ )
		{
			int i = v*image->pitch + u*4;
			int ri = i;
			int gi = ri+1;
			int bi = ri+2;
			int ai = ri+3;

			// flip our image.  Our image is stored upside-down, relative to a standard SDL Surface.
			vsColor pixel = GetPixel(u,(m_height-1)-v);

			((unsigned char*)image->pixels)[ri] = (unsigned char)(255.f * pixel.r);
			((unsigned char*)image->pixels)[gi] = (unsigned char)(255.f * pixel.g);
			((unsigned char*)image->pixels)[bi] = (unsigned char)(255.f * pixel.b);
			((unsigned char*)image->pixels)[ai] = (unsigned char)(255.f * pixel.a);
		}
	}
	//
	// now, let's save out our surface.
	const int pngDataSize = 1024*1024*10;
	char* pngData = new char[pngDataSize];
	SDL_RWops *dst = SDL_RWFromMem(pngData, pngDataSize);
	vsLog( "%s", SDL_GetError() );
	int retval = IMG_SavePNG_RW(image,
			dst,
			false);
	SDL_UnlockSurface( image );
	if ( retval == -1 )
		vsLog( "%s", SDL_GetError() );
	int bytes = (int)SDL_RWtell(dst);
	vsStore *result = new vsStore(bytes);
	result->WriteBuffer(pngData,bytes);
	SDL_RWclose(dst);
	SDL_FreeSurface(image);
	delete [] pngData;

	return result;
}

void
vsFloatImage::SavePNG(int compression, const vsString& filename)
{
	vsStore *store = BakePNG(compression);
	vsFile file( filename, vsFile::MODE_Write );
	file.Store(store);
	vsDelete(store);
}

