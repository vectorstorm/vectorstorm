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
#include "VS_RenderTarget.h"
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
	m_clampU(false),
	m_clampV(false),
	m_applyClamp(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL)
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	// vsString filename = vsFile::GetFullFilename(filename_in);

	m_imageToLoad = new vsImage(filename_in);
	// m_texture = ::loadTexture( filename.c_str() );
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsImage *maker ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_clampU(false),
	m_clampV(false),
	m_applyClamp(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_imageToLoad = new vsImage(*maker);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_clampU(false),
	m_clampV(false),
	m_applyClamp(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_floatImageToLoad = new vsFloatImage(*image);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsHalfFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_clampU(false),
	m_clampV(false),
	m_applyClamp(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_halfFloatImageToLoad = new vsHalfFloatImage(*image);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsHalfIntImage *image ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_clampU(false),
	m_clampV(false),
	m_applyClamp(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL)
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_halfIntImageToLoad = new vsHalfIntImage(*image);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsSurface *surface, bool depth ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_clampU(false),
	m_clampV(false),
	m_applyClamp(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL)
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	if ( surface )
		m_texture = (depth) ? surface->m_depth : surface->m_texture;
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsRenderBuffer *buffer ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(false),
	m_clampU(false),
	m_clampV(false),
	m_applyClamp(false),
	m_tbo(buffer),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL)
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	// GLuint t;
	// glGenTextures(1, &t);
	// m_texture = t;
	m_nearestSampling = false;
}

vsTextureInternal::~vsTextureInternal()
{
	GLuint t = m_texture;
	glDeleteTextures(1, &t);
	m_texture = 0;

	vsDelete( m_tbo );
	m_renderTarget = NULL;
}



#else // TARGET_OS_IPHONE

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
	// if ( m_applyClamp )
	// {
	// 	glBindTexture(GL_TEXTURE_2D, m_texture);
	// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_clampU ? GL_CLAMP_TO_EDGE : GL_REPEAT );
	// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_clampV ? GL_CLAMP_TO_EDGE : GL_REPEAT );
	// 	m_applyClamp = false;
	// }
}

/* Quick utility function for texture creation */
void
vsTextureInternal::SetRenderTarget( vsRenderTarget* renderTarget, int surfaceBuffer, bool depth )
{
	// to be used for deferred texture creation  (We went through the vsSurface
	// constructor with a NULL argument;  now we're providing the surface)

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

		if ( m_nearestSampling )
			SetNearestSampling();
	}
}

vsTextureInternal::vsTextureInternal( const vsString &filename_in ):
	vsResource(filename_in),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_imageToLoad = new vsImage(filename_in);
	// vsString filename = vsFile::GetFullFilename(filename_in);
    //
	// SDL_Surface *loadedImage = IMG_Load(filename.c_str());
	// vsAssert(loadedImage != NULL, vsFormatString("Unable to load texture %s: %s", filename.c_str(), IMG_GetError()));
	// ProcessSurface(loadedImage);
	// SDL_FreeSurface(loadedImage);
	// m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_imageToLoad = new vsImage(*image);
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_floatImageToLoad = new vsFloatImage(*image);
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsRenderBuffer *buffer ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(false),
	m_tbo(buffer),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, uint32_t glTextureId ):
	vsResource(name),
	m_texture(glTextureId),
	m_premultipliedAlpha(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_nearestSampling = false;
}

void
vsTextureInternal::DoLoadFromImage()
{
	if ( m_imageToLoad && m_imageToLoad->IsOK() )
	{
		int w = m_imageToLoad->GetWidth();
		int h = m_imageToLoad->GetHeight();

		m_width = w;
		m_height = w;

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
				m_imageToLoad->RawData());
		glGenerateMipmap(GL_TEXTURE_2D);
		m_nearestSampling = false;

		vsDelete( m_imageToLoad );
	}
}

void
vsTextureInternal::DoLoadFromFloatImage()
{
	int w = m_floatImageToLoad->GetWidth();
	int h = m_floatImageToLoad->GetHeight();

	m_width = w;
	m_height = w;

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
			m_floatImageToLoad->RawData());
	glGenerateMipmap(GL_TEXTURE_2D);
	m_nearestSampling = false;

	vsDelete( m_floatImageToLoad );
}

void
vsTextureInternal::DoLoadFromHalfFloatImage()
{
	int w = m_halfFloatImageToLoad->GetWidth();
	int h = m_halfFloatImageToLoad->GetHeight();

	m_width = w;
	m_height = w;

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
			m_halfFloatImageToLoad->RawData());
	glGenerateMipmap(GL_TEXTURE_2D);
	m_nearestSampling = false;

	vsDelete( m_halfFloatImageToLoad );
}

