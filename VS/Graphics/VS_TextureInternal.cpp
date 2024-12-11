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
#include "VS_HalfIntImage.h"
#include "VS_HalfFloatImage.h"
#include "VS_SingleFloatImage.h"
#include "VS_RawImage.h"
#include "VS_RenderTarget.h"
#include "VS_RenderBuffer.h"

#include "VS/Files/VS_File.h"
#include "VS/Memory/VS_Store.h"

#include "VS_OpenGL.h"

#include "VS_GraphicsMemoryProfiler.h"

#include "stb_image.h"

namespace
{
	vsImage s_missingImage(8,8);
}

void
vsTextureInternal::PrepareToBind()
{
	if ( m_renderTarget )
	{
		if ( IsDepth() )
			m_renderTarget->ResolveDepth();
		else
			m_renderTarget->Resolve(m_surfaceBuffer);
	}
}

/* Quick utility function for texture creation */
void
vsTextureInternal::SetRenderTarget( vsRenderTarget* renderTarget, int surfaceBuffer, bool depth )
{
	// to be used for deferred texture creation  (We went through the vsSurface
	// constructor with a nullptr argument;  now we're providing the surface)

	m_renderTarget = renderTarget;
	m_surfaceBuffer = surfaceBuffer;
	vsSurface *surface = renderTarget->GetTextureSurface();
	if ( surface )
	{
		m_texture = (depth) ? surface->m_depth : surface->m_texture[surfaceBuffer];

		m_glTextureWidth = surface->m_width;
		m_glTextureHeight = surface->m_height;
		m_width = surface->m_width;
		m_height = surface->m_height;

		// if ( m_nearestSampling )
		// 	SetNearestSampling();
	}
}

vsTextureInternal::vsTextureInternal( const vsString &filename_in ):
	vsResource(filename_in),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(false),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;

	_SimpleLoadFilename(filename_in);
}

vsTextureInternal::vsTextureInternal( const vsString&name, const vsArray<vsString> &mipmaps ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(false),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;
	glBindTexture(GL_TEXTURE_2D, m_texture);

	for ( int i = 0; i < mipmaps.ItemCount(); i++ )
	{
		vsImage image(mipmaps[i]);

		int w = image.GetWidth();
		int h = image.GetHeight();

		m_width = w;
		m_height = h;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,
				i,
				GL_RGBA,
				w, h,
				0,
				GL_RGBA,
				GL_UNSIGNED_INT_8_8_8_8_REV,
				image.RawData());

		_UseMemory( w * h * sizeof(uint32_t) );
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	// Anisotropic filtering didn't become part of OpenGL core contextx until
	// OpenGL 4.6 (!!), so.. we sort of still have to explicitly check for
	// support.  Blah!!
	if ( GL_EXT_texture_filter_anisotropic )
	{
		float aniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
		aniso = vsMin(aniso,9);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	}
}

vsTextureInternal::vsTextureInternal( const vsString &name, const vsImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(false),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	m_width = w;
	m_height = h;

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
	// m_nearestSampling = false;
	_UseMemory( w * h * sizeof(uint32_t));
}

vsTextureInternal::vsTextureInternal( const vsString &name, const vsFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(false),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	m_width = w;
	m_height = h;

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
	// m_nearestSampling = false;
	_UseMemory( w * h * sizeof(float) );
}

vsTextureInternal::vsTextureInternal( const vsString &name, const vsHalfIntImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(true),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	m_width = w;
	m_height = h;

	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;

	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA16UI,
			w, h,
			0,
			GL_RGBA_INTEGER,
			GL_UNSIGNED_SHORT,
			image->RawData());
	// m_nearestSampling = false;
	_UseMemory( w * h * sizeof(uint16_t) * 4 );
}

vsTextureInternal::vsTextureInternal( const vsString &name, const vsRawImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(true),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	int w = image->GetWidth();
	int h = image->GetHeight();

	m_width = w;
	m_height = h;

	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;

	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	int internalFormat = GL_RG16UI;
	int glFormat = GL_RG_INTEGER;
	int glType = GL_UNSIGNED_SHORT;

	glTexImage2D(GL_TEXTURE_2D,
			0,
			internalFormat,
			w, h,
			0,
			glFormat,
			glType,
			image->RawData());
	// m_nearestSampling = false;
	_UseMemory( image->GetLength() );
}

vsTextureInternal::vsTextureInternal( const vsString &name, const vsHalfFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(true),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA16F,
			w, h,
			0,
			GL_RGBA,
			GL_HALF_FLOAT,
			image->RawData());
	// m_nearestSampling = false;
	_UseMemory( w * h * sizeof(uint16_t) * 4 );
}

vsTextureInternal::vsTextureInternal( const vsString &name, const vsSingleFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_lockedSampling(false),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_R32F,
			w, h,
			0,
			GL_RED,
			GL_FLOAT,
			image->RawData());
	// m_nearestSampling = false;
	_UseMemory( w * h * sizeof(float) );
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsRenderBuffer *buffer ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(false),
	m_lockedSampling(false),
	m_tbo(buffer),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	GLuint t;
	glGenTextures(1, &t);
	m_texture = t;
	// m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, uint32_t glTextureId ):
	vsResource(name),
	m_texture(glTextureId),
	m_premultipliedAlpha(false),
	m_lockedSampling(false),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	// m_nearestSampling = false;
}


