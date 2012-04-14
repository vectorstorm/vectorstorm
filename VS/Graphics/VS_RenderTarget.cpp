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

vsRenderTarget::vsRenderTarget( Type t, const Settings &settings ):
	m_texture(NULL),
	m_renderBufferSurface(NULL),
	m_textureSurface(NULL),
	m_type(t)
{
	vsString name = vsFormatString("RenderTarget%d", s_renderTargetCount++);

	if ( t == Type_Window )
	{
		m_textureSurface = new vsSurface( settings.width, settings.height );

		m_texWidth = 1.f;
		m_texHeight = 1.f;
	}
	else
	{
		if ( t == Type_Multisample )
		{
			m_renderBufferSurface = new vsSurface( vsNextPowerOfTwo(settings.width), vsNextPowerOfTwo(settings.height), settings.depth, settings.linear, false, true );
		}
		m_textureSurface = new vsSurface( vsNextPowerOfTwo(settings.width), vsNextPowerOfTwo(settings.height), settings.depth, settings.linear, settings.mipMaps, false, (t == Type_Depth) );
		m_texWidth = settings.width / (float)vsNextPowerOfTwo(settings.width);
		m_texHeight = settings.height / (float)vsNextPowerOfTwo(settings.height);
	}

	m_viewportWidth = settings.width;
	m_viewportHeight = settings.height;

	vsTextureInternal *ti = new vsTextureInternal(name, m_textureSurface, (t == Type_Depth));
	vsTextureManager::Instance()->Add(ti);
	m_texture = new vsTexture(name);
	m_ortho = settings.ortho;

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
	if ( m_renderBufferSurface )	// need to copy from the render buffer surface to the regular texture.
	{
		if ( glBindFramebufferEXT && glBlitFramebufferEXT )
		{
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_renderBufferSurface->m_fbo);
			//Bind the standard FBO
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_textureSurface->m_fbo);
			//Let's say I want to copy the entire surface
			//Let's say I only want to copy the color buffer only
			//Let's say I don't need the GPU to do filtering since both surfaces have the same dimensions
			glBlitFramebufferEXT(0, 0, m_renderBufferSurface->m_width, m_renderBufferSurface->m_height, 0, 0, m_textureSurface->m_width, m_textureSurface->m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

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

	return NULL;
}

void
vsRenderTarget::Bind()
{
	if ( m_renderBufferSurface )
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_renderBufferSurface->m_fbo);
	}
	else
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_textureSurface->m_fbo);
	}
    glViewport(0,0,m_viewportWidth, m_viewportHeight);
    glMatrixMode(GL_PROJECTION);
	if ( m_ortho )
	{
		glOrtho(0, GetWidth(), 0, GetHeight(), 0, 10);
	}
	else
	{
		glLoadIdentity();
	}
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void
vsRenderTarget::Clear()
{
	if ( m_renderBufferSurface )
	{
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | (m_renderBufferSurface->m_depth ? GL_DEPTH_BUFFER_BIT : 0));
	}
	else
	{
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | (m_textureSurface->m_depth ? GL_DEPTH_BUFFER_BIT : 0));
	}
}

void
vsRenderTarget::BlitTo( vsRenderTarget *other )
{
	if ( glBindFramebufferEXT && glBlitFramebufferEXT )
	{
		if ( m_renderBufferSurface )
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_renderBufferSurface->m_fbo);
		else
			glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_textureSurface->m_fbo);

		if ( other->m_renderBufferSurface )
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, other->m_renderBufferSurface->m_fbo);
		else
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, other->m_textureSurface->m_fbo);

		glBlitFramebufferEXT(0, 0, m_viewportWidth, m_viewportHeight, 0, 0, other->m_viewportWidth, other->m_viewportHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}
	else
	{
		assert(0);
		/*
		glBindTexture( GL_TEXTURE_2D, other->GetTexture()->GetResource()->GetTexture() );
		BindSurface(&m_pass[p]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		 */
	}
}

vsSurface::vsSurface( int width, int height ):
	m_width(width),
	m_height(height),
	m_texture(0),
	m_depth(0),
	m_fbo(0),
	m_isRenderbuffer(false)
{
}



