/*
 *  VS_Renderer_OpenGL2.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Renderer_OpenGL2.h"

#include "VS_Camera.h"
#include "VS_Debug.h"
#include "VS_DisplayList.h"
#include "VS_Image.h"
#include "VS_MaterialInternal.h"
#include "VS_Matrix.h"
#include "VS_RenderBuffer.h"
#include "VS_Screen.h"
#include "VS_Shader.h"
#include "VS_ShaderSuite.h"
#include "VS_System.h"
#include "VS_Texture.h"
#include "VS_TextureInternal.h"

#include "VS_RenderSchemeBloom.h"
#include "VS_RenderSchemeShader.h"
#include "VS_RenderSchemeFixedFunction.h"

#include "VS_OpenGL.h"

#include "VS_TimerSystem.h"

extern const char *normalv;
extern const char *texv;
extern const char *litv;
extern const char *litf;
extern const char *litTexf;
extern const char *normalf;
extern const char *texf;

GLuint			vsRenderer_OpenGL2::s_normalProg = -1;
GLuint			vsRenderer_OpenGL2::s_litProg = -1;
GLuint			vsRenderer_OpenGL2::s_normalTexProg = -1;
GLuint			vsRenderer_OpenGL2::s_litTexProg = -1;

GLuint			vsRenderer_OpenGL2::s_normalProgFogLoc = -1;
GLuint			vsRenderer_OpenGL2::s_litProgFogLoc = -1;
GLuint			vsRenderer_OpenGL2::s_normalTexProgFogLoc = -1;
GLuint			vsRenderer_OpenGL2::s_litTexProgFogLoc = -1;
static GLint s_litTexfAlphaRef;
static GLint s_texfAlphaRef;

bool				vsRenderer_OpenGL2::s_shadersBuilt = false;


#if TARGET_OS_IPHONE

#define glOrtho( a, b, c, d, e, f ) glOrthof( a, b, c, d, e, f )
#define glFrustum( a, b, c, d, e, f ) glFrustumf( a, b, c, d, e, f )
#define glClearDepth( a ) glClearDepthf( a )
#define glFogi( a, b ) glFogx( a, b )
#define glTexParameteri( a, b, c ) glTexParameterx( a, b, c )

#endif

static SDL_Window *m_sdlWindow = NULL;
static SDL_Renderer *m_sdlRenderer = NULL;
static SDL_GLContext m_sdlGlContext;

void vsRenderDebug( const vsString &message )
{
	vsLog("%s", message.c_str());
}

//static bool s_vertexBuffersSupported = false;

vsRenderer_OpenGL2::vsRenderer_OpenGL2(int width, int height, int depth, int flags):
	vsRenderer(width, height, depth, flags),
	m_window(NULL),
	m_scene(NULL)
{
	m_viewportWidth = m_width = width;
	m_viewportHeight = m_height = height;

#if !TARGET_OS_IPHONE
	//const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
	int videoFlags;

	videoFlags = SDL_WINDOW_OPENGL;
#ifdef HIGHDPI_SUPPORTED
	videoFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

	if ( flags & Flag_Fullscreen )
		videoFlags |= SDL_WINDOW_FULLSCREEN;
	else if ( flags & Flag_Resizable )
		videoFlags |= SDL_WINDOW_RESIZABLE;


	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );

//#ifdef _DEBUG
	//SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 0 );
//#else
	//SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );	// in release builds, lock our frames to vsynch.
//#endif

	// ATI cards misbehave if you request accelerated visuals, and NVidia cards do the right thing regardless.
	// So comment it out!
	//SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );

	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 8 );

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#ifdef __APPLE__
/*	GLint swapInterval = 1;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &swapInterval);

	// Enable the multi-threading
	CGLEnable( CGLGetCurrentContext(), kCGLCEMPEngine);*/
#endif // __APPLE__

	//SDL_Surface *s = SDL_SetVideoMode( width, height, videoInfo->vfmt->BitsPerPixel, videoFlags );
	SDL_CreateWindowAndRenderer(width, height, videoFlags, &m_sdlWindow, &m_sdlRenderer);

	if ( !m_sdlWindow || !m_sdlRenderer  ){
		fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
				width, height, depth, SDL_GetError() );
		exit(1);
	}
#ifdef HIGHDPI_SUPPORTED
	SDL_GL_GetDrawableSize(m_sdlWindow, &m_widthPixels, &m_heightPixels);
#else
	m_widthPixels = width;
	m_heightPixels = height;
#endif
	m_viewportWidthPixels = m_widthPixels;
	m_viewportHeightPixels = m_heightPixels;
	if ( m_viewportWidthPixels != m_widthPixels ||
			m_viewportHeightPixels != m_heightPixels )
	{
		vsLog("High DPI Rendering enabled");
		vsLog("High DPI backing store is: %dx%d", m_viewportWidthPixels, m_viewportHeightPixels);
	}

	m_sdlGlContext = SDL_GL_CreateContext(m_sdlWindow);
	if ( !m_sdlGlContext )
	{
		vsLog("Failed to create OpenGL context??");
		exit(1);
	}

	GLenum err = glewInit();
	vsAssert(GLEW_OK == err, vsFormatString("GLEW error: %s", glewGetErrorString(err)).c_str());
	if ( GL_VERSION_2_1 )
	{
		vsLog("Support for GL 2.1 found");
	}
	else
	{
		vsLog("No support for GL 2.1");
	}

	m_antialias = (glRenderbufferStorageMultisampleEXT != NULL);

	if ( SDL_GL_SetSwapInterval(flags & Flag_VSync ? 1 : 0) == -1 )
	{
		vsLog("Couldn't set vsync");
	}

	vsLog( "VSync: %s", SDL_GL_GetSwapInterval() > 0 ? "ENABLED" : "DISABLED" );

	int MaxVertexTextureImageUnits;
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &MaxVertexTextureImageUnits);
	int MaxCombinedTextureImageUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &MaxCombinedTextureImageUnits);

	vsLog( "TextureUnits: %d from vertex shader, %d total", MaxVertexTextureImageUnits, MaxCombinedTextureImageUnits );

	int val;
	SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS, &val );
	if ( val )
	{
		SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES, &val );
		vsLog("Using %d-sample multisampling.", val);
	}
	SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &val );
	if ( !val )
		vsLog("WARNING:  Failed to initialise double-buffering");

	//SDL_GL_SetSwapInterval(1);
	//SDL_GL_GetAttribute( SDL_GL_SWAP_CONTROL, &val );
	//if ( !val )
		//vsLog("WARNING:  Failed to initialise swap control");
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &val );
	if ( !val )
		vsLog("WARNING:  Failed to get stencil buffer bits");
//	SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL, &val );
//	if ( !val )
//		vsLog("WARNING:  Failed to initialise accelerated rendering");

#endif // !TARGET_OS_IPHONE


	glShadeModel( GL_SMOOTH );
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
#if !TARGET_OS_IPHONE
	glClearDepth( 1.0f );  // arbitrary large value