void
vsTextureInternal::Blit( const vsImage *image, const vsVector2D &where)
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
vsTextureInternal::Blit( const vsFloatImage *image, const vsVector2D &where)
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

void
vsTextureInternal::Blit( const vsSingleFloatImage *image, const vsVector2D& where)
{
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexSubImage2D(GL_TEXTURE_2D,
			0,
			(int)where.x, (int)where.y,
			image->GetWidth(), image->GetHeight(),
			GL_RED,
			GL_FLOAT,
			image->RawData());
	glGenerateMipmap(GL_TEXTURE_2D);
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsRenderTarget *renderTarget, int surfaceBuffer, bool depth ):
	vsResource(name),
	m_texture(0),
	m_glTextureWidth(0),
	m_glTextureHeight(0),
	m_width(0),
	m_height(0),
	m_depth(depth),
	m_premultipliedAlpha(true),
	m_tbo(nullptr),
	m_renderTarget(nullptr),
	m_surfaceBuffer(0),
	m_memoryUsage(0L),
	m_state(0)
{
	if ( renderTarget && renderTarget->GetTextureSurface() )
	{
		SetRenderTarget(renderTarget, surfaceBuffer, depth);
	}
	// m_nearestSampling = false;
}

vsTextureInternal::~vsTextureInternal()
{
	GLuint t = m_texture;
	glDeleteTextures(1, &t);
	m_texture = 0;

	vsGraphicsMemoryProfiler::Remove( vsGraphicsMemoryProfiler::Type_Texture, m_memoryUsage );

	vsDelete( m_tbo );
	m_renderTarget = nullptr; // this doesn't belong to us;  don't destroy it!
}

void
vsTextureInternal::_UseMemory( uint64_t amt )
{
	m_memoryUsage += amt;
	vsGraphicsMemoryProfiler::Add( vsGraphicsMemoryProfiler::Type_Texture, amt );
}

// void
// vsTextureInternal::SetNearestSampling()
// {
// 	glBindTexture(GL_TEXTURE_2D, m_texture);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
// 	m_nearestSampling = true;
// }
//
// void
// vsTextureInternal::SetLinearSampling(bool linearMipmaps)
// {
// 	glBindTexture(GL_TEXTURE_2D, m_texture);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	if ( linearMipmaps )
// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
// 	else
// 		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 	m_nearestSampling = false;
// }

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

// void
// vsTextureInternal::ApplyClampUV( bool u, bool v )
// {
// 	glBindTexture(GL_TEXTURE_2D, m_texture);
// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, u ? GL_CLAMP_TO_EDGE : GL_REPEAT );
// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, v ? GL_CLAMP_TO_EDGE : GL_REPEAT );
// }
//

void
vsTextureInternal::Reload()
{
	vsString filename = vsFormatString("%s", GetName().c_str());

	if ( vsFile::Exists(filename) )
	{
		// skip files which don't end in .png;  we've only implemented PNG reloading here.
		if ( filename.find(".png") != filename.size()-4 )
			return;

		// vsLog("Would reload %s", filename);
		_SimpleLoadFilename(filename);
	}
}

void
vsTextureInternal::_SimpleLoadFilename( const vsString &filename_in )
{
	bool success = false;
	if ( vsFile::Exists(filename_in) )
	{
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		vsFile img(filename_in, vsFile::MODE_Read);

		if ( img.IsOK() )
		{
			vsStore *s = new vsStore( img.GetLength() );
			img.Store(s);

			int w,h,n;

			// glTexImage2D expects pixel data to start at the BOTTOM LEFT, but
			// stbi_load functions give us the pixel data starting at the TOP LEFT.
			// So we need to flip them here!
			//
			// ref:	https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml

			stbi_set_flip_vertically_on_load(1);
			unsigned char* data = stbi_load_from_memory( (uint8_t*)s->GetReadHead(), s->BytesLeftForReading(), &w, &h, &n, STBI_rgb_alpha );
			vsDelete(s);

			if ( !data )
				vsLog( "Failure while loading %s: %s", filename_in, stbi_failure_reason() );
			else
			{
				glTexImage2D(GL_TEXTURE_2D,
						0,
						GL_RGBA,
						w, h,
						0,
						GL_RGBA,
						GL_UNSIGNED_INT_8_8_8_8_REV,
						data);
				stbi_image_free(data);

				m_width = w;
				m_height = w;

				glGenerateMipmap(GL_TEXTURE_2D);
				success = true;

				_UseMemory( w * h * sizeof(uint32_t) );
			}
		}
	}

	if ( !success )
	{
		// put a placeholder image in here.
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,
				0,
				GL_RGBA,
				8, 8,
				0,
				GL_RGBA,
				GL_UNSIGNED_INT_8_8_8_8_REV,
				s_missingImage.RawData());
		m_width = m_height = 1;
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}
