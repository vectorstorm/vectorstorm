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
class vsColor;

#include "VS_OpenGL.h"

class vsTexture;

class vsSurface
{
public:

	// [TODO]  Depth and DepthStencil both belong here in the 'Channels'
	// enum, but that's a bigger refactor than I really want to do today.

	enum Channels
	{
		// Channels_Depth,
		// Channels_DepthStencil,
		Channels_R,
		Channels_RG,
		Channels_RGB,
		Channels_RGBA,
	};
	enum Format
	{
		Format_Byte,
		Format_SByte,
		Format_Int16,
		Format_SInt16,
		Format_HalfFloat,
		Format_Float
	};

	class Settings
	{
	public:
		class Buffer
		{
		public:
			bool linear;
			bool anisotropy;
			Format format;
			Channels channels;

			Buffer():
				linear(true),
				anisotropy(true),
				format(Format_Byte),
				channels(Channels_RGBA)
			{
			}

			bool operator==(const vsSurface::Settings::Buffer& o) const
			{
				return ( format == o.format &&
						linear == o.linear &&
						anisotropy == o.anisotropy &&
						channels == o.channels );
			}
			bool operator!=(const vsSurface::Settings::Buffer& o) const
			{
				return !( *this == o );
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

		bool operator==( const vsSurface::Settings& other ) const;
	};


	GLsizei m_width;
	GLsizei m_height;
	GLuint	*m_texture;
	int		m_textureCount;
	GLuint	m_depth;
	GLuint	m_stencil;
	GLuint	m_fbo;

	bool	m_isRenderbuffer;
	bool	m_multisample;
	bool	m_depthCompare;
	bool	m_isDepthOnly;
	bool    m_isFramebuffer;

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

	// bitfield of buffers that need resolving.
	int			m_needsResolve;
	bool		m_needsDepthResolve;

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

	void		InvalidateResolve() { m_needsResolve = 0xffff; m_needsDepthResolve = true; }

	/* if we're a multisample target, Resolve() copies the multisample data into
	 * our renderable texture.  If not, it does nothing.
	 * We always need to call this function before using the render target as a texture.
	 */
	vsTexture *	Resolve(int id=0);
	vsTexture *	ResolveDepth();
	vsTexture *	GetTexture(int id=0) { return m_texture[id]; }
	vsTexture *	GetDepthTexture() { return m_depthTexture; }

	// this should probably only be used by the TextureInternal, handling
	// deferred creation
	vsSurface * GetTextureSurface() { return m_textureSurface; };

	bool		IsDepthOnly();

	void		Clear();
	void		ClearColor( const vsColor& c );
	void		BlitTo( vsRenderTarget *other );

	void		BlitRect( vsRenderTarget *other, const vsBox2D& src, const vsBox2D& dst );

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

