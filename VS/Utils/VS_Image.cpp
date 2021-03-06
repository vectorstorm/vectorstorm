/*
 *  VS_Image.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 01-02-2012.
 *  Copyright 2012 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Image.h"

#include "VS_Color.h"
#include "VS_Texture.h"
#include "VS_TextureManager.h"
#include "VS_TextureInternal.h"
#include "VS_RenderTarget.h"

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

int vsImage::s_textureMakerCount = 0;
bool vsImage::s_allowLoadFailure = false;

vsImage::vsImage():
	m_pixel(NULL),
	m_pixelCount(0),
	m_width(0),
	m_height(0),
	m_pbo(0),
	m_sync(0)
{
}

vsImage::vsImage(unsigned int width, unsigned int height):
	m_pixel(NULL),
	m_pixelCount(0),
	m_width(width),
	m_height(height),
	m_pbo(0),
	m_sync(0)
{
	m_pixelCount = width * height;

	m_pixel = new uint32_t[m_pixelCount];
	memset(m_pixel,0,sizeof(uint32_t)*m_pixelCount);
}

vsImage::vsImage( const vsString &filename ):
	m_pixel(NULL),
	m_pbo(0),
	m_sync(0)
{
#if !TARGET_OS_IPHONE
	vsFile img(filename, vsFile::MODE_Read);
	vsStore *s = new vsStore( img.GetLength() );
	img.Store(s);

	vsCheck( img.GetLength() > 0, "Zero-length file??" );

	SDL_RWops* rwops = SDL_RWFromMem( s->GetReadHead(), s->BytesLeftForReading() );
	SDL_Surface *loadedImage = IMG_Load_RW( rwops, true );

	vsDelete(s);
	if ( !loadedImage && s_allowLoadFailure )
	{
		vsCheckF(loadedImage != NULL, "Unable to load texture %s: %s", filename.c_str(), IMG_GetError());

		m_pixel = new uint32_t[1];
		m_width = m_height = 1;
		SetPixel(0, 0, vsColor(1.f,0.f,1.f,1.f)); // magenta for missing texture
	}
	else
	{
		vsAssertF(loadedImage != NULL, "Unable to load texture %s: %s", filename.c_str(), IMG_GetError());
	}

	if ( loadedImage )
	{
		LoadFromSurface(loadedImage);
		SDL_FreeSurface(loadedImage);
	}
#endif
}

vsImage::vsImage( const vsStore &filedata ):
	m_pixel(NULL),
	m_pbo(0),
	m_sync(0)
{
#if !TARGET_OS_IPHONE
	vsCheck( filedata.Length() > 0, "Zero-length file??" );

	SDL_RWops* rwops = SDL_RWFromMem( filedata.GetReadHead(), filedata.BytesLeftForReading() );
	SDL_Surface *loadedImage = IMG_Load_RW( rwops, true );

	if ( !loadedImage )
	{
		vsCheckF(loadedImage != NULL, "Unable to load texture from filedata: %s", IMG_GetError());

		m_pixel = new uint32_t[1];
		m_width = m_height = 1;
		SetPixel(0, 0, vsColor(1.f,0.f,1.f,1.f)); // magenta for missing texture
	}

	if ( loadedImage )
	{
		LoadFromSurface(loadedImage);
		SDL_FreeSurface(loadedImage);
	}
#endif
}

vsImage::vsImage( vsTexture * texture ):
	m_pixel(NULL),
	m_width(0),
	m_height(0),
	m_pbo(0),
	m_sync(0)
{
	Read(texture);
}

vsImage::~vsImage()
{
	if ( m_pbo != 0 )
	{
		if ( m_pixel )
			AsyncUnmap();

		glDeleteBuffers( 1, (GLuint*)&m_pbo );
		glDeleteSync( m_sync );

		vsAssert( m_pixel == NULL, "async-mapped pixel data not cleared during destruction??" );
		m_pbo = 0;
		m_sync = 0;
	}

	vsDeleteArray( m_pixel );
}

void
vsImage::Read( vsTexture *texture )
{
	// GL_CHECK_SCOPED("vsImage");

	if ( m_width != (unsigned int)texture->GetResource()->GetWidth() ||
			m_height != (unsigned int)texture->GetResource()->GetHeight() )
	{
		vsDeleteArray(m_pixel);

		m_width = texture->GetResource()->GetWidth();
		m_height = texture->GetResource()->GetHeight();

		m_pixelCount = m_width * m_height;
		m_pixel = new uint32_t[m_pixelCount];
	}

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

				SetPixel(x, (m_height-1)-y, vsColor(rVal, rVal, rVal, 1.0f) );
			}
		}
		vsDeleteArray( pixels );
	}
	else
	{
		size_t imageSize = size_t(m_width) * size_t(m_height);
		uint32_t* pixels = new uint32_t[imageSize];
		// TODO:  THis would be faster if it was BGRA.
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glBindTexture( GL_TEXTURE_2D, 0 );

		for ( unsigned int y = 0; y < m_height; y++ )
		{
			int rowStart = y * m_width;

			for ( unsigned int x = 0; x < m_width; x++ )
			{
				int rInd = rowStart + (x);
				uint32_t pixel = pixels[rInd];

				SetRawPixel(x, (m_height-1)-y, pixel);
			}
		}
		vsDeleteArray( pixels );
	}
}

void
vsImage::PrepForAsyncRead( vsTexture *texture )
{
	if ( m_pbo == 0 )
		glGenBuffers(1, &m_pbo);

	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	size_t width = texture->GetResource()->GetWidth();
	size_t height = texture->GetResource()->GetHeight();
	if ( width != m_width || height != m_height )
	{
		m_width = width;
		m_height = height;
		int bytes = width * height * sizeof(uint32_t);
		glBufferData( GL_PIXEL_PACK_BUFFER, bytes, NULL, GL_DYNAMIC_READ );
	}

	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
}

void
vsImage::AsyncRead( vsTexture *texture )
{
	PrepForAsyncRead(texture);

	if ( m_sync != 0 )
		glDeleteSync( m_sync );

	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, texture->GetResource()->GetTexture() );
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture( GL_TEXTURE_2D, 0 );

	// GL_CHECK("glGetTexImage");
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);

	m_sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
}

void
vsImage::AsyncReadRenderTarget(vsRenderTarget *target, int buffer)
{
	GL_CHECK_SCOPED("AsyncReadRenderTarget");
	PrepForAsyncRead(target->Resolve(0));
	GL_CHECK("Prepped");

	if ( m_sync != 0 )
		glDeleteSync( m_sync );
	GL_CHECK("Deleted Sync");

	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	GL_CHECK("BindBuffer");

	target->Bind();
	int width = target->GetWidth();
	int height = target->GetHeight();
	glReadBuffer(GL_COLOR_ATTACHMENT0+buffer);
	GL_CHECK("glReadBuffer");
	glReadPixels(0,0,width,height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	GL_CHECK("glReadPixels");

	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
	GL_CHECK("glUnbindBuffer");

	m_sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	GL_CHECK("glFenceSync");
}

bool
vsImage::AsyncReadIsReady()
{
	// GL_CHECK_SCOPED("AsyncReadIsReady");
	if ( glClientWaitSync( m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0 ) != GL_TIMEOUT_EXPIRED )
	{
		return true;
	}
	return false;
}

void
vsImage::AsyncMap()
{
	vsAssert( m_pixel == NULL, "Non-null during pbo async mapping");
	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	m_pixel = (uint32_t*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	m_pixelCount = m_width * m_height;
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
}

void
vsImage::AsyncUnmap()
{
	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
	m_pixel = NULL;
}

uint32_t
vsImage::GetRawPixel(unsigned int u, unsigned int v) const
{
	vsAssert(u >= 0 && u < m_width && v >= 0 && v < m_height, "Texel out of bounds!");
	return m_pixel[ PixelIndex(u,v) ];
}

void
vsImage::SetRawPixel(unsigned int u, unsigned int v, uint32_t c)
{
	vsAssert(u >= 0 && u < m_width && v >= 0 && v < m_height, "Texel out of bounds!");
	m_pixel[ PixelIndex(u,v) ] = c;
}

vsColor
vsImage::GetPixel(unsigned int u, unsigned int v) const
{
	return vsColor::FromUInt32(GetRawPixel(u,v));
}

void
vsImage::SetPixel(unsigned int u, unsigned int v, const vsColor &c)
{
	SetRawPixel(u,v,c.AsUInt32());
}

void
vsImage::Clear( const vsColor &clearColor )
{
	uint32_t cc = clearColor.AsUInt32();
	// TODO:  This should really be being done via memset.
	for ( int i = 0; i < m_pixelCount; i++ )
	{
		m_pixel[i] = cc;
	}
}

void
vsImage::CopyTo( vsImage *other )
{
	// in CopyTo, we've already validated that 'other' can contain our data.
	int bytes = m_width * m_height * sizeof(uint32_t);
	if ( m_pbo )
	{
		glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
		// glGetBufferSubData( GL_PIXEL_PACK_BUFFER, 0, bytes, other->m_pixel );
		void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		memcpy(other->m_pixel, ptr, bytes);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
	}
	else
	{
		memcpy( other->m_pixel, m_pixel, bytes );
	}
}

void
vsImage::Copy( vsImage *other )
{
	if ( m_width != (unsigned int)other->GetWidth() ||
			m_height != (unsigned int)other->GetHeight() )
	{
		vsDeleteArray(m_pixel);

		m_width = other->GetWidth();
		m_height = other->GetHeight();

		m_pixelCount = m_width * m_height;
		m_pixel = new uint32_t[m_pixelCount];
	}
	other->CopyTo(this);
}

vsTexture *
vsImage::Bake( const vsString& name_in )
{
	vsString name(name_in);
	if ( name.empty() )
		name = vsFormatString("MakerTexture%d", s_textureMakerCount++);

	vsTextureInternal *ti = new vsTextureInternal(name, this);
	vsTextureManager::Instance()->Add(ti);

	return new vsTexture(name);
}

void
vsImage::LoadFromSurface( SDL_Surface *source )
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

	vsAssert( m_pixel == NULL, "Non-null pixel storage during LoadFromSurface" );
	m_pixelCount = w*h;
	m_pixel = new uint32_t[m_pixelCount];

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

			uint32_t pixel;
			uint8_t *cp = reinterpret_cast<uint8_t*>(&pixel);
			cp[0] = r;
			cp[1] = g;
			cp[2] = b;
			cp[3] = a;

			SetRawPixel( u, v, pixel );
		}
	}

	SDL_FreeSurface(image); /* No longer needed */
