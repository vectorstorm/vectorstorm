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
#include "VS_RenderTarget.h"

#include "VS_File.h"
#include "VS_Store.h"

#include "stb_image.h"
#include "VS_OpenGL.h"
#include "VS_Image.h"
#include <atomic>

namespace
{
	static std::atomic<int> s_textureMakerCount = 0;
};

vsFloatImage::vsFloatImage(unsigned int width, unsigned int height):
	m_pixel(nullptr),
	m_pixelCount(0),
	m_width(width),
	m_height(height),
	m_pbo(0),
	m_sync(0)
{
	m_pixelCount = width * height;

	m_pixel = new vsColor[m_pixelCount];
}

vsFloatImage::vsFloatImage( const vsString &filename ):
	m_pbo(0),
	m_sync(0)
{
	vsFile img(filename, vsFile::MODE_Read);
	vsStore *s = new vsStore( img.GetLength() );
	img.Store(s);

	// [TODO] handle loading actual floats from HDR formats.

	int w,h,n;
	unsigned char* data = stbi_load_from_memory( (uint8_t*)s->GetReadHead(), s->BytesLeftForReading(), &w, &h, &n, STBI_rgb_alpha );
	if ( !data )
		vsLog( "Failure: %s", stbi_failure_reason() );

	vsDelete(s);

	m_width = w;
	m_height = h;

	m_pixelCount = m_width*m_height;
	m_pixel = new vsColor[m_pixelCount];

	for ( unsigned int v = 0; v < m_height; v++ )
	{
		for ( unsigned int u = 0; u < m_width; u++ )
		{
			int ind = PixelIndex(u,v);
			uint32_t raw = ((uint32_t*)(data))[ind];
			SetPixel(u,v, vsColor::FromUInt32(raw) );
		}
	}

	stbi_image_free(data);
}

vsFloatImage::vsFloatImage( vsTexture * texture ):
	m_pixel(nullptr),
	m_width(0),
	m_height(0),
	m_pbo(0),
	m_sync(0)
{
	Read(texture);
}

vsFloatImage::~vsFloatImage()
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
	vsDeleteArray(m_pixel);
}

void
vsFloatImage::Read( vsTexture *texture )
{
	// GL_CHECK_SCOPED("vsFloatImage");

	if ( m_width != (unsigned int)texture->GetResource()->GetWidth() ||
			m_height != (unsigned int)texture->GetResource()->GetHeight() )
	{
		vsDeleteArray(m_pixel);

		m_width = texture->GetResource()->GetWidth();
		m_height = texture->GetResource()->GetHeight();

		m_pixelCount = m_width * m_height;
		m_pixel = new vsColor[m_pixelCount];
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

void
vsFloatImage::AsyncRead( vsTexture *texture )
{
	// GL_CHECK_SCOPED("AsyncRead");
	if ( m_pbo == 0 )
		glGenBuffers(1, &m_pbo);
	else
		glDeleteSync( m_sync );

	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	size_t width = texture->GetResource()->GetWidth();
	size_t height = texture->GetResource()->GetHeight();
	if ( width != m_width || height != m_height )
	{
		m_width = width;
		m_height = height;
		int bytes = width * height * sizeof(vsColor);
		glBufferData( GL_PIXEL_PACK_BUFFER, bytes, nullptr, GL_DYNAMIC_READ );
	}
	// int bytes = sizeof(uint32_t) * width * height;

	// GL_CHECK("BindBuffer");
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, texture->GetResource()->GetTexture() );
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, 0);
	glBindTexture( GL_TEXTURE_2D, 0 );
	// GL_CHECK("ReadPixels");
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);

	m_sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	// GL_CHECK("FenceSync");
}

void
vsFloatImage::AsyncReadRenderTarget(vsRenderTarget *target, int buffer)
{
	if ( m_pbo == 0 )
		glGenBuffers(1, &m_pbo);
	else
		glDeleteSync( m_sync );

	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);

	size_t width = target->GetWidth();
	size_t height = target->GetHeight();
	if ( width != m_width || height != m_height )
	{
		m_width = width;
		m_height = height;
		int bytes = width * height * sizeof(vsColor);
		glBufferData( GL_PIXEL_PACK_BUFFER, bytes, nullptr, GL_DYNAMIC_READ );
	}

	target->Bind();
	glReadBuffer(GL_COLOR_ATTACHMENT0+buffer);
	glReadPixels(0,0,width,height, GL_RGBA, GL_FLOAT, 0);

	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);

	m_sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
}

bool
vsFloatImage::AsyncReadIsReady()
{
	// GL_CHECK_SCOPED("AsyncReadIsReady");
	if ( glClientWaitSync( m_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0 ) != GL_TIMEOUT_EXPIRED )
	{
		return true;
	}
	return false;
}

void
vsFloatImage::AsyncMap()
{
	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	m_pixel = (vsColor*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	m_pixelCount = m_width * m_height;
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
}

void
vsFloatImage::AsyncUnmap()
{
	glBindBuffer( GL_PIXEL_PACK_BUFFER, m_pbo);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0);
	m_pixel = nullptr;
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

void
vsFloatImage::CopyTo( vsFloatImage *other )
{
	// in CopyTo, we've already validated that 'other' can contain our data.
	int bytes = m_width * m_height * sizeof(vsColor);
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
vsFloatImage::Copy( vsFloatImage *other )
{
	if ( m_width != (unsigned int)other->GetWidth() ||
			m_height != (unsigned int)other->GetHeight() )
	{
		vsDeleteArray(m_pixel);

		m_width = other->GetWidth();
		m_height = other->GetHeight();

		m_pixelCount = m_width * m_height;
		m_pixel = new vsColor[m_pixelCount];
	}
	other->CopyTo(this);
}


vsTexture *
vsFloatImage::Bake( const vsString& name_in )
{
	vsString name(name_in);
	if ( name.empty() )
		name = vsFormatString("FloatMakerTexture%d", s_textureMakerCount++);

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

void
vsFloatImage::SavePNG(const vsString& filename)
{
	vsFile file( filename, vsFile::MODE_Write );

	vsImage *dup = new vsImage( m_width, m_height );
	for ( size_t y = 0; y < m_height; y++ )
	{
		for ( size_t x = 0; x < m_width; x++ )
		{
			vsColor c = GetPixel(x,y);
			c.a = 1.0;
			dup->SetPixel(x,y,c);
		}
	}

	dup->SavePNG( filename );
	vsDelete(dup);
}