void
vsTextureInternal::DoLoadFromHalfIntImage()
{
	int w = m_halfIntImageToLoad->GetWidth();
	int h = m_halfIntImageToLoad->GetHeight();

	m_width = w;
	m_height = w;

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
			m_halfIntImageToLoad->RawData());
	m_nearestSampling = false;

	vsDelete( m_halfIntImageToLoad );
}

// 	// Anisotropic filtering didn't become part of OpenGL core contextx until
// 	// OpenGL 4.6 (!!), so.. we sort of still have to explicitly check for
// 	// support.  Blah!!
// 	if ( GL_EXT_texture_filter_anisotropic )
// 	{
// 		float aniso = 0.0f;
// 		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
// 		aniso = vsMin(aniso,9);
// 		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
// 	}
// }

void
vsTextureInternal::EnsureLoaded()
{
	if ( m_texture == 0 )
	{
		GLuint t;
		glGenTextures(1, &t);
		m_texture = t;
	}
	if ( m_imageToLoad && m_imageToLoad->IsOK())
	{
		DoLoadFromImage();
	}
	if ( m_floatImageToLoad )
	{
		DoLoadFromFloatImage();
	}
	if ( m_halfFloatImageToLoad )
	{
		DoLoadFromHalfFloatImage();
	}
	if ( m_halfIntImageToLoad )
	{
		DoLoadFromHalfIntImage();
	}
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsHalfFloatImage *image ):
	vsResource(name),
	m_texture(0),
	m_depth(false),
	m_premultipliedAlpha(false),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_halfFloatImageToLoad = new vsHalfFloatImage(*image);
	m_nearestSampling = false;
}

vsTextureInternal::vsTextureInternal( const vsString &name, vsHalfIntImage *image ):
	vsResource(name),
	m_texture(0),
	m_premultipliedAlpha(true),
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	m_halfIntImageToLoad = new vsHalfIntImage(*image);
	m_nearestSampling = false;
}


//
// vsTextureInternal::vsTextureInternal( const vsString &name, uint32_t glTextureId ):
// 	vsResource(name),
// 	m_texture(glTextureId),
// 	m_premultipliedAlpha(false),
// 	m_tbo(NULL),
// 	m_renderTarget(NULL),
// 	m_surfaceBuffer(0)


void
vsTextureInternal::Blit( vsImage *image, const vsVector2D &where)
{
	// if ( vsRenderer_OpenGL3::Instance()->IsLoadingContext() )
	// [TODO]  It's possible for these Blit functions to be called from
	// a background thread.  These blit operations (this one and
	// the one below) will have to be queued up as well, I guess!
	vsDelete( m_imageToLoad );
	m_imageToLoad = new vsImage(*image);
	// EnsureLoaded();
    //
	// glBindTexture(GL_TEXTURE_2D, m_texture);
	// glTexSubImage2D(GL_TEXTURE_2D,
	// 		0,
	// 		(int)where.x, (int)where.y,
	// 		image->GetWidth(), image->GetHeight(),
	// 		GL_RGBA,
	// 		GL_UNSIGNED_INT_8_8_8_8_REV,
	// 		image->RawData());
	// glGenerateMipmap(GL_TEXTURE_2D);
}

void
vsTextureInternal::Blit( vsFloatImage *image, const vsVector2D &where)
{
	vsDelete( m_floatImageToLoad );
	m_floatImageToLoad = new vsFloatImage(*image);
	// EnsureLoaded();
    //
	// glBindTexture(GL_TEXTURE_2D, m_texture);
	// glTexSubImage2D(GL_TEXTURE_2D,
	// 		0,
	// 		(int)where.x, (int)where.y,
	// 		image->GetWidth(), image->GetHeight(),
	// 		GL_RGBA,
	// 		GL_FLOAT,
	// 		image->RawData());
	// glGenerateMipmap(GL_TEXTURE_2D);
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
	m_tbo(NULL),
	m_renderTarget(NULL),
	m_surfaceBuffer(0),
	m_imageToLoad(NULL),
	m_floatImageToLoad(NULL),
	m_halfFloatImageToLoad(NULL),
	m_halfIntImageToLoad(NULL)
{
	if ( renderTarget && renderTarget->GetTextureSurface() )
	{
		SetRenderTarget(renderTarget, surfaceBuffer, depth);
	}
	m_nearestSampling = false;
}

vsTextureInternal::~vsTextureInternal()
{
	GLuint t = m_texture;
	glDeleteTextures(1, &t);
	m_texture = 0;


	vsDelete( m_tbo );
	m_renderTarget = NULL; // this doesn't belong to us;  don't destroy it!
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
	m_clampU = u;
	m_clampV = v;
	m_applyClamp = true;
	// glBindTexture(GL_TEXTURE_2D, m_texture);
	// glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, u ? GL_CLAMP_TO_EDGE : GL_REPEAT );
	// glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, v ? GL_CLAMP_TO_EDGE : GL_REPEAT );
}