#endif // !TARGET_OS_IPHONE

	//glEnable( GL_TEXTURE_2D );		// no textures in vector graphics!
	//glEnable( GL_DEPTH_TEST );		// no depth in vector graphics!
	//glShadeModel( GL_SMOOTH );

	glBlendFunc(GL_SRC_ALPHA,GL_ONE);							// Set The Blending Function For Additive
	glEnable(GL_BLEND);											// Enable Blending

	// no depth in vector graphics, so no need to provide depth tests!
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);


	m_state.SetBool( vsRendererState::Bool_DepthTest, true );
	glDepthFunc( GL_LEQUAL );

	glViewport( 0, 0, (GLsizei)m_widthPixels, (GLsizei)m_heightPixels );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	s_normalProg = Compile(normalv, normalf);
	s_litProg = Compile(litv, litf);
	s_normalTexProg = Compile(texv, texf);
	s_litTexProg = Compile(litv, litTexf);
	s_shadersBuilt = true;

	s_litTexfAlphaRef = glGetUniformLocation(s_litTexProg, "alphaRef");
	s_texfAlphaRef = glGetUniformLocation(s_normalTexProg, "alphaRef");

	s_normalProgFogLoc = glGetUniformLocation(s_normalProg, "fog");
	s_litProgFogLoc = glGetUniformLocation(s_litProg, "fog");
	s_normalTexProgFogLoc = glGetUniformLocation(s_normalTexProg, "fog");
	s_litTexProgFogLoc = glGetUniformLocation(s_litTexProg, "fog");

	Resize();

	CheckGLError("Initialising OpenGL rendering");
}

vsRenderer_OpenGL2::~vsRenderer_OpenGL2()
{
	vsDelete(m_window);
	vsDelete(m_scene);
	SDL_DestroyRenderer( m_sdlRenderer );
	SDL_GL_DeleteContext( m_sdlGlContext );
	SDL_DestroyWindow( m_sdlWindow );
	m_sdlWindow = NULL;
	m_sdlRenderer = NULL;
}

void
vsRenderer_OpenGL2::Resize()
{
	vsDelete( m_window );
	vsDelete( m_scene );

	// Create Window Surface
	vsSurface::Settings settings;
	settings.depth = 0;
	settings.width = GetWidthPixels();
	settings.height = GetHeightPixels();
	settings.ortho = true;
	m_window = new vsRenderTarget( vsRenderTarget::Type_Window, settings );

    // Create 3D Scene Surface
	// we want to be big enough to hold our full m_window resolution, and set our viewport to match the window.

    settings.width = GetWidthPixels();
    settings.height = GetHeightPixels();
	settings.depth = true;
    settings.linear = true;
	settings.mipMaps = false;
	settings.ortho = true;
	settings.stencil = true;

	if ( m_antialias )
	{
		m_scene = new vsRenderTarget( vsRenderTarget::Type_Multisample, settings );
	}
	else
	{
		m_scene = new vsRenderTarget( vsRenderTarget::Type_Texture, settings );
	}
	SetViewportWidthPixels( m_scene->GetViewportWidth() );
	SetViewportHeightPixels( m_scene->GetViewportHeight() );
}

void
vsRenderer_OpenGL2::UpdateVideoMode(int width, int height, int depth, bool fullscreen)
{
	UNUSED(depth);
	UNUSED(fullscreen);
	//vsAssert(0, "Not yet implemented");
	m_width = m_viewportWidth = width;
	m_height = m_viewportHeight = height;
#ifdef HIGHDPI_SUPPORTED
	SDL_GL_GetDrawableSize(m_sdlWindow, &m_widthPixels, &m_heightPixels);
#else
	m_widthPixels = width;
	m_heightPixels = height;
#endif
	// m_scheme->Resize();
}

void
vsRenderer_OpenGL2::SetCameraTransform( const vsTransform2D &t )
{
	vsScreen *s = vsSystem::GetScreen();
	float hei = t.GetScale().x;
	float wid = s->GetAspectRatio() * hei;
	float hw = wid * 0.5f;
	float hh = hei * 0.5f;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Normal:
		case Orientation_Six:
			glOrtho( -hw, hw, hh, -hh, -1000, 1000 );
			break;
		case Orientation_Three:
		case Orientation_Nine:
			glOrtho( -hh, hh, hw, -hw, -1000, 1000 );
			break;
	}

	glMatrixMode( GL_MODELVIEW );

	//glRotatef( rotationDegrees, 0, 0, 1 );
	//glTranslatef( 0, -640, 0 );
	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Normal:
			break;
		case Orientation_Six:
			glRotatef(180.f, 0.f, 0.f, 1.f);
			break;
		case Orientation_Three:
			glRotatef(90.f, 0.f, 0.f, 1.f);
			break;
		case Orientation_Nine:
			glRotatef(270.f, 0.f, 0.f, 1.f);
			break;
	}



	glRotatef( -t.GetAngle().GetDegrees(), 0.0f, 0.0f, 1.0f );
	glTranslatef( -t.GetTranslation().x, -t.GetTranslation().y, 0.0f );

	m_state.SetBool( vsRendererState::Bool_DepthTest, false );
	m_state.SetBool( vsRendererState::Bool_CullFace, false );

	CheckGLError("SetCameraTransform");
}

void
vsRenderer_OpenGL2::Set3DProjection( float fov, float nearPlane, float farPlane )
{
	vsScreen *s = vsSystem::GetScreen();

	float hh = vsTan(fov * .5f) * nearPlane;
	float hw = hh * s->GetAspectRatio();

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Normal:
		case Orientation_Six:
			glFrustum(-hw,hw,-hh,hh,nearPlane,farPlane);
			break;
		case Orientation_Three:
		case Orientation_Nine:
			glFrustum(-hh,hh,-hw,hw,nearPlane,farPlane);
			break;
	}

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Normal:
			break;
		case Orientation_Six:
			glRotatef(180.f, 0.f, 0.f, 1.f);
			break;
		case Orientation_Three:
			glRotatef(270.f, 0.f, 0.f, 1.f);
			break;
		case Orientation_Nine:
			glRotatef(90.f, 0.f, 0.f, 1.f);
			break;
	}

	glScalef(-1.f, 1.f, 1.f);

	m_state.SetBool( vsRendererState::Bool_DepthTest, true );
	m_state.SetBool( vsRendererState::Bool_DepthMask, true );
	m_state.SetBool( vsRendererState::Bool_CullFace, true );
	m_state.SetInt( vsRendererState::Int_CullFace, GL_FRONT );

	CheckGLError("Set3DProjection");
}

void
vsRenderer_OpenGL2::SetCameraProjection( const vsMatrix4x4 &m )
{
	m_currentCameraPosition = m.w;

	//vsVector3D p = vsVector3D::Zero;
	vsVector3D forward = /*m.t + */m.z;
	vsVector3D up = m.y;
	vsVector3D side = forward.Cross(up);
	/*
	gluLookAt(p.x, p.y, p.z,
			  t.x, t.y, t.z,
			  u.x, u.y, u.z );*/

	float mat[16];
	mat[0] = side[0];
	mat[4] = side[1];
	mat[8] = side[2];
	mat[12] = 0.0;
	//------------------
	mat[1] = up[0];
	mat[5] = up[1];
	mat[9] = up[2];
	mat[13] = 0.0;
	//------------------
	mat[2] = -forward[0];
	mat[6] = -forward[1];
	mat[10] = -forward[2];
	mat[14] = 0.0;
	//------------------
	mat[3] = mat[7] = mat[11] = 0.0;
	mat[15] = 1.0;

	glMultMatrixf( (float*)&mat );

	CheckGLError("SetCameraProjection");
}

