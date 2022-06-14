//
//  VS_RenderTarget.cpp
//  MMORPG2
//
//  Created by Trevor Powell on 16/05/11.
//  Copyright 2011 Trevor Powell. All rights reserved.
//

#include "VS_RenderTarget.h"
#include "VS_TextureManager.h"
#include "VS_Color.h"
#include "VS_OpenGL.h"
#include "VS_RendererState.h"
#include <atomic>

namespace
{
	// [INFO]
	//
	// We sometimes want to temporarily modify the currently bound framebuffer
	// object so that we can do temporary actions;  for example, resolving an
	// MSAA rendertarget or doing a blit.  In order to transparently do those,
	// we need to be able to swap the FBO back to the previously set one, and to
	// do THAT, we need to keep track of what the previous FBO was.  So that's
	// what the following values are for.
	//
	// This implementation relies on vsRenderTarget being the only class to ever
	// call glBindFramebuffer().  If anybody else every calls that, then these
	// values will go stale and bad behaviour could occur!  So if you're reading
	// this in the future, Trevor, it might be worth searching the whole codebase
	// for glBindFramebuffer to make sure that it isn't being called from anywhere
	// that isn't keeping these values updated or restoring to these values
	// immediately afterward!
	//
	int s_currentReadFBO = 0;
	int s_currentDrawFBO = 0;
};

static std::atomic<int> s_renderTargetCount(0);

vsRenderTarget::vsRenderTarget( Type t, const vsSurface::Settings &settings, bool deferred ):
	m_settings(settings),
	m_texture(nullptr),
	m_depthTexture(nullptr),
	m_renderBufferSurface(nullptr),
	m_textureSurface(nullptr),
	m_type(t),
	m_needsResolve(0xffff),
	m_needsDepthResolve(true)
{
	bool isDepth = ( m_type == Type_Depth || m_type == Type_DepthCompare );
	m_viewportWidth = settings.width;
	m_viewportHeight = settings.height;

	m_bufferCount = settings.buffers;
	m_texture = new vsTexture*[m_bufferCount];
	for ( int i = 0; i < m_bufferCount; i++ )
	{
		vsString name = vsFormatString("RenderTarget%d", s_renderTargetCount++);
		vsTextureInternal *ti = new vsTextureInternal(name,
				this,
				i,
				isDepth);
		vsTextureManager::Instance()->Add(ti);
		m_texture[i] = new vsTexture(name);
		// pre-set some values, so they can be used even in the case of deferred
		// creation..
		m_texture[i]->GetResource()->m_width = settings.width;
		m_texture[i]->GetResource()->m_height = settings.height;
	}

	if ( settings.depth && !isDepth )
	{
		vsString name = vsFormatString("RenderTarget%d", s_renderTargetCount++);
		vsTextureInternal *ti = new vsTextureInternal(name,
				this,
				0,
				true);
		vsTextureManager::Instance()->Add(ti);
		m_depthTexture = new vsTexture(name);
	}

	if ( !deferred )
		Create();
}

void
vsRenderTarget::Create()
{
	GL_CHECK_SCOPED("RenderTarget");
	bool isDepth = ( m_type == Type_Depth || m_type == Type_DepthCompare );

	if ( m_type == Type_Window )
	{
		m_textureSurface = new vsSurface( m_settings.width, m_settings.height );

		m_texWidth = 1.f;
		m_texHeight = 1.f;
	}
	else
	{
		if ( m_type == Type_Multisample )
		{
			vsSurface::Settings rbs = m_settings;
			rbs.mipMaps = false;
			m_renderBufferSurface = new vsSurface(rbs, false, true, false);
		}
		m_textureSurface = new vsSurface(m_settings, isDepth, false, m_type == Type_DepthCompare );
		m_texWidth = 1.0;//settings.width / (float)vsNextPowerOfTwo(settings.width);
		m_texHeight = 1.0;//settings.height / (float)vsNextPowerOfTwo(settings.height);
	}
	for ( int i = 0; i < m_bufferCount; i++ )
	{
		m_texture[i]->GetResource()->SetRenderTarget(this, i, isDepth);
	}
	if ( m_depthTexture )
		m_depthTexture->GetResource()->SetRenderTarget( this, 0, true );

	Clear();
}