#endif // TARGET_OS_IPHONE
}

vsStore *
vsImage::BakePNG(int compression)
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

			uint32_t pixel = GetRawPixel(u,v);
			uint8_t *cp = reinterpret_cast<uint8_t*>(&pixel);

			((unsigned char*)image->pixels)[ri] = cp[0];
			((unsigned char*)image->pixels)[gi] = cp[1];
			((unsigned char*)image->pixels)[bi] = cp[2];
			((unsigned char*)image->pixels)[ai] = cp[3];
		}
	}
	//
	// now, let's save out our surface.
	// raw image size will be width*height*4 bytes.  So let's allocate that much
	// space to start with;  our PNG should be no bigger than that!.
	const int pngDataSize = m_width*m_height*4;
	char* pngData = new char[pngDataSize];
	SDL_RWops *dst = SDL_RWFromMem(pngData, pngDataSize);
	if ( !dst )
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

vsStore *
vsImage::BakeJPG(int quality)
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
			uint32_t pixel = GetRawPixel(u,(m_height-1)-v);
			uint8_t *cp = reinterpret_cast<uint8_t*>(&pixel);

			((unsigned char*)image->pixels)[ri] = cp[0];
			((unsigned char*)image->pixels)[gi] = cp[1];
			((unsigned char*)image->pixels)[bi] = cp[2];
			((unsigned char*)image->pixels)[ai] = cp[3];
		}
	}
	//
	// now, let's save out our surface.
	const int pngDataSize = 1024*1024*10;
	char* pngData = new char[pngDataSize];
	SDL_RWops *dst = SDL_RWFromMem(pngData, pngDataSize);
	if ( !dst )
		vsLog( "%s", SDL_GetError() );
	int retval = IMG_SaveJPG_RW(image,
			dst,
			false,
			quality);
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
vsImage::SaveJPG(int quality, const vsString& filename)
{
	vsStore *store = BakeJPG(quality);
	vsFile file( filename, vsFile::MODE_Write );
	file.Store(store);
	vsDelete(store);
}

void
vsImage::SavePNG(int compression, const vsString& filename)
{
	vsStore *store = BakePNG(compression);
	vsFile file( filename, vsFile::MODE_Write );
	file.Store(store);
	vsDelete(store);
}

void
vsImage::SavePNG_FullAlpha(int compression, const vsString& filename)
{
	vsImage dup( m_width, m_height );
	for ( size_t y = 0; y < m_height; y++ )
	{
		for ( size_t x = 0; x < m_width; x++ )
		{
			vsColor c = GetPixel(x,y);
			c.a = 1.0;
			dup.SetPixel(x,y,c);
		}
	}
	dup.SavePNG(compression, filename);
}