void
vsRenderer_OpenGL2::PreRender(const Settings &s)
{
	CheckGLError("PreRender");
	glViewport( 0, 0, (GLsizei)m_widthPixels, (GLsizei)m_heightPixels );

	m_state.SetBool( vsRendererState::Bool_DepthMask, true );
	m_currentMaterial = NULL;

	m_scene->Bind();

	if ( m_antialias )
	{
		m_state.SetBool( vsRendererState::Bool_Multisample, true );
	}
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );

	m_state.SetBool( vsRendererState::Bool_DepthMask, true );
	m_state.Flush();

	glClearColor(0.f,0.f,0.f,0.f);
	glClearDepth(1.f);
	glClearStencil(0);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	m_state.SetBool( vsRendererState::Bool_StencilTest, true );
	m_state.Flush();
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	// our baseline size is 1024x768.  We want single-pixel lines at that size.
	float lineScaleFactor = vsMax(2.0f,m_heightPixels / 384.f);
	glLineWidth( lineScaleFactor );

	CheckGLError("PreRender");
}

void
vsRenderer_OpenGL2::PostRender()
{
	m_scene->Resolve();
	m_window->Bind();
	m_scene->BlitTo(m_window);

	vsTimerSystem::Instance()->EndRenderTime();
	//glFinish();
#if !TARGET_OS_IPHONE
	SDL_GL_SwapWindow(m_sdlWindow);
#endif
	vsTimerSystem::Instance()->EndGPUTime();
	CheckGLError("PostRender");
}

void
vsRenderer_OpenGL2::RenderDisplayList( vsDisplayList *list )
{
	vsTransform2D defCamera;
	defCamera.SetTranslation( vsVector2D::Zero );
	defCamera.SetAngle( vsAngle::Zero );
	defCamera.SetScale( vsVector2D(1000.0f,1000.0f) );
	SetCameraTransform(defCamera);

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RawRenderDisplayList(list);

	CheckGLError("RenderDisplayList");
}