vsRenderTarget::~vsRenderTarget()
{
	GL_CHECK_SCOPED("vsRenderTarget::~vsRenderTarget");

	for ( int i = 0; i < m_bufferCount; i++ )
	{
		vsDelete( m_texture[i] );
	}
	vsDeleteArray( m_texture );
	vsDelete( m_depthTexture );
	vsDelete( m_textureSurface );
	vsDelete( m_renderBufferSurface );
	m_bufferCount = 0;
}

vsTexture *
vsRenderTarget::ResolveDepth()
{
	GL_CHECK_SCOPED("vsRenderTarget::Resolve");

	if ( m_needsDepthResolve )
	{
		if ( m_renderBufferSurface )
		{
			vsRendererStateBlock backup = vsRendererState::Instance()->StateBlock();

			vsRendererState::Instance()->SetBool(vsRendererState::Bool_ScissorTest,false);
			vsRendererState::Instance()->SetBool(vsRendererState::Bool_StencilTest,false);
			vsRendererState::Instance()->Flush();

			// need to copy from the render buffer surface to the regular texture.
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_textureSurface->m_fbo);

			GLbitfield bufferBits = GL_DEPTH_BUFFER_BIT;

			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0,
					m_renderBufferSurface->m_width, m_renderBufferSurface->m_height,
					0, 0,
					m_textureSurface->m_width, m_textureSurface->m_height,
					bufferBits,
					GL_NEAREST);

			// and bind us back to the previously set read/draw framebuffers.
			glBindFramebuffer(GL_READ_FRAMEBUFFER, s_currentReadFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_currentDrawFBO);

			vsRendererState::Instance()->Apply( backup );
			vsRendererState::Instance()->Flush();
		}
		m_needsDepthResolve = false;
	}

	return m_depthTexture;
	// return nullptr;
}

