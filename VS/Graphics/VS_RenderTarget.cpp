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

	m_bufferCount = settings.buffers;
	m_texture = new vsTexture*[m_bufferCount];
	for ( int i = 0; i < settings.buffers; i++ )
	{
		vsString name = vsFormatString("RenderTarget%d", s_renderTargetCount++);
		vsTextureInternal *ti = new vsTextureInternal(name, m_textureSurface, i, (t == Type_Depth));
		vsTextureManager::Instance()->Add(ti);
		m_texture[i] = new vsTexture(name);
	}

	Clear();
}

vsRenderTarget::~vsRenderTarget()
{
	for ( int i = 0; i < m_bufferCount; i++ )
	{
		vsDelete( m_texture[i] );
	}
	vsDeleteArray( m_texture );
	vsDelete( m_textureSurface );
	vsDelete( m_renderBufferSurface );
}

vsTexture *
vsRenderTarget::Resolve(int id)
{
	if ( m_renderBufferSurface )
	{
		// need to copy from the render buffer surface to the regular texture.
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_textureSurface->m_fbo);
		for ( int i = 0; i < m_bufferCount; i++ )
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0+i);
			glDrawBuffer(GL_COLOR_ATTACHMENT0+i);
			glBlitFramebuffer(0, 0, m_renderBufferSurface->m_width, m_renderBufferSurface->m_height, 0, 0, m_textureSurface->m_width, m_textureSurface->m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
	}
	if ( m_textureSurface )
	{
		// TODO:  Consider whether to re-generate mipmaps on the textures,
		// since somebody's asked for them, such as with the following
		// (commented-out) code.
		//
		// for ( int i = 0; i < m_bufferCount; i++ )
		// {
		// 	glBindTexture(GL_TEXTURE_2D, m_textureSurface->m_texture[i]);
		// 	glGenerateMipmap(GL_TEXTURE_2D);
		// 	glBindTexture(GL_TEXTURE_2D, 0);
		// }

		return GetTexture(id);
	}

	return NULL;
}

void
vsRenderTarget::Bind()
{
	if ( m_renderBufferSurface )
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_textureSurface->m_fbo);
	}
	if ( m_type == Type_Texture || m_type == Type_Multisample )
	{
		GLenum buffers[6] =
		{
			GL_COLOR_ATTACHMENT0_EXT,
			GL_COLOR_ATTACHMENT1_EXT,
			GL_COLOR_ATTACHMENT2_EXT,
			GL_COLOR_ATTACHMENT3_EXT,
			GL_COLOR_ATTACHMENT4_EXT,
			GL_COLOR_ATTACHMENT5_EXT
		};
		glDrawBuffers(m_bufferCount,buffers);
	}
	glViewport(0,0,m_viewportWidth, m_viewportHeight);
}

void
vsRenderTarget::Clear()
{
	GLbitfield bits = GL_COLOR_BUFFER_BIT;
	vsSurface *surface = m_renderBufferSurface ? m_renderBufferSurface : m_textureSurface;
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
}

void
vsRenderTarget::BlitTo( vsRenderTarget *other )
{
	if ( m_renderBufferSurface )
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
	else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_textureSurface->m_fbo);

	if ( other->m_renderBufferSurface )
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other->m_renderBufferSurface->m_fbo);
	else
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other->m_textureSurface->m_fbo);

	glBlitFramebuffer(0, 0, m_viewportWidth, m_viewportHeight, 0, 0, other->m_viewportWidth, other->m_viewportHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
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
	m_texture = new GLuint[1];
	m_texture[0] = 0;
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
	m_textureCount(settings.buffers),
	m_depth(0),
	m_fbo(0),
	m_isRenderbuffer(false)
{
	CheckGLError("vsSurface");

	GLenum internalFormat = GL_RGBA8;
	GLenum pixelFormat = GL_RGBA;
	GLenum type = GL_UNSIGNED_INT_8_8_8_8_REV;
	GLenum filter = settings.linear ? GL_LINEAR : GL_NEAREST;

	vsAssert( !( multisample && settings.mipMaps ), "Can't do both multisample and mipmaps!" );
	glActiveTexture(GL_TEXTURE0);

	// create FBO
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	m_texture = new GLuint[m_textureCount];

	if ( depthOnly )
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}
	else
	{
		for ( int i = 0; i < m_textureCount; i++ )
		{
			if (multisample)
			{
				glGenRenderbuffers(1, &m_texture[i]);
				glBindRenderbuffer( GL_RENDERBUFFER, m_texture[i] );
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, pixelFormat, m_width, m_height );
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_RENDERBUFFER, m_texture[i]);
				m_isRenderbuffer = true;
			}
			else
			{
				glGenTextures(1, &m_texture[i]);
				glBindTexture(GL_TEXTURE_2D, m_texture[i]);
				m_isRenderbuffer = false;
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, pixelFormat, type, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, m_texture[i], 0);
				if ( settings.mipMaps )
				{
					glGenerateMipmap(GL_TEXTURE_2D);
				}
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}

	if (settings.depth || depthOnly)
	{
		if ( multisample )
		{
			glGenRenderbuffers(1, &m_depth);
			glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
			if ( settings.stencil )
			{
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, m_width, m_height );
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth);
			}
			else
			{
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, m_width, m_height );
			}
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);
		}
		else
		{
			glGenTextures(1, &m_depth);
			glBindTexture(GL_TEXTURE_2D, m_depth);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
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
			}
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);
		}
	}

	CheckFBO();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CheckGLError("vsSurface");
}

vsSurface::~vsSurface()
{
	for ( int i = 0; i < m_textureCount; i++ )
	{
		if ( m_isRenderbuffer )
		{
			glDeleteRenderbuffers(1, &m_texture[i]);
			CheckGLError("vsSurface");
		}
		else
		{
			// really shouldn't delete this;  we have a texture that's using this and will delete it itself.
			//glDeleteTextures(1, &m_texture[i]);
		}
	}
	if ( m_depth )
	{
		glDeleteRenderbuffers(1, &m_depth);
		CheckGLError("vsSurface");
	}
	glDeleteFramebuffers(1, &m_fbo);
	CheckGLError("vsSurface");
	vsDeleteArray(m_texture);
}





