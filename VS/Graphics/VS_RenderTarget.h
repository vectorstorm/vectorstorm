//
//  VS_RenderTarget.h
//  MMORPG2
//
//  Created by Trevor Powell on 16/05/11.
//  Copyright 2011 Trevor Powell. All rights reserved.
//

#ifndef VS_RENDERTARGET_H
#define VS_RENDERTARGET_H

#include "VS_Texture.h"

#include "VS_OpenGL.h"

class vsTexture;

class vsSurface
{
public:
	class Settings
	{
	public:
		class Buffer
		{
		public:
			bool floating;
			bool halfFloating;
			bool linear;
			bool anisotropy;
			bool singleChannel;

			Buffer():
				floating(false),
				halfFloating(false),
				linear(true),
				anisotropy(true),
				singleChannel(false)
			{
			}
		};

		Settings():
			width(512),
			height(512),
			buffers(1),
			depth(true),
			mipMaps(false),
			stencil(false)
		{
		}

		int width;
		int height;

		Buffer bufferSettings[4];
		int buffers;
		bool depth;
		bool mipMaps; // currently unused
		bool stencil;
	};


	GLsizei m_width;
	GLsizei m_height;
	GLuint	*m_texture;
	int		m_textureCount;
	GLuint	m_depth;
	GLuint	m_stencil;
	GLuint	m_fbo;

	bool	m_isRenderbuffer;

	Settings m_settings;

	vsSurface( int width, int height );	// for main window.
	vsSurface( const Settings& settings, bool depthOnly, bool multisample, bool depthCompare );
	~vsSurface();

	// experimental support for resizing an existing render target.
	void Resize( int width, int height );
};


class vsRenderTarget
{
public:

	enum Type
	{
		Type_Window,			// our main OpenGL context window.  Won't allocate texture space, will just use our main framebuffer.
		Type_Texture,			// regular texture
		Type_Multisample,		// regular texture, with multisample enabled
		Type_Depth,				// depth-only, regular access.
		Type_DepthCompare		// depth-only, access via compare mode.
	};

private:

	vsSurface::Settings m_settings;

	vsTexture **m_texture;
	int			m_bufferCount;
	vsTexture * m_depthTexture;

	vsSurface *	m_renderBufferSurface;
	vsSurface *	m_textureSurface;

	GLsizei		m_viewportWidth;
	GLsizei		m_viewportHeight;
	float		m_texWidth;
	float		m_texHeight;
	Type		m_type;

	void		Create(); // If we were deferred, this creates us.

public:

	vsRenderTarget( Type t, const vsSurface::Settings &settings, bool deferred = false );
	~vsRenderTarget();

	// if deferred, create us now!  Only to be called from the render thread!
	// This is basically here to let games untangle complex render target
	// dependencies for targets which were created in a deferred fashion from a
	// background thread.  Normally, we "create" a deferred render target the
	// first time we bind it as a render target, but sometimes you can get a
	// loop where one render target is used as a texture for writing into a second
	// render target, which itself uses its texture to draw back into the first
	// render target.  In that situation, we can't safely do either draw to get
	// the deferred creation to occur;  instead, we need to manually trigger one
	// or both of the targets to get created, before we start drawing.
	void		CreateDeferred();

	void		Bind();

	/* if we're a multisample target, Resolve() copies the multisample data into
	 * our renderable texture.  If not, it does nothing.
	 * We always need to call this function before using the render target as a texture.
	 */
	vsTexture *	Resolve(int id=0);
	vsTexture *	GetTexture(int id=0) { return m_texture[id]; }
	vsTexture *	GetDepthTexture() { return m_depthTexture; }

	void		Clear();
	void		BlitTo( vsRenderTarget *other );

	GLsizei GetWidth() { return m_textureSurface->m_width; }
	GLsizei GetHeight() { return m_textureSurface->m_height; }
	GLsizei GetViewportWidth() { return m_viewportWidth; }
	GLsizei GetViewportHeight() { return m_viewportHeight; }

	float	GetTexWidth() { return m_texWidth; }
	float	GetTexHeight() { return m_texHeight; }

	// experimental support for resizing an existing render target.
	void Resize( int width, int height );
};

#endif // VS_RENDERTARGET_H