vsTexture *
vsRenderTarget::Resolve(int id)
{
	GL_CHECK_SCOPED("vsRenderTarget::Resolve");
	vsAssert(m_bufferCount > 0, "vsRenderTarget::Resolve called with <= 0 bufferCount?" );
	vsAssert(m_texture, "No texture array??");

	if ( m_needsResolve & BIT(id) )
	{
		if ( m_renderBufferSurface )
		{
			vsAssert(m_textureSurface, "renderbuffer RenderTarget has no texture surface??");

			vsRendererStateBlock backup = vsRendererState::Instance()->StateBlock();

			vsRendererState::Instance()->SetBool(vsRendererState::Bool_ScissorTest,false);
			vsRendererState::Instance()->SetBool(vsRendererState::Bool_StencilTest,false);
			vsRendererState::Instance()->Flush();

			// need to copy from the render buffer surface to the regular texture.
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_textureSurface->m_fbo);
			// only blit the specific buffer we're using.
			int i = id;
			// for ( int i = 0; i < m_bufferCount; i++ )
			{
				GLbitfield bufferBits = GL_COLOR_BUFFER_BIT;
				if ( id == 0 && m_renderBufferSurface->m_depth )
				{
					bufferBits |= GL_DEPTH_BUFFER_BIT; // transfer depth while we're here.
					m_needsDepthResolve = false;
				}

				glReadBuffer(GL_COLOR_ATTACHMENT0+i);
				glDrawBuffer(GL_COLOR_ATTACHMENT0+i);
				glBlitFramebuffer(0, 0,
						m_renderBufferSurface->m_width, m_renderBufferSurface->m_height,
						0, 0,
						m_textureSurface->m_width, m_textureSurface->m_height,
						bufferBits,
						GL_NEAREST);
			}
			// and bind us back to the previously set read/draw framebuffers.
			glBindFramebuffer(GL_READ_FRAMEBUFFER, s_currentReadFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_currentDrawFBO);

			vsRendererState::Instance()->Apply( backup );
			vsRendererState::Instance()->Flush();
		}

		// [TODO]  Consider whether to re-generate mipmaps on the textures,
		// since somebody's asked for them, such as with the following
		// (currently disabled) code.
		//
#if 0
		if ( m_textureSurface )
		{
			for ( int i = 0; i < m_bufferCount; i++ )
			{
				glBindTexture(GL_TEXTURE_2D, m_textureSurface->m_texture[i]);
				glGenerateMipmap(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
#endif // 0
		m_needsResolve &= ~BIT(id);
	}

	return GetTexture(id);
}

void
vsRenderTarget::CreateDeferred()
{
	if ( m_textureSurface == nullptr )
		Create();
}

void
vsRenderTarget::Bind()
{
	// somebody's going to draw into us, mark us as needing to be resolved.
	CreateDeferred();

	GL_CHECK_SCOPED("vsRenderTarget::Bind");
	if ( m_renderBufferSurface )
	{
		s_currentReadFBO = s_currentDrawFBO = m_renderBufferSurface->m_fbo;
		glBindFramebuffer(GL_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
	}
	else
	{
		s_currentReadFBO = s_currentDrawFBO = m_textureSurface->m_fbo;
		glBindFramebuffer(GL_FRAMEBUFFER, m_textureSurface->m_fbo);
	}
	if ( m_type == Type_Texture || m_type == Type_Multisample )
	{
		GLenum buffers[6] =
		{
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5
		};
		glDrawBuffers(m_bufferCount,buffers);
	}
	glViewport(0,0,m_viewportWidth, m_viewportHeight);
}

void
vsRenderTarget::Clear()
{
	Bind();
	GL_CHECK_SCOPED("vsRenderTarget::Clear");

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
	glStencilMask(0xff);
	glClearDepth(1.0);
	glClearStencil(0);
	glClearColor(0,0,0,0);
	glClear(bits);
	InvalidateResolve();
}

void
vsRenderTarget::ClearColor( const vsColor&c )
{
	Bind();
	GL_CHECK_SCOPED("vsRenderTarget::ClearColor");

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
	glStencilMask(0xff);
	glClearDepth(1.0);
	glClearStencil(0);
	glClearColor(c.r,c.g,c.b,c.a);
	glClear(bits);
	glClearColor(0,0,0,0);
	InvalidateResolve();
}

void
vsRenderTarget::BlitTo( vsRenderTarget *other )
{
	CreateDeferred();
	other->CreateDeferred();

	vsRendererStateBlock backup = vsRendererState::Instance()->StateBlock();

	vsRendererState::Instance()->SetBool(vsRendererState::Bool_Blend,false);
	vsRendererState::Instance()->SetBool(vsRendererState::Bool_ScissorTest,false);
	vsRendererState::Instance()->SetBool(vsRendererState::Bool_StencilTest,false);
	vsRendererState::Instance()->Flush();

	if ( m_renderBufferSurface )
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_renderBufferSurface->m_fbo);
	}
	else
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_textureSurface->m_fbo);
	}

	if ( other->m_renderBufferSurface )
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other->m_renderBufferSurface->m_fbo);
	}
	else
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other->m_textureSurface->m_fbo);
	}

	// for the moment, assume that a 'BlitTo' is only copying the first color attachment.
	for ( int i = 0; i < vsMin( m_bufferCount, other->m_bufferCount ); i++ )
	{
		if ( other->m_type == Type_Window ) // default framebuffer!  Don't use attachments
			glDrawBuffer(GL_BACK);
		else
			glDrawBuffer(GL_COLOR_ATTACHMENT0+i);

		glReadBuffer(GL_COLOR_ATTACHMENT0+i);


		glBlitFramebuffer(0, 0,
				m_viewportWidth, m_viewportHeight,
				0, 0, other->m_viewportWidth,
				other->m_viewportHeight,
				GL_COLOR_BUFFER_BIT,
				GL_LINEAR);
	}
	vsRendererState::Instance()->Apply( backup );
	vsRendererState::Instance()->Flush();
	// mark 'other' as needing a resolve.
	other->InvalidateResolve();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_currentReadFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_currentDrawFBO);
}

vsSurface::vsSurface( int width, int height ):
	m_width(width),
	m_height(height),
	m_texture(0),
	m_textureCount(1),
	m_depth(0),
	m_stencil(0),
	m_fbo(0),
	m_isRenderbuffer(false),
	m_multisample(false),
	m_depthCompare(false)
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

