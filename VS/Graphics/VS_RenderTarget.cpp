//
//  VS_RenderTarget.cpp
//  MMORPG2
//
//  Created by Trevor Powell on 16/05/11.
//  Copyright 2011 Trevor Powell. All rights reserved.
//

#include "VS_RenderTarget.h"
#include "VS_TextureManager.h"

static int s_renderTargetCount = 0;

vsRenderTarget::vsRenderTarget( Type t, const vsSurface::Settings &settings_in ):
	m_texture(NULL),
	m_renderBufferSurface(NULL),
	m_textureSurface(NULL),
	m_type(t)
{
	CheckGLError("RenderTarget");
	vsSurface::Settings settings = settings_in;
	vsString name = vsFormatString("RenderTarget%d", s_renderTargetCount);
	s_renderTargetCount += 1;

	if ( m_type == Type_Window )
	{
		m_textureSurface = new vsSurface( settings.width, settings.height );

		m_texWidth = 1.f;
		m_texHeight = 1.f;
	}
	else
	{
		//settings.width = vsNextPowerOfTwo(settings.width);
		//settings.height = vsNextPowerOfTwo(settings.height);
		if ( m_type == Type_Multisample )
		{
			m_renderBufferSurface = new vsSurface(settings, false, true);
		}
		m_textureSurface = new vsSurface(settings, (t == Type_Depth), false);
		m_texWidth = 1.0;//settings.width / (float)vsNextPowerOfTwo(settings.width);
		m_texHeight = 1.0;//settings.height / (float)vsNextPowerOfTwo(settings.height);
	}

	m_viewportWidth = settings.width;
	m_viewportHeight = settings.height;

	vsTextureInternal *ti = new vsTextureInternal(name, m_textureSurface, (t == Type_Depth));
	vsTextureManager::Instance()->Add(ti);
	m_texture = new vsTexture(name);

	Clear();
}

vsRenderTarget::~vsRenderTarget()
{
	vsDelete( m_texture );
	vsDelete( m_textureSurface );
	vsDelete( m_renderBufferSurface );
}

vsTexture *
vsRenderTarget::Resolve()
{
	CheckGLError("RenderTarget");
	if ( m_renderBufferSurface )	// need to copy from the render buffer surface to the regular texture.
	{
		if ( glBindFramebuffer && glBlitFramebuffer )
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
			//Bind the standard FBO
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_textureSurface->m_fbo);
			//Let's say I want to copy the entire surface
			//Let's say I only want to copy the color buffer only
			//Let's say I don't need the GPU to do filtering since both surfaces have the same dimensions
			glBlitFramebuffer(0, 0, m_renderBufferSurface->m_width, m_renderBufferSurface->m_height, 0, 0, m_textureSurface->m_width, m_textureSurface->m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			//Consider:  Re-generate mipmaps on the texture now?

			return GetTexture();
		}
		else
		{
			// other, non-glBlitFramebuffer-based implementation goes here
			assert(0);
		}
	}
	else if ( m_textureSurface )	// don't need to do anything special;  just give them our texture.  We were rendering straight into it anyway.
	{
		return m_texture;
	}

	CheckGLError("RenderTarget");
	return NULL;
}

void
vsRenderTarget::Bind()
{
	CheckGLError("RenderTarget");
	if ( m_renderBufferSurface )
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_textureSurface->m_fbo);
	}
	glViewport(0,0,m_viewportWidth, m_viewportHeight);
	CheckGLError("RenderTarget");
}

void
vsRenderTarget::Clear()
{
	CheckGLError("RenderTarget");
	GLbitfield bits = GL_COLOR_BUFFER_BIT;
	vsSurface *surface = m_textureSurface;
	if ( m_renderBufferSurface )
	{
		surface = m_renderBufferSurface;
	}
	if ( surface->m_depth )
	{
		bits |= GL_DEPTH_BUFFER_BIT;
	}
	if ( surface->m_stencil )
	{
		bits |= GL_STENCIL_BUFFER_BIT;
	}
	glClearStencil(0);
	glClearColor(0,0,0,0);
	glClear(bits);
	CheckGLError("RenderTarget");
}

void
vsRenderTarget::BlitTo( vsRenderTarget *other )
{
	CheckGLError("RenderTarget");
	if ( m_renderBufferSurface )
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
	else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_textureSurface->m_fbo);

	if ( other->m_renderBufferSurface )
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other->m_renderBufferSurface->m_fbo);
	else
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other->m_textureSurface->m_fbo);

	glBlitFramebuffer(0, 0, m_viewportWidth, m_viewportHeight, 0, 0, other->m_viewportWidth, other->m_viewportHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	CheckGLError("RenderTarget");
}

vsSurface::vsSurface( int width, int height ):
	m_width(width),
	m_height(height),
	m_texture(0),
	m_depth(0),
	m_stencil(0),
	m_fbo(0),
	m_isRenderbuffer(false)
{
}


const char c_enums[][30] =
{
	"attachment",         // GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT........... All framebuffer attachment points are 'framebuffer attachment complete'.
	"missing attachment", // GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT....There is at least one image attached to the framebuffer.
	"duplicate attachment",// GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT
	"dimensions",         // GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS............All attached images have the same width and height.
	"formats",            // GL_FRAMEBUFFER_INCOMPLETE_FORMATS...............All images attached to the attachment points COLOR_ATTACHMENT0 through COLOR_ATTACHMENTn must have the same internal format.
	"draw buffer",        // GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER...........The value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE must not be NONE for any color attachment point(s) named by DRAW_BUFFERi.
	"read buffer",        // GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER...........If READ_BUFFER is not NONE, then the value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE must not be NONE for the color attachment point named by READ_BUFFER.
	"unsupported format"  // GL_FRAMEBUFFER_UNSUPPORTED......................The combination of internal formats of the attached images does not violate an implementation-dependent set of restrictions.
};