void
vsRenderer_OpenGL2::RawRenderDisplayList( vsDisplayList *list )
{
	m_currentCameraPosition = vsVector3D::Zero;

	m_currentMaterial = NULL;

	vsDisplayList::op *op = list->PopOp();
	//vsVector3D	cursorPos;
	//vsColor		cursorColor;
	//vsColor		currentColor(-1,-1,-1,0);
	//vsColor		nextColor;
	//vsOverlay	currentOverlay;
	//bool		colorSet = false;
	//bool		cursorSet = false;
	//bool		inLineStrip = false;
	//bool		inPointList = false;
	//bool		recalculateColor = false;	// if true, recalc color even if we don't think it's changed

	//bool		usingVertexArray = false;
	m_usingNormalArray = false;
	m_usingTexelArray = false;
	m_lightCount = 0;

	m_currentVertexArray = NULL;
	m_currentVertexBuffer = NULL;
	m_currentTexelArray = NULL;
	m_currentTexelBuffer = NULL;
	m_currentColorArray = NULL;
	m_currentColorBuffer = NULL;
	m_currentTransformStackLevel = 0;
	m_currentVertexArrayCount = 0;

	m_inOverlay = false;

	while(op)
	{
		switch( op->type )
		{
			case vsDisplayList::OpCode_SetColor:
			{
				const vsColor &nextColor = op->data.GetColor();
				glColor4f( nextColor.r, nextColor.g, nextColor.b, nextColor.a );
				m_currentMaterial = NULL;	// explicitly set a color, that means our cached material no longer matches the OpenGL state.
				break;
			}
			case vsDisplayList::OpCode_SetSpecularColor:
			{
				const vsColor &nextColor = op->data.GetColor();
				glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, (float*)&nextColor );
				m_currentMaterial = NULL;	// explicitly set a color, that means our cached material no longer matches the OpenGL state.
				break;
			}
			case vsDisplayList::OpCode_MoveTo:
			{
				break;
			}
			case vsDisplayList::OpCode_LineTo:
			{
				break;
			}
			case vsDisplayList::OpCode_SetMaterial:
			{
				vsMaterialInternal *material = (vsMaterialInternal *)op->data.p;
				if ( material->GetName() == "BrushLine" )
				{
					int a = 10;
					a++;
				}
				SetMaterial( material );
				break;
			}
			case vsDisplayList::OpCode_SetTexture:
			{
				vsTexture *t = (vsTexture *)op->data.p;
				if ( t )
				{
					{
						glEnable(GL_TEXTURE_2D);
						glBindTexture( GL_TEXTURE_2D, t->GetResource()->GetTexture() );
					}
				}
				else
				{
					glDisable(GL_TEXTURE_2D);
				}
				m_currentMaterial = NULL;	// explicitly set a texture, that means our cached material no longer matches the OpenGL state.
				break;
			}
#if !TARGET_OS_IPHONE
			case vsDisplayList::OpCode_DrawPoint:
			{
				m_state.Flush();
				glBegin( GL_POINTS );
					vsVector3D pos = op->data.GetVector3D();
					glVertex3f( pos.x, pos.y, pos.z );
				glEnd();

				break;
			}
			case vsDisplayList::OpCode_CompiledDisplayList:
			{
				uint32_t id = op->data.GetUInt();
				glCallList(id);
				m_currentMaterial = NULL;	// display state could have been changed by the display list.
				break;
			}
#endif // !TARGET_OS_IPHONE
			case vsDisplayList::OpCode_PushTransform:
			{
				vsTransform2D t = op->data.GetTransform();

				vsVector3D v = t.GetTranslation();
				if ( m_currentTransformStackLevel == 0 )
				{
					v -= m_currentCameraPosition;
				}

				++m_currentTransformStackLevel;

				bool translation = (v != vsVector2D::Zero);
				bool rotation = (t.GetAngle() != vsAngle::Zero);
				bool scale = (t.GetScale() != vsVector2D::One);
				glPushMatrix();
				if ( translation )
					glTranslatef( v.x, v.y, v.z );
				if ( rotation )
					glRotatef( t.GetAngle().GetDegrees(), 0.0f, 0.0f, 1.0f );
				if ( scale )
					glScalef( t.GetScale().x, t.GetScale().y, 1.0f );

				vsMatrix4x4 mat;
				glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&mat);
				if ( mat.x == mat.y )
				{
					mat.y = mat.x;
				}
				break;
			}
			case vsDisplayList::OpCode_PushTranslation:
			{
				vsVector3D &v = op->data.vector;
				glPushMatrix();
				if ( m_currentTransformStackLevel == 0 )
				{
					v -= m_currentCameraPosition;
				}

				glTranslatef( v.x, v.y, v.z );
				m_currentTransformStackLevel++;
				break;
			}
			case vsDisplayList::OpCode_PushMatrix4x4:
			{
				vsMatrix4x4 m = op->data.GetMatrix4x4();
				glPushMatrix();

				if ( m_currentTransformStackLevel == 0 )
				{
					m.w -= m_currentCameraPosition;
				}

				glMultMatrixf((float *)&m);
				m_currentTransformStackLevel++;
				break;
			}
			case vsDisplayList::OpCode_SetMatrix4x4:
			{
				glPushMatrix();
				vsMatrix4x4 &m = op->data.GetMatrix4x4();
				glLoadMatrixf((float *)&m);
				m_currentTransformStackLevel++;
				break;
			}
			case vsDisplayList::OpCode_PopTransform:
			{
				vsAssert(m_currentTransformStackLevel > 0, "Renderer transform stack underflow??");
				m_currentTransformStackLevel--;
				glPopMatrix();
				break;
			}
			case vsDisplayList::OpCode_SetCameraTransform:
			{
				vsTransform2D t = op->data.GetTransform();
				m_currentCameraPosition = vsVector3D::Zero;

				SetCameraTransform(t);
				break;
			}
			case vsDisplayList::OpCode_Set3DProjection:
			{
				float fov = op->data.fov;
				float nearPlane = op->data.nearPlane;
				float farPlane = op->data.farPlane;

				Set3DProjection(fov, nearPlane, farPlane);
				break;
			}
			case vsDisplayList::OpCode_SetProjectionMatrix4x4:
			{
				glMatrixMode( GL_PROJECTION );
				vsMatrix4x4 &m = op->data.GetMatrix4x4();
				glLoadMatrixf((float *)&m);
				glMatrixMode( GL_MODELVIEW );
				glLoadIdentity();

				switch( vsSystem::Instance()->GetOrientation() )
				{
					case Orientation_Normal:
						break;
					case Orientation_Six:
						glRotatef(180.f, 0.f, 0.f, 1.f);
						break;
					case Orientation_Three:
						glRotatef(270.f, 0.f, 0.f, 1.f);
						break;
					case Orientation_Nine:
						glRotatef(90.f, 0.f, 0.f, 1.f);
						break;
				}

				glScalef(-1.f, 1.f, 1.f);
				break;
			}
			case vsDisplayList::OpCode_SetCameraProjection:
			{
				const vsMatrix4x4 &m = op->data.GetMatrix4x4();

				SetCameraProjection(m);
				break;
			}
			case vsDisplayList::OpCode_VertexArray:
			{
				glVertexPointer( 3, GL_FLOAT, 0, op->data.p );
				m_currentVertexArray = (vsVector3D *)op->data.p;
				m_currentVertexArrayCount = op->data.i;
				m_state.SetBool( vsRendererState::ClientBool_VertexArray, true );
				break;
			}
			case vsDisplayList::OpCode_VertexBuffer:
			{
				m_currentVertexBuffer = (vsRenderBuffer *)op->data.p;
				m_currentVertexBuffer->BindVertexBuffer( &m_state );
				break;
			}
			case vsDisplayList::OpCode_NormalArray:
			{
				glNormalPointer( GL_FLOAT, 0, op->data.p );
				m_currentNormalArray = (vsVector3D *)op->data.p;
				m_currentNormalArrayCount = op->data.i;
				m_state.SetBool( vsRendererState::ClientBool_NormalArray, true );
				break;
			}
			case vsDisplayList::OpCode_NormalBuffer:
			{
				m_currentNormalBuffer = (vsRenderBuffer *)op->data.p;
				m_currentNormalBuffer->BindNormalBuffer( &m_state );
				m_state.SetBool( vsRendererState::ClientBool_NormalArray, true );
				break;
			}
			case vsDisplayList::OpCode_ClearVertexArray:
			{
				if ( m_currentVertexBuffer )
				{
					//m_currentVertexBuffer->UnbindVertexBuffer();
					m_currentVertexBuffer = NULL;
				}
				m_state.SetBool( vsRendererState::ClientBool_VertexArray, false );
				m_currentVertexArray = NULL;
				m_currentVertexArrayCount = 0;
				break;
			}
			case vsDisplayList::OpCode_ClearNormalArray:
			{
				if ( m_currentNormalBuffer )
				{
					//m_currentNormalBuffer->UnbindNormalBuffer();
					m_currentNormalBuffer = NULL;
				}
				m_currentNormalArray = NULL;
				m_currentNormalArrayCount = 0;
				m_state.SetBool( vsRendererState::ClientBool_NormalArray, false );
				break;
			}
			case vsDisplayList::OpCode_TexelArray:
			{
				glTexCoordPointer( 2, GL_FLOAT, 0, op->data.p );
				m_currentTexelArray = (vsVector2D *)op->data.p;
				m_currentTexelArrayCount = op->data.i;
				m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );
				break;
			}
			case vsDisplayList::OpCode_TexelBuffer:
			{
				m_currentTexelBuffer = (vsRenderBuffer *)op->data.p;
				m_currentTexelBuffer->BindTexelBuffer( &m_state );
				break;
			}
			case vsDisplayList::OpCode_ClearTexelArray:
			{
				if ( m_currentTexelBuffer )
				{
					//m_currentTexelBuffer->UnbindTexelBuffer();
					m_currentTexelBuffer = NULL;
				}
				m_currentTexelArray = NULL;
				m_currentTexelArrayCount = 0;
				m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
				break;
			}
			case vsDisplayList::OpCode_ColorArray:
			{
				glColorPointer( 4, GL_FLOAT, 0, op->data.p );
				m_state.SetBool( vsRendererState::ClientBool_ColorArray, true );
				m_currentColorArray = (vsColor *)op->data.p;
				m_currentColorArrayCount = op->data.i;
				m_currentMaterial = NULL;
				break;
			}
			case vsDisplayList::OpCode_ColorBuffer:
			{
				m_currentColorBuffer = (vsRenderBuffer *)op->data.p;
				m_currentColorBuffer->BindColorBuffer( &m_state );
				m_currentMaterial = NULL;
				break;
			}
			case vsDisplayList::OpCode_ClearColorArray:
			{
				if ( m_currentColorBuffer )
				{
					//m_currentColorBuffer->UnbindColorBuffer();
					m_currentColorBuffer = NULL;
				}
				m_currentColorArray = NULL;
				m_currentColorArrayCount = 0;
				m_state.SetBool( vsRendererState::ClientBool_ColorArray, false );
				break;
			}
			case vsDisplayList::OpCode_ClearArrays:
			{
				m_currentColorArray = NULL;
				m_currentColorBuffer = NULL;
				m_currentColorArrayCount = 0;
				m_state.SetBool( vsRendererState::ClientBool_ColorArray, false );

				m_currentTexelBuffer = NULL;
				m_currentTexelArray = NULL;
				m_currentTexelArrayCount = 0;
				m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );

				m_currentNormalBuffer = NULL;
				m_currentNormalArray = NULL;
				m_currentNormalArrayCount = 0;
				m_state.SetBool( vsRendererState::ClientBool_NormalArray, false );

				m_currentVertexBuffer = NULL;
				m_currentVertexArray = NULL;
				m_currentVertexArrayCount = 0;
				m_state.SetBool( vsRendererState::ClientBool_VertexArray, false );
				break;
			}
			case vsDisplayList::OpCode_BindBuffer:
			{
				vsRenderBuffer *buffer = (vsRenderBuffer *)op->data.p;
				buffer->Bind( &m_state );
				break;
			}
			case vsDisplayList::OpCode_UnbindBuffer:
			{
				vsRenderBuffer *buffer = (vsRenderBuffer *)op->data.p;
				buffer->Unbind( &m_state );
				break;
			}
			case vsDisplayList::OpCode_LineList:
			{
				m_state.Flush();
				glDrawElements( GL_LINES, op->data.GetUInt(), GL_UNSIGNED_SHORT, op->data.p );
				break;
			}
			case vsDisplayList::OpCode_LineStrip:
			{
				m_state.Flush();
				glDrawElements( GL_LINE_STRIP, op->data.GetUInt(), GL_UNSIGNED_SHORT, op->data.p );
				break;
			}
			case vsDisplayList::OpCode_TriangleList:
			{
				m_state.Flush();
				glDrawElements( GL_TRIANGLES, op->data.GetUInt(), GL_UNSIGNED_SHORT, op->data.p );
				break;
			}
			case vsDisplayList::OpCode_TriangleStrip:
			{
				m_state.Flush();
				glDrawElements( GL_TRIANGLE_STRIP, op->data.GetUInt(), GL_UNSIGNED_SHORT, op->data.p );
				break;
			}
			case vsDisplayList::OpCode_TriangleStripBuffer:
			{
				m_state.Flush();
				vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
				ib->TriStripBuffer();
				break;
			}
			case vsDisplayList::OpCode_TriangleListBuffer:
			{
				m_state.Flush();
				vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
				ib->TriListBuffer();
				break;
			}
			case vsDisplayList::OpCode_TriangleFanBuffer:
			{
				m_state.Flush();
				vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
				ib->TriFanBuffer();
				break;
			}
			case vsDisplayList::OpCode_LineListBuffer:
			{
				m_state.Flush();
				vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
				ib->LineListBuffer();
				break;
			}
			case vsDisplayList::OpCode_LineStripBuffer:
			{
				m_state.Flush();
				vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
				ib->LineStripBuffer();
				break;
			}
			case vsDisplayList::OpCode_TriangleFan:
			{
				m_state.Flush();
				glDrawElements( GL_TRIANGLE_FAN, op->data.GetUInt(), GL_UNSIGNED_SHORT, op->data.p );
				break;
			}
			/*case vsDisplayList::OpCode_SetDrawMode:
			{
				vsDrawMode newMode = (vsDrawMode)op->data.GetUInt();

				SetDrawMode(newMode);
				break;
			}*/
			case vsDisplayList::OpCode_Light:
			{
				if ( m_lightCount < GL_MAX_LIGHTS - 1 )
				{
					GLfloat pos[4] = {0.f,0.f,0.f,0.f};
					GLfloat whiteColor[4] = {1.f,1.f,1.f,1.f};
					GLfloat blackColor[4] = {0.f,0.f,0.f,1.f};

					vsLight &l = op->data.light;
					int lightId = GL_LIGHT0 + m_lightCount;
					glEnable(lightId);
					if ( l.m_type == vsLight::Type_Ambient )
					{
						glLightfv(lightId, GL_AMBIENT, (float *)&l.m_color);
						glLightfv(lightId, GL_DIFFUSE, (float *)&blackColor);
						glLightfv(lightId, GL_SPECULAR, (float *)&blackColor);
					}
					else
					{
						if ( l.m_type == vsLight::Type_Point )
						{
							pos[0] = l.m_position.x;
							pos[1] = l.m_position.y;
							pos[2] = l.m_position.z;
							pos[3] = 1.f;
							glLightfv(lightId, GL_POSITION, pos);
						}
						else if ( l.m_type == vsLight::Type_Directional )
						{
							pos[0] = l.m_direction.x;
							pos[1] = l.m_direction.y;
							pos[2] = l.m_direction.z;
							pos[3] = 0.f;
							glLightfv(lightId, GL_POSITION, pos);
						}
						glLightfv(lightId, GL_AMBIENT, (float *)&l.m_ambient);
						glLightfv(lightId, GL_DIFFUSE, (float *)&l.m_color);
						glLightfv(lightId, GL_SPECULAR, (float *)&whiteColor);

						glLightf(lightId, GL_LINEAR_ATTENUATION, 0.05f);
						glLightf(lightId, GL_QUADRATIC_ATTENUATION, 0.01f);
					}


					m_lightCount++;
				}
				break;
			}
			case vsDisplayList::OpCode_ClearLights:
			{
				for ( int i = 0; i < m_lightCount; i++ )
				{
					glDisable( GL_LIGHT0 + i );
				}
				m_lightCount = 0;
				break;
			}
			case vsDisplayList::OpCode_Fog:
			{
				vsColor fogColor = op->data.fog.GetColor();
				glFogfv(GL_FOG_COLOR, (GLfloat*)&fogColor);

				if ( op->data.fog.IsLinear() )
				{
					glFogi(GL_FOG_MODE, GL_LINEAR);
					glFogf(GL_FOG_START, op->data.fog.GetStart() );
					glFogf(GL_FOG_END, op->data.fog.GetEnd() );
				}
				else
				{
					glFogi(GL_FOG_MODE, GL_EXP2);
					glFogf(GL_FOG_DENSITY, op->data.fog.GetDensity() );
				}

				break;
			}
			case vsDisplayList::OpCode_ClearFog:
			{
				glFogf(GL_FOG_DENSITY, 0.f );
				break;
			}
			case vsDisplayList::OpCode_FlatShading:
			{
				glShadeModel( GL_FLAT );
				break;
			}
			case vsDisplayList::OpCode_SmoothShading:
			{
				glShadeModel( GL_SMOOTH );
				break;
			}
			case vsDisplayList::OpCode_EnableStencil:
			{
				glStencilFunc(GL_EQUAL, 0x1, 0x1);
				break;
			}
			case vsDisplayList::OpCode_DisableStencil:
			{
				//m_state.SetBool( vsRendererState::Bool_StencilTest, false );
				glStencilFunc(GL_ALWAYS, 0x1, 0x1);
				break;
			}
			case vsDisplayList::OpCode_ClearStencil:
			{
				glClearStencil(0);
				glClear(GL_STENCIL_BUFFER_BIT);
				break;
			}
			case vsDisplayList::OpCode_EnableScissor:
			{
				m_state.SetBool( vsRendererState::Bool_ScissorTest, true );
				const vsBox2D& box = op->data.box2D;
				glScissor( box.GetMin().x * m_viewportWidthPixels,
						box.GetMin().y * m_viewportHeightPixels,
						box.Width() * m_viewportWidthPixels,
						box.Height() * m_viewportHeightPixels );
				break;
			}
			case vsDisplayList::OpCode_DisableScissor:
			{
				m_state.SetBool( vsRendererState::Bool_ScissorTest, false );
				break;
			}
			case vsDisplayList::OpCode_SetViewport:
			{
				const vsBox2D& box = op->data.box2D;
				glViewport( box.GetMin().x * m_viewportWidthPixels,
						box.GetMin().y * m_viewportHeightPixels,
						box.Width() * m_viewportWidthPixels,
						box.Height() * m_viewportHeightPixels );
				//glViewport( box.GetMin().x,
						//box.GetMin().y,
						//box.Width(),
						//box.Height());
				break;
			}
			case vsDisplayList::OpCode_ClearViewport:
			{
				glViewport( 0, 0, (GLsizei)m_viewportWidthPixels, (GLsizei)m_viewportHeightPixels );
				break;
			}
			case vsDisplayList::OpCode_Debug:
			{
				vsRenderDebug( op->data.string );
				break;
			}
			default:
				vsAssert(false, "Unknown opcode type in display list!");	// error;  unknown opcode type in the display list!
		}
		CheckGLError("RenderOp");
		op = list->PopOp();

		//CheckGLError("Testing");
	}

	//SetUsingTexels(false);
	//SetUsingColorArray(false);