#undef GL_CHECK_SCOPED
#define GL_CHECK_SCOPED(s) vsGLContext glContextTester(s, __FILE__, __LINE__);
#undef GL_CHECK
#define GL_CHECK(s) CheckGLError(s);


vsSurface::vsSurface( const Settings& settings, bool depthOnly, bool multisample, bool depthCompare ):
	m_width(-1),
	m_height(-1),
	m_texture(0),
	m_textureCount(settings.buffers),
	m_depth(0),
	m_stencil(0),
	m_fbo(0),
	m_isRenderbuffer(false),
	m_multisample(multisample),
	m_depthCompare(depthCompare),
	m_isDepthOnly(depthOnly),
	m_settings(settings)
{
	m_texture = new GLuint[m_textureCount];

	Resize(settings.width, settings.height);
}

vsSurface::~vsSurface()
{
	GL_CHECK_SCOPED("vsSurface destructor");
	for ( int i = 0; i < m_textureCount; i++ )
	{
		if ( m_isRenderbuffer )
		{
			glDeleteRenderbuffers(1, &m_texture[i]);
		}
		else
		{
			// don't delete our textures manually;  we have vsTexture objects
			// which will do it automatically when they're destroyed, below.
		}
	}
	if ( m_depth )
	{
		glDeleteRenderbuffers(1, &m_depth);
	}
	glDeleteFramebuffers(1, &m_fbo);
	vsDeleteArray(m_texture);
}





void
vsRenderTarget::Resize( int width, int height )
{
	GL_CHECK_SCOPED("vsRenderTarget::Resize");
	if ( m_settings.width == width && m_settings.height == height )
		return;
	m_viewportWidth = width;
	m_viewportHeight = height;
	m_settings.width = width;
	m_settings.height = height;

	if ( m_type == Type_Window )
	{
		m_textureSurface->m_width = width;
		m_textureSurface->m_height = height;
	}
	else
	{
		if ( m_renderBufferSurface )
			m_renderBufferSurface->Resize(width, height);
		if ( m_textureSurface )
			m_textureSurface->Resize(width, height);

		for ( int i = 0; i < m_bufferCount; i++ )
		{
			m_texture[i]->GetResource()->m_width = m_settings.width;
			m_texture[i]->GetResource()->m_height = m_settings.height;
		}
		for ( int i = 0; i < m_bufferCount; i++ )
		{
			bool isDepth = ( m_type == Type_Depth || m_type == Type_DepthCompare );
			m_texture[i]->GetResource()->SetRenderTarget(this, i, isDepth);
		}
		if ( m_depthTexture )
			m_depthTexture->GetResource()->SetRenderTarget( this, 0, true );
	}
}

