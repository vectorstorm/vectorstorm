/*
 *  VS_SingleFloatImage.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 29/08/2018
 *  Copyright 2018 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_SingleFloatImage.h"

#include "VS_Color.h"
#include "VS_Texture.h"
#include "VS_TextureManager.h"
#include "VS_TextureInternal.h"
#include "VS_RenderTarget.h"

#include "VS_File.h"
#include "VS_Store.h"
#include "VS_Image.h"

#include "stb_image.h"
// #include <png.h>
#include "VS_OpenGL.h"

int vsSingleFloatImage::m_textureMakerCount = 0;

vsSingleFloatImage::vsSingleFloatImage():
	m_pixel(nullptr),
	m_pixelCount(0),
	m_width(0),
	m_height(0),
	m_pbo(0),
	m_sync(0)
{
}

vsSingleFloatImage::vsSingleFloatImage(unsigned int width, unsigned int height):
	m_pixel(nullptr),
	m_pixelCount(0),
	m_width(width),
	m_height(height),
	m_pbo(0),
	m_sync(0)
{
	m_pixelCount = width * height;

	m_pixel = new float[m_pixelCount];
}

vsSingleFloatImage::vsSingleFloatImage( const vsString &filename ):
	m_pbo(0),
	m_sync(0)
{
	vsFile img(filename, vsFile::MODE_Read);
	vsStore *s = new vsStore( img.GetLength() );
	img.Store(s);

	// [TODO] handle loading actual floats from HDR formats.

	int w,h,n;
	unsigned char* data = stbi_load_from_memory( (uint8_t*)s->GetReadHead(), s->BytesLeftForReading(), &w, &h, &n, STBI_grey );
	if ( !data )
		vsLog( "Failure: %s", stbi_failure_reason() );

	vsDelete(s);

	m_width = w;
	m_height = h;

	m_pixelCount = m_width*m_height;
	m_pixel = new float[m_pixelCount];

	for ( unsigned int v = 0; v < m_height; v++ )
	{
		for ( unsigned int u = 0; u < m_width; u++ )
		{
			int ind = PixelIndex(u,v);
			m_pixel[ind] = ((uint8_t*)(data))[ind] / 255.f;
		}
	}

	stbi_image_free(data);
}

vsSingleFloatImage::vsSingleFloatImage( vsTexture * texture ):
	m_pixel(nullptr),
	m_width(0),
	m_height(0),
	m_pbo(0),
	m_sync(0)
{
	Read(texture);
}

vsSingleFloatImage::~vsSingleFloatImage()
{
	if ( m_pbo != 0 )
	{
		if ( m_pixel )
			AsyncUnmap();

		glDeleteBuffers( 1, (GLuint*)&m_pbo );
		glDeleteSync( m_sync );

		vsAssert( m_pixel == nullptr, "async-mapped pixel data not cleared during destruction??" );
		m_pbo = 0;
		m_sync = 0;
	}

	vsDeleteArray( m_pixel );
}

void
vsSingleFloatImage::Read( vsTexture *texture )
{
	// GL_CHECK_SCOPED("vsSingleFloatImage");

	if ( m_width != (unsigned int)texture->GetResource()->GetWidth() ||
			m_height != (unsigned int)texture->GetResource()->GetHeight() )
	{
		vsDeleteArray(m_pixel);

		m_width = texture->GetResource()->GetWidth();
		m_height = texture->GetResource()->GetHeight();

		m_pixelCount = m_width * m_height;
		m_pixel = new float[m_pixelCount];
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
		glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, m_pixel);
	}
	else
	{
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, m_pixel);
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
}

void
vsSingleFloatImage::PrepForAsyncRead( vsTexture *texture )
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
		int bytes = width * height * sizeof(float);
		glBufferData( GL_PIXEL_PACK_BUFFER, bytes, nullptr, GL_DYNAMIC_READ );
	}

	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
}

void
vsSingleFloatImage::AsyncRead( vsTexture *texture )
{
	PrepForAsyncRead( texture );
	if ( m_sync != 0 )
		glDeleteSync( m_sync );

	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);

	// GL_CHECK("BindBuffer");
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, texture->GetResource()->GetTexture() );
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, 0);
	glBindTexture( GL_TEXTURE_2D, 0 );
	GL_CHECK("ReadPixels");
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);

	m_sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	// GL_CHECK("FenceSync");
}

void
vsSingleFloatImage::AsyncReadRenderTarget(vsRenderTarget *target, int buffer)
{
	PrepForAsyncRead( target->GetTexture(0) );
	if ( m_sync != 0 )
		glDeleteSync( m_sync );

	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);

	size_t width = target->GetWidth();
	size_t height = target->GetHeight();

	target->Bind();
	glReadBuffer(GL_COLOR_ATTACHMENT0+buffer);
	glReadPixels(0,0,width,height, GL_RED, GL_FLOAT, 0);

	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);

	m_sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
}

bool
vsSingleFloatImage::AsyncReadIsReady()
{
	// GL_CHECK_SCOPED("AsyncReadIsReady");
	if ( glClientWaitSync( m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0 ) != GL_TIMEOUT_EXPIRED )
	{
		return true;
	}
	return false;
}

void
vsSingleFloatImage::AsyncMap()
{
	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	m_pixel = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	m_pixelCount = m_width * m_height;
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
}

void
vsSingleFloatImage::AsyncUnmap()
{
	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
	m_pixel = nullptr;
}

float
vsSingleFloatImage::GetPixel(unsigned int u, unsigned int v) const
{
	vsAssert(u >= 0 && u < m_width && v >= 0 && v < m_height, "Texel out of bounds!");
	return m_pixel[ PixelIndex(u,v) ];
}

void
vsSingleFloatImage::SetPixel(unsigned int u, unsigned int v, float c)
{
	vsAssert(u >= 0 && u < m_width && v >= 0 && v < m_height, "Texel out of bounds!");
	m_pixel[ PixelIndex(u,v) ] = c;
}

void
vsSingleFloatImage::Clear( float clearValue )
{
	for ( int i = 0; i < m_pixelCount; i++ )
	{
		m_pixel[i] = clearValue;
	}
}

void
vsSingleFloatImage::CopyTo( vsSingleFloatImage *other )
{
	// in CopyTo, we've already validated that 'other' can contain our data.
	int bytes = m_width * m_height * sizeof(float);
	if ( m_pbo )
	{
		glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
		void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		memcpy(other->m_pixel, ptr, bytes);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		// glGetBufferSubData( GL_PIXEL_PACK_BUFFER, 0, bytes, other->m_pixel );
		glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
	}
	else
	{
		memcpy( other->m_pixel, m_pixel, bytes );
	}
}


void
vsSingleFloatImage::Copy( vsSingleFloatImage *other )
{
	if ( m_width != (unsigned int)other->GetWidth() ||
			m_height != (unsigned int)other->GetHeight() )
	{
		vsDeleteArray(m_pixel);

		m_width = other->GetWidth();
		m_height = other->GetHeight();

		m_pixelCount = m_width * m_height;
		m_pixel = new float[m_pixelCount];
	}
	other->CopyTo(this);
}


// vsTexture *
// vsSingleFloatImage::Bake()
// {
// 	vsString name = vsFormatString("FloatMakerTexture%d", m_textureMakerCount++);
//
// 	vsTextureInternal *ti = new vsTextureInternal(name, this);
// 	vsTextureManager::Instance()->Add(ti);
//
// 	return new vsTexture(name);
// }

void
vsSingleFloatImage::LoadFromSurface( SDL_Surface *source )
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

	SDL_Surface *image = SDL_CreateSurface(
			w, h,
			SDL_PIXELFORMAT_RGBA8888
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
	m_pixel = new float[m_pixelCount];

	for ( int v = 0; v < h; v++ )
	{
		for ( int u = 0; u < w; u++ )
		{
			int i = v*image->pitch + u*4;
			int ri = i;
			// int gi = ri+1;
			// int bi = ri+2;
			// int ai = ri+3;

			unsigned char r = ((unsigned char*)image->pixels)[ri];

			// flip our image.  Our image is stored upside-down, relative to a standard SDL Surface.
			SetPixel(u,(w-1)-v, r / 255.f );
		}
	}

	SDL_DestroySurface(image); /* No longer needed */
#endif // TARGET_OS_IPHONE
}

void
vsSingleFloatImage::SavePNG(const vsString& filename) const
{
	vsFile file( filename, vsFile::MODE_Write );

	vsImage *dup = new vsImage( m_width, m_height );
	for ( size_t y = 0; y < m_height; y++ )
	{
		for ( size_t x = 0; x < m_width; x++ )
		{
			vsColor c = c_white * GetPixel(x,y);
			c.a = 1.0;
			dup->SetPixel(x,y,c);
		}
	}

	dup->SavePNG( filename );
	vsDelete(dup);
}