/*	if ( inLineStrip )
	{
		inLineStrip = false;
		glEnd();
	}
	if ( inPointList )
	{
		inPointList = false;
		glEnd();
	}*/
}

void
vsRenderer_OpenGL2::SetMaterial(vsMaterialInternal *material)
{
	if ( material == m_currentMaterial )
	{
		return;
	}
	m_currentMaterial = material;

    if ( m_currentSettings.writeColor )
    {
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,material->m_glow);
    }
    else
    {
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    }
    if ( m_currentSettings.writeDepth )
    {
        m_state.SetBool( vsRendererState::Bool_DepthMask, material->m_zWrite );
    }
    else
    {
        m_state.SetBool( vsRendererState::Bool_DepthMask, false );
    }
    m_currentShader = NULL;

	switch ( material->m_stencil )
	{
		case StencilOp_None:
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			break;
		case StencilOp_One:
			glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			break;
		case StencilOp_Zero:
			glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
			break;
		case StencilOp_Inc:
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			break;
		case StencilOp_Dec:
			glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
			break;
		case StencilOp_Invert:
			glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
			break;
		default:
			vsAssert(0, vsFormatString("Unhandled stencil type: %d", material->m_stencil));
	}

	if ( material->m_shader )
	{
		glUseProgram( material->m_shader->GetShaderId() );
		material->m_shader->Prepare();
        m_currentShader = material->m_shader;
	}
	else
	{
		switch( material->m_drawMode )
		{
			case DrawMode_Add:
			case DrawMode_Subtract:
			case DrawMode_Normal:
			case DrawMode_Absolute:
				if ( material->m_texture[0] )
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::NormalTex) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::NormalTex);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
                        m_currentShader->SetAlphaRef( material->m_alphaRef );
					}
					else
					{
						glUseProgram( s_normalTexProg );
						glUniform1f( s_normalTexProgFogLoc, material->m_fog );
						glUniform1f( s_texfAlphaRef, material->m_alphaRef );
					}
				}
				else
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Normal) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Normal);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
					}
					else
					{
						glUseProgram( s_normalProg );
						glUniform1f( s_normalProgFogLoc, 0 );
					}
				}
				break;
			case DrawMode_Lit:

				if ( material->m_texture[0] )
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::LitTex) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::LitTex);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
                        m_currentShader->SetAlphaRef( material->m_alphaRef );
					}
					else
					{
						glUseProgram( s_litTexProg );
						glUniform1f( s_litTexProgFogLoc, material->m_fog );
						glUniform1f( s_litTexfAlphaRef, material->m_alphaRef );
					}
				}
				else
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Lit) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Lit);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
					}
					else
					{
						glUseProgram( s_litProg );
						glUniform1f( s_litProgFogLoc, 1 );
						//glUniform1f( s_litProgFogLoc, material->m_fog );
					}
				}
				break;
			default:
				vsAssert(0,"Unknown drawmode??");
		}
	}

	/*static bool doIt = false;
	if ( doIt )
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}*/
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
	{
		vsTexture *t = material->GetTexture(i);
		glActiveTexture(GL_TEXTURE0 + i);
		if ( t )
		{
			//glEnable(GL_BLEND);
			//if ( !m_currentTexture || t->GetResource() != m_currentTexture->GetResource() )
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture( GL_TEXTURE_2D, t->GetResource()->GetTexture() );

				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, material->m_clampU ? GL_CLAMP_TO_EDGE : GL_REPEAT );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, material->m_clampV ? GL_CLAMP_TO_EDGE : GL_REPEAT );
			}
		}
		else
		{
			//glDisable(GL_BLEND);
			glDisable(GL_TEXTURE_2D);
		}
	}
	glActiveTexture(GL_TEXTURE0);

	if ( material->m_alphaTest )
	{
		m_state.SetFloat( vsRendererState::Float_AlphaThreshhold, material->m_alphaRef );
	}

	if ( material->m_zRead )
	{
		glDepthFunc( GL_LEQUAL );
	}

	//glPolygonOffset( material->m_depthBiasConstant, material->m_depthBiasFactor );
	if ( material->m_depthBiasConstant == 0.f && material->m_depthBiasFactor == 0.f )
	{
		m_state.SetBool( vsRendererState::Bool_PolygonOffsetFill, false );
	}
	else
	{
		m_state.SetBool( vsRendererState::Bool_PolygonOffsetFill, true );
		m_state.SetFloat2( vsRendererState::Float2_PolygonOffsetConstantAndFactor, material->m_depthBiasConstant, material->m_depthBiasFactor );
	}

	m_state.SetBool( vsRendererState::Bool_AlphaTest, material->m_alphaTest );
	m_state.SetBool( vsRendererState::Bool_DepthTest, material->m_zRead );
	m_state.SetBool( vsRendererState::Bool_DepthMask, material->m_zWrite );
	m_state.SetBool( vsRendererState::Bool_Fog, material->m_fog );

	if ( material->m_cullingType == Cull_None )
	{
		m_state.SetBool( vsRendererState::Bool_CullFace, false );
	}
	else
	{
		m_state.SetBool( vsRendererState::Bool_CullFace, true );

		bool cullingBack = (material->m_cullingType == Cull_Back);
		if ( m_currentSettings.invertCull )
		{
			cullingBack = !cullingBack;
		}

		if ( cullingBack )
		{
			m_state.SetInt( vsRendererState::Int_CullFace, GL_BACK );
		}
		else
		{
			m_state.SetInt( vsRendererState::Int_CullFace, GL_FRONT );
		}
	}
	switch ( material->m_stencil )
	{
		case StencilOp_None:
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			break;
		case StencilOp_One:
			glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			break;
		case StencilOp_Zero:
			glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
			break;
		case StencilOp_Inc:
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			break;
		case StencilOp_Dec:
			glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
			break;
		case StencilOp_Invert:
			glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
			break;
		default:
			vsAssert(0, vsFormatString("Unhandled stencil type: %d", material->m_stencil));
	}


	m_state.SetBool( vsRendererState::Bool_Blend, material->m_blend );
	switch( material->m_drawMode )
	{
		case DrawMode_Add:
		{
#if !TARGET_OS_IPHONE
			glBlendEquation(GL_FUNC_ADD);
#endif
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);					// additive
			m_state.SetBool( vsRendererState::Bool_Lighting, false );
			m_state.SetBool( vsRendererState::Bool_ColorMaterial, false );
			break;
		}
		case DrawMode_Subtract:
		{
#if !TARGET_OS_IPHONE
			glBlendEquation(GL_FUNC_SUBTRACT);
#endif
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			m_state.SetBool( vsRendererState::Bool_Lighting, false );
			m_state.SetBool( vsRendererState::Bool_ColorMaterial, false );
			break;
		}
		case DrawMode_Normal:
		{
#if !TARGET_OS_IPHONE
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);	// opaque
#else
			glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);	// opaque