static void CheckFBO()
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE)
		return;

	status -= GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
	vsAssert(status == GL_FRAMEBUFFER_COMPLETE,vsFormatString("incomplete framebuffer object due to %s", c_enums[status]));
}


vsSurface::vsSurface( const Settings& settings, bool depthOnly, bool multisample ):
	m_width(settings.width),
	m_height(settings.height),
	m_texture(0),
	m_depth(0),
	m_fbo(0),
	m_isRenderbuffer(false)
{
	GLenum internalFormat = GL_RGBA8;
	GLenum pixelFormat = GL_RGBA;
	GLenum type = GL_UNSIGNED_INT_8_8_8_8_REV;
	GLenum filter = settings.linear ? GL_LINEAR : GL_NEAREST;

	CheckGLError("vsSurface");
	vsAssert( !( multisample && settings.mipMaps ), "Can't do both multisample and mipmaps!" );
	glActiveTexture(GL_TEXTURE0);
	CheckGLError("vsSurface");
	CheckGLError("vsSurface");

	// create FBO
	glGenFramebuffers(1, &m_fbo);
	CheckGLError("vsSurface");
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	CheckGLError("vsSurface");

	if ( depthOnly )
	{
		glDrawBuffer(GL_NONE);
	CheckGLError("vsSurface");
		glReadBuffer(GL_NONE);
	CheckGLError("vsSurface");
	}
	else
	{
		if (multisample)
		{
			glGenRenderbuffers(1, &m_texture);
	CheckGLError("vsSurface");
			glBindRenderbuffer( GL_RENDERBUFFER, m_texture );
	CheckGLError("vsSurface");
			glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, pixelFormat, m_width, m_height );
	CheckGLError("vsSurface");
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_texture);
	CheckGLError("vsSurface");
			m_isRenderbuffer = true;
		}
		else
		{
			glGenTextures(1, &m_texture);
	CheckGLError("vsSurface");
			glBindTexture(GL_TEXTURE_2D, m_texture);
	CheckGLError("vsSurface");
	CheckGLError("vsSurface");
			m_isRenderbuffer = false;
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, pixelFormat, type, 0);
	CheckGLError("vsSurface");
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
	CheckGLError("vsSurface");
			if ( settings.mipMaps )
			{
				int wid = m_width;
				int hei = m_height;
				int mapMapId = 1;

				while ( wid > 32 || hei > 32 )
				{
					wid = wid >> 1;
					hei = hei >> 1;
					glTexImage2D(GL_TEXTURE_2D, mapMapId++, internalFormat, wid, hei, 0, pixelFormat, type, 0);
	CheckGLError("vsSurface");
				}
			}
			glBindTexture(GL_TEXTURE_2D, m_texture);
	CheckGLError("vsSurface");
	CheckGLError("vsSurface");
			glGenerateMipmap(GL_TEXTURE_2D);
	CheckGLError("vsSurface");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	CheckGLError("vsSurface");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	CheckGLError("vsSurface");
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	CheckGLError("vsSurface");
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	CheckGLError("vsSurface");
			glBindTexture(GL_TEXTURE_2D, 0);
	CheckGLError("vsSurface");
		}
	}

	if (settings.depth || depthOnly)
	{
		if ( multisample )
		{
			glGenRenderbuffers(1, &m_depth);
	CheckGLError("vsSurface");
			glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
	CheckGLError("vsSurface");
			if ( settings.stencil )
			{
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, m_width, m_height );
	CheckGLError("vsSurface");
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth);
	CheckGLError("vsSurface");
			}
			else
			{
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, m_width, m_height );
	CheckGLError("vsSurface");
			}
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);
	CheckGLError("vsSurface");
		}
		else
		{
			glGenTextures(1, &m_depth);
	CheckGLError("vsSurface");
			glBindTexture(GL_TEXTURE_2D, m_depth);
	CheckGLError("vsSurface");
			//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	CheckGLError("vsSurface");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CheckGLError("vsSurface");
			//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	CheckGLError("vsSurface");
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	CheckGLError("vsSurface");
	CheckGLError("vsSurface");
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

			//if ( stencil )
			//{
				//glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
			//}
			//else
			{
				glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	CheckGLError("vsSurface");
			}
			glBindTexture(GL_TEXTURE_2D, 0);
	CheckGLError("vsSurface");
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);
	CheckGLError("vsSurface");
		}
		//CheckGLError("Creation of the depth renderbuffer for the FBO");
	}

	CheckFBO();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CheckGLError("vsSurface");

	//CheckGLError("Creation of the FBO itself");
}

vsSurface::~vsSurface()
{
	if ( m_isRenderbuffer )
	{
		glDeleteRenderbuffers(1, &m_texture);
	CheckGLError("vsSurface");
	}
	else
	{
		// really shouldn't delete this;  we have a texture that's using this and will delete it itself.
		//glDeleteTextures(1, &m_texture);
	}
	if ( m_depth )
	{
		glDeleteRenderbuffers(1, &m_depth);
	CheckGLError("vsSurface");
	}
	glDeleteFramebuffers(1, &m_fbo);
	CheckGLError("vsSurface");
}