const char c_enums[][20] =
{
	"attachment",         // GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT........... All framebuffer attachment points are 'framebuffer attachment complete'.
	"missing attachment", // GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT....There is at least one image attached to the framebuffer.
	"",                   //
	"dimensions",         // GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT............All attached images have the same width and height.
	"formats",            // GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT...............All images attached to the attachment points COLOR_ATTACHMENT0_EXT through COLOR_ATTACHMENTn_EXT must have the same internal format.
	"draw buffer",        // GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT...........The value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT must not be NONE for any color attachment point(s) named by DRAW_BUFFERi.
	"read buffer",        // GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT...........If READ_BUFFER is not NONE, then the value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT must not be NONE for the color attachment point named by READ_BUFFER.
	"unsupported format"  // GL_FRAMEBUFFER_UNSUPPORTED_EXT......................The combination of internal formats of the attached images does not violate an implementation-dependent set of restrictions.
};

static void CheckFBO()
{
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
        return;

    status -= GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
    vsAssert(status == GL_FRAMEBUFFER_COMPLETE_EXT,vsFormatString("incomplete framebuffer object due to %s", c_enums[status]));
}


vsSurface::vsSurface( int width, int height, bool depth, bool linear, bool withMipMaps, bool withMultisample, bool depthOnly ):
	m_width(width),
	m_height(height),
	m_texture(0),
	m_depth(0),
	m_fbo(0),
	m_isRenderbuffer(false)

{
    GLenum internalFormat = GL_RGBA8;
	GLenum pixelFormat = GL_RGBA;
    GLenum type = GL_UNSIGNED_INT_8_8_8_8_REV;
    GLenum filter = linear ? GL_LINEAR : GL_NEAREST;

	vsAssert( !( withMultisample && withMipMaps ), "Can't do both multisample and mipmaps!" );
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

    if ( !depthOnly )
    {
        // create a color texture
        if ( withMultisample )
        {
            glGenRenderbuffersEXT(1, &m_texture);
            glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, m_texture );
            glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, 4, pixelFormat, m_width, m_height );
            m_isRenderbuffer = true;
        }
        else
        {
            glGenTextures(1, &m_texture);
            glBindTexture(GL_TEXTURE_2D, m_texture);
            glEnable(GL_TEXTURE_2D);
            m_isRenderbuffer = false;
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, pixelFormat, type, 0);
        }
        if ( withMipMaps )
        {
            int wid = m_width;
            int hei = m_height;
            int mapMapId = 1;

            while ( wid > 32 || hei > 32 )
            {
                wid = wid >> 1;
                hei = hei >> 1;
                glTexImage2D(GL_TEXTURE_2D, mapMapId++, internalFormat, wid, hei, 0, pixelFormat, type, 0);
            }
        }
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glEnable(GL_TEXTURE_2D);
        glGenerateMipmapEXT(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
        glBindTexture(GL_TEXTURE_2D, 0);

    }

	//CheckGLError("Creation of the color texture for the FBO");

    // create depth
    if (depth || depthOnly)
    {
		if ( withMultisample )
		{
            glGenRenderbuffersEXT(1, &m_depth);
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depth);
			glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, 4, GL_DEPTH_COMPONENT24, m_width, m_height );
		}
		else
		{
            glGenTextures(1, &m_depth);
            glBindTexture(GL_TEXTURE_2D, m_depth);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
            
            glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
		}
        //CheckGLError("Creation of the depth renderbuffer for the FBO");
    }

    // create FBO itself
    glGenFramebuffersEXT(1, &m_fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
    
    if ( depthOnly )
    {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        if (withMultisample)
        {
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, m_texture);
        }
        else
        {
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_texture, 0);
        }
    }
    
    if (depth || depthOnly)
    {
		if ( withMultisample )
		{
            glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depth);
		}
		else
		{
            glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_depth, 0);
		}
        //CheckGLError("Creation of the depth renderbuffer for the FBO");
    }

	CheckFBO();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    //CheckGLError("Creation of the FBO itself");
}

vsSurface::~vsSurface()
{
	if ( m_isRenderbuffer )
	{
		glDeleteRenderbuffersEXT(1, &m_texture);
	}
	else
	{
		// really shouldn't delete this;  we have a texture that's using this and will delete it itself.
		//glDeleteTextures(1, &m_texture);
	}
	if ( m_depth )
	{
		glDeleteRenderbuffersEXT(1, &m_depth);
	}
	glDeleteFramebuffersEXT(1, &m_fbo);
}