void
vsSurface::Resize( int width, int height )
{
	GL_CHECK_SCOPED("vsSurface");
	if ( m_width == width && m_height == height )
		return;

	if ( m_fbo != 0 )
	{
		for ( int i = 0; i < m_textureCount; i++ )
		{
			if ( m_isRenderbuffer )
			{
				glDeleteRenderbuffers(1, &m_texture[i]);
			}
			else
			{
				// don't delete our textures manually;  we have vsTexture objects
				// which will do it automatically when they're destroyed, below.
				glDeleteTextures(1, &m_texture[i]);
			}
		}
		if ( m_depth )
		{
			glDeleteRenderbuffers(1, &m_depth);
		}
		glDeleteFramebuffers(1, &m_fbo);
	}

	m_width = width;
	m_height = height;
	m_settings.width = width;
	m_settings.height = height;

	GLint maxSamples = 0;
	if ( m_multisample )
	{
		glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
		maxSamples = vsMin(maxSamples,4);
	}

	vsAssert( !( m_multisample && m_settings.mipMaps ), "Can't do both multisample and mipmaps!" );
	glActiveTexture(GL_TEXTURE0);

	// create FBO
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	if ( m_isDepthOnly )
	{
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		for ( int i = 0; i < m_textureCount; i++ )
			m_texture[i] = 0;
	}
	else
	{
		for ( int i = 0; i < m_textureCount; i++ )
		{
			const char* checkString[] = {
				"vsSurface texture0",
				"vsSurface texture1",
				"vsSurface texture2",
				"vsSurface texture3",
				"vsSurface texture+",
			};
			GL_CHECK_SCOPED( i > 3 ? checkString[4] : checkString[i] );
			const Settings::Buffer& settings = m_settings.bufferSettings[i];

			GLenum internalFormat = GL_RGBA8;
			GLenum format = GL_RGBA;
			GLenum type = GL_UNSIGNED_BYTE;
			if ( settings.floating )
			{
				internalFormat = GL_RGBA32F;
				type = GL_FLOAT;

				if ( settings.singleChannel )
				{
					format = GL_RED;
					internalFormat = GL_R32F;
				}
			}
			if ( settings.halfFloating )
			{
				internalFormat = GL_RGBA16F;
				type = GL_FLOAT;
				if ( settings.singleChannel )
				{
					format = GL_RED;
					internalFormat = GL_R16F;
				}
			}
			GLenum filter =  settings.linear  ? GL_LINEAR : GL_NEAREST;

			if (m_multisample)
			{
				// vsLog("MSAA in vsSurface enabled");
				glGenRenderbuffers(1, &m_texture[i]);
				glBindRenderbuffer( GL_RENDERBUFFER, m_texture[i] );
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, maxSamples, internalFormat, m_width, m_height );
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_RENDERBUFFER, m_texture[i]);
				m_isRenderbuffer = true;
			}
			else
			{
				GL_CHECK_SCOPED( "gentexture" );
				glGenTextures(1, &m_texture[i]);
				glBindTexture(GL_TEXTURE_2D, m_texture[i]);
				m_isRenderbuffer = false;
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, type, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, m_texture[i], 0);
				/* if ( settings.mipMaps ) */
				/* { */
				/* 	glGenerateMipmap(GL_TEXTURE_2D); */
				/* } */
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

				// Anisotropic filtering didn't become part of OpenGL core contextx until
				// OpenGL 4.6 (!!), so.. we sort of still have to explicitly check for
				// support.  Blah!!
				if ( GL_EXT_texture_filter_anisotropic )
				{
					if ( settings.anisotropy )
					{
						float aniso = 0.0f;
						glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
						aniso = vsMin(aniso,16.f);
						glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso );
					}
				}
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}

	if (m_settings.stencil || m_settings.depth || m_isDepthOnly)
	{
		if ( m_multisample )
		{
			GL_CHECK_SCOPED( "multisample stencil/depth" );
			glGenRenderbuffers(1, &m_depth);
			glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
			if ( m_settings.stencil )
			{
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, maxSamples, GL_DEPTH24_STENCIL8, m_width, m_height );
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth);
				m_stencil = true;
			}
			else
			{
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, maxSamples, GL_DEPTH_COMPONENT24, m_width, m_height );
			}
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);
		}
		else
		{
			GL_CHECK_SCOPED( "normal stencil/depth" );
			glGenTextures(1, &m_depth);
			glBindTexture(GL_TEXTURE_2D, m_depth);
			/* if ( settings.mipMaps ) */
			/* 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); */
			/* else */
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			if ( m_depthCompare )
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			}

			if ( m_settings.stencil )
			{
				GL_CHECK_SCOPED( "setup depthstencil texture data" );
				glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_width, m_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
			}
			else
			{
				GL_CHECK_SCOPED( "setup depthonly texture data" );
				glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0);
			}

			{
				glBindTexture(GL_TEXTURE_2D, 0);

				GL_CHECK_SCOPED( "Bind depth/stencil to framebuffer" );
				if ( m_settings.stencil )
				{
					// we're using a single depth/stencil texture, so bind it as
					// our FBO's depth_stencil attachment.
					m_stencil = true;
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);
				}
				else
				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);
				}
			}
		}
	}

	CheckFBO();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool
vsRenderTarget::IsDepthOnly()
{
	return m_type == Type_Depth || m_type == Type_DepthCompare;
}

