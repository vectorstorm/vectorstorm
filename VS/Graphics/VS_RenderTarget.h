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
		Settings():
			width(512),
			height(512),
			buffers(1),
			depth(true),
			linear(true),
			mipMaps(false),
			stencil(false)
		{
		}
		int		width;
		int		height;
		int		buffers;
		bool	depth;
		bool	linear;
		bool	mipMaps;
		bool	stencil;
	};


	GLsizei m_width;
	GLsizei m_height;
	GLuint	*m_texture;
	int		m_textureCount;
	GLuint	m_depth;
	GLuint	m_stencil;
	GLuint	m_fbo;

	bool	m_isRenderbuffer;

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

	vsSurface *	m_renderBufferSurface;
	vsSurface *	m_textureSurface;

	GLsizei		m_viewportWidth;
	GLsizei		m_viewportHeight;
	float		m_texWidth;
	float		m_texHeight;
	Type		m_type;

public:


	vsRenderTarget( Type t, const vsSurface::Settings &settings );
	~vsRenderTarget();

	/* if we're a multisample target, Resolve() copies the multisample data into
	 * our renderable texture.  If not, it does nothing.
	 * We always need to call this function before using the render target as a texture.
	 */

	void		Bind();
	vsTexture *	Resolve(int id=0);
	vsTexture *	GetTexture(int id=0) { return m_texture[id]; }

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