#endif
			m_state.SetBool( vsRendererState::Bool_Lighting, false );
			m_state.SetBool( vsRendererState::Bool_ColorMaterial, false );
			break;
		}
		case DrawMode_Lit:
		{
#if !TARGET_OS_IPHONE
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);	// opaque
#else
			glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);	// opaque
#endif
			m_state.SetBool( vsRendererState::Bool_Lighting, true );
			m_state.SetBool( vsRendererState::Bool_ColorMaterial, true );
#if !TARGET_OS_IPHONE
			glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) ;
#endif
			float materialAmbient[4] = {0.f, 0.f, 0.f, 1.f};

			glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 50.f );
			glLightModelfv( GL_LIGHT_MODEL_AMBIENT, materialAmbient);
			break;
		}
		case DrawMode_Absolute:
		{
#if !TARGET_OS_IPHONE
			glBlendEquation(GL_FUNC_ADD);
#endif
			glBlendFunc(GL_ONE,GL_ZERO);	// absolute, no alpha processing
			break;
		}
		default:
			vsAssert(0, "Unknown draw mode requested!");
	}

	if ( material->m_hasColor )
	{
		const vsColor &c = material->m_color;
		glColor4f( c.r, c.g, c.b, c.a );

		if ( material->m_drawMode == DrawMode_Lit )
		{
			const vsColor &specColor = material->m_specularColor;
			glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, (float*)&specColor );
		}
	}
	m_state.Flush();
}

GLuint
vsRenderer_OpenGL2::Compile(const char *vert, const char *frag, int vLength, int fLength )
{
    GLchar buf[256];
    GLuint vertShader, fragShader, program;
    GLint success;

	GLint *vLengthPtr = NULL;
	GLint *fLengthPtr = NULL;

	if ( vLength > 0 )
		vLengthPtr = (GLint*)&vLength;
	if ( fLength > 0 )
		fLengthPtr = (GLint*)&fLength;

    vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, (const GLchar**) &vert, vLengthPtr);
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertShader, sizeof(buf), 0, buf);
        vsLog(buf);
        vsAssert(success,"Unable to compile vertex shader.\n");
    }

    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, (const GLchar**) &frag, fLengthPtr);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragShader, sizeof(buf), 0, buf);
        vsLog(buf);
        vsAssert(success,"Unable to compile fragment shader.\n");
    }

    program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, sizeof(buf), 0, buf);
        vsLog(buf);
        vsAssert(success,"Unable to link shaders.\n");
    }
	glDetachShader(program,vertShader);
	glDetachShader(program,fragShader);
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

    return program;
}

void
vsRenderer_OpenGL2::DestroyShader(GLuint shader)
{
	glDeleteProgram(shader);
}


vsImage *
vsRenderer_OpenGL2::Screenshot()
{
	const size_t bytesPerPixel = 3;	// RGB
	const size_t imageSizeInBytes = bytesPerPixel * size_t(m_widthPixels) * size_t(m_heightPixels);

	uint8_t* pixels = new uint8_t[imageSizeInBytes];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, m_widthPixels, m_heightPixels, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	CheckGLError("glReadPixels");


	vsImage *image = new vsImage( m_widthPixels, m_heightPixels );

	for ( int y = 0; y < m_heightPixels; y++ )
	{
		int rowStart = y * m_widthPixels * bytesPerPixel;

		for ( int x = 0; x < m_widthPixels; x++ )
		{
			int rInd = rowStart + (x*bytesPerPixel);
			int gInd = rInd+1;
			int bInd = rInd+2;

			int rVal = pixels[rInd];
			int gVal = pixels[gInd];
			int bVal = pixels[bInd];

			image->Pixel(x,y).Set( rVal/255.f, gVal/255.f, bVal/255.f, 1.0f );
		}
	}

	vsDeleteArray( pixels );

	return image;
}

vsImage *
vsRenderer_OpenGL2::ScreenshotDepth()
{
#if !TARGET_OS_IPHONE
	int imageSize = sizeof(float) * m_widthPixels * m_heightPixels;

	float* pixels = new float[imageSize];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, m_widthPixels, m_heightPixels, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
	CheckGLError("glReadPixels");


	vsImage *image = new vsImage( m_widthPixels, m_heightPixels );

	for ( int y = 0; y < m_heightPixels; y++ )
	{
		int rowStart = y * m_widthPixels;

		for ( int x = 0; x < m_widthPixels; x++ )
		{
			int ind = rowStart + x;

			float val = pixels[ind];

			image->Pixel(x,y).Set( val, val, val, 1.0f );
		}
	}

	vsDeleteArray( pixels );

	return image;
#else
	return NULL;
#endif
}

vsImage *
vsRenderer_OpenGL2::ScreenshotAlpha()
{
	int imageSize = sizeof(float) * m_widthPixels * m_heightPixels;

	float* pixels = new float[imageSize];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, m_widthPixels, m_heightPixels, GL_ALPHA, GL_FLOAT, pixels);
	CheckGLError("glReadPixels");


	vsImage *image = new vsImage( m_widthPixels, m_heightPixels );

	for ( int y = 0; y < m_heightPixels; y++ )
	{
		int rowStart = y * m_widthPixels;

		for ( int x = 0; x < m_widthPixels; x++ )
		{
			int ind = rowStart + x;

			float val = pixels[ind];

			image->Pixel(x,y).Set( val, val, val, 1.0f );
		}
	}

	vsDeleteArray( pixels );

	return image;
}

bool
vsRenderer_OpenGL2::PreRenderTarget( const vsRenderer::Settings &s, vsRenderTarget *target )
{
	target->Bind();
	if ( m_antialias )
	{
		m_state.SetBool( vsRendererState::Bool_Multisample, true );
	}
	m_state.SetBool( vsRendererState::Bool_DepthMask, true );
	m_state.Flush();
	glClearColor(0.f,0.f,0.f,0.f);
	glClearDepth(1.f);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	return true;
}

bool
vsRenderer_OpenGL2::PostRenderTarget( vsRenderTarget *target )
{
	m_scene->Bind();
	return true;
}

#ifdef CHECK_GL_ERRORS
void
vsRenderer_OpenGL2::CheckGLError(const char* string)
{
    char enums[][20] =
    {
        "invalid enumeration", // GL_INVALID_ENUM
        "invalid value",       // GL_INVALID_VALUE
        "invalid operation",   // GL_INVALID_OPERATION
        "stack overflow",      // GL_STACK_OVERFLOW
        "stack underflow",     // GL_STACK_UNDERFLOW
        "out of memory"        // GL_OUT_OF_MEMORY
    };

    GLenum errcode = glGetError();
    if (errcode == GL_NO_ERROR)
        return;

    errcode -= GL_INVALID_ENUM;

	vsString errString = vsFormatString("OpenGL %s in '%s'", enums[errcode], call);
	vsLog(errString);
    vsAssert(false,errString);
}
#endif

#define STRINGIFY(A)  #A

const char *normalv = STRINGIFY(
									   uniform bool fog;
									   varying float fogFactor;
									   void main(void)
									   {
										   gl_FrontColor = gl_Color;
										   gl_Position    = ftransform();

										   fogFactor = 1.0;
										   if ( fog )
										   {
										   const float LOG2 = 1.442695;
										   vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
										   float distance = length(vVertex);
										   fogFactor = exp2( -gl_Fog.density *
															gl_Fog.density *
															distance *
															distance *
															LOG2 );
										   fogFactor = clamp(fogFactor, 0.0, 1.0);
										   }
									   }
									   );

const char *texv = STRINGIFY(
									uniform bool fog;
									varying float fogFactor;
									void main(void)
									{
										gl_TexCoord[0] = gl_MultiTexCoord0;
										gl_FrontColor = gl_Color;
										gl_Position    = ftransform();

										fogFactor = 1.0;
										if ( fog )
										{
										const float LOG2 = 1.442695;
										vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
										float distance = length(vVertex);
										fogFactor = exp2( -gl_Fog.density *
														 gl_Fog.density *
														 distance *
														 distance *
														 LOG2 );
										fogFactor = clamp(fogFactor, 0.0, 1.0);
										}
									}
									);

const char *litv = STRINGIFY(
									varying vec4 diffuse;
									varying vec4 ambient;
									varying vec3 normal;
									varying vec3 lightDir;
									varying vec3 halfVector;
									varying float fogFactor;
									uniform bool fog;

									void main()
									{
										gl_TexCoord[0] = gl_MultiTexCoord0;
										/* first transform the normal into eye space and
										 normalize the result */
										normal = normalize(gl_NormalMatrix * gl_Normal);

										/* now normalize the light's direction. Note that
										 according to the OpenGL specification, the light
										 is stored in eye space. Also since we're talking about
										 a directional light, the position field is actually direction */
										lightDir = normalize(vec3(gl_LightSource[0].position));

										/* Normalize the halfVector to pass it to the fragment shader */
										halfVector = normalize(gl_LightSource[0].halfVector.xyz);

										/* Compute the diffuse, ambient and globalAmbient terms */
										diffuse = /*gl_FrontMaterial.diffuse*/ gl_Color * gl_LightSource[0].diffuse;
										ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
										ambient += gl_LightModel.ambient * gl_FrontMaterial.ambient;

										gl_Position = ftransform();

										fogFactor = 1.0;
										if ( fog )
										{
										const float LOG2 = 1.442695;
										vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
										float distance = length(vVertex);
										fogFactor = exp2( -gl_Fog.density *
														 gl_Fog.density *
														 distance *
														 distance *
														 LOG2 );
										fogFactor = clamp(fogFactor, 0.0, 1.0);
										}
									}
									);

const char *litf = STRINGIFY(
									varying vec4 diffuse;
									varying vec4 ambient;
									varying vec3 normal;
									varying vec3 lightDir;
									varying vec3 halfVector;
									varying float fogFactor;

									void main()
									{
										vec3 n;
										vec3 halfV;
										float NdotL;
										float NdotHV;

										/* The ambient term will always be present */
										vec4 color = diffuse * ambient;

										/* a fragment shader can't write a varying variable, hence we need
										 a new variable to store the normalized interpolated normal */
										n = normalize(normal);

										/* compute the dot product between normal and ldir */
										NdotL = max(dot(n,lightDir)+1.0,0.0) * 0.5;
										color += diffuse * NdotL;

										if (NdotL > 0.0) {
											halfV = normalize(halfVector);
											NdotHV = max(dot(n,halfV),0.0);
											color.rgb += gl_FrontMaterial.specular.rgb *
													gl_LightSource[0].specular.rgb *
													pow(NdotHV, gl_FrontMaterial.shininess);
										}

										gl_FragColor.rgb = mix(gl_Fog.color.rgb, color.rgb, fogFactor );
										gl_FragColor.a = color.a;
									}
									);
/*
static const char *litTexv = STRINGIFY(
									   void main(void)
									   {
										   vec3 Normal = gl_NormalMatrix * gl_Normal;

										   vec3 Light = gl_LightSource[0].position.xyz;//normalize();

										   float Diffuse = max(dot(Normal, Light),0.0);

										   gl_TexCoord[0] = gl_MultiTexCoord0;
										   gl_FrontColor = gl_Color;
										   gl_FrontColor.rgb *= Diffuse;
										   gl_Position = ftransform();
									   }
									   );
*/
const char *litTexf = STRINGIFY(
									   uniform sampler2D source;
									   uniform float alphaRef;

									   varying vec4 diffuse;
									   varying vec4 ambient;
									   varying vec3 normal;
									   varying vec3 lightDir;
									   varying vec3 halfVector;
									   varying float fogFactor;

									   void main()
									   {
										   vec3 n;
										   vec3 halfV;
										   float NdotL;
										   float NdotHV;

										   /* The ambient term will always be present */
										   vec4 color = diffuse * ambient;
										   vec4 textureSample = texture2D(source, gl_TexCoord[0].st);
										   if ( textureSample.a < alphaRef )
											   discard;

										   /* a fragment shader can't write a varying variable, hence we need
											a new variable to store the normalized interpolated normal */
										   n = normalize(normal);

										   /* compute the dot product between normal and ldir */
										   //NdotL = max(dot(n,lightDir),0.0);
										   NdotL = max(dot(n,lightDir)+1.0,0.0) * 0.5;
										   color += diffuse * NdotL;

										   if (gl_FrontMaterial.shininess > 0.0 && NdotL > 0.0) {
											   halfV = normalize(halfVector);
											   NdotHV = max(dot(n,halfV),0.0);
											   color.rgb += gl_FrontMaterial.specular.rgb *
											   gl_LightSource[0].specular.rgb *
											   pow(NdotHV, gl_FrontMaterial.shininess);
										   }

										   gl_FragColor.rgb = mix(gl_Fog.color.rgb, color.rgb * textureSample.rgb, fogFactor );
										   gl_FragColor.a = color.a * textureSample.a;
									   }
);


const char *normalf = STRINGIFY(
									   varying float fogFactor;
									   void main(void)
									   {
										   gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_Color.rgb, fogFactor );
										   gl_FragColor.a = gl_Color.a;
									   }
);

const char *texf = STRINGIFY(
									uniform sampler2D source;
									uniform float alphaRef;
									varying float fogFactor;

									void main(void)
									{
										vec4 color = texture2D(source, gl_TexCoord[0].st);
										if ( color.a < alphaRef )
											discard;
										gl_FragColor.rgb = mix(gl_Fog.color.rgb, color.rgb * gl_Color.rgb, fogFactor );
										gl_FragColor.a = color.a * gl_Color.a;
									}
);

