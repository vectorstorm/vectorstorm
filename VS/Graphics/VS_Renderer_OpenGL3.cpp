/*
 *
 *  VS_Renderer_OpenGL3.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */


#include "VS_Renderer_OpenGL3.h"

#include "VS_Camera.h"
#include "VS_Debug.h"
#include "VS_DisplayList.h"
#include "VS_Image.h"
#include "VS_MaterialInternal.h"
#include "VS_Matrix.h"
#include "VS_RenderBuffer.h"
#include "VS_RenderTarget.h"
#include "VS_Screen.h"
#include "VS_Shader.h"
// #include "VS_ShaderRef.h"
#include "VS_ShaderSuite.h"
#include "VS_System.h"
#include "VS_Texture.h"
#include "VS_TextureInternal.h"
#include "VS_Profile.h"

#include "VS_Mutex.h"

#include "VS_OpenGL.h"

#include "VS_DynamicBatchManager.h"
#include "VS_TimerSystem.h"

#include "VS_Input.h" // flag event queue to ignore resize events while we're changing window type
#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
// #include "tracy/TracyOpenGL.hpp"
#endif // VS_TRACY

#if SDL_VERSION_ATLEAST(2,0,5)
#define HAS_SDL_SET_RESIZABLE
#endif

#include <unordered_set>


#if TARGET_OS_IPHONE

#define glClearDepth( a ) glClearDepthf( a )
#define glFogi( a, b ) glFogx( a, b )
#define glTexParameteri( a, b, c ) glTexParameterx( a, b, c )

#endif

#ifdef _WIN32
// workaround for NVidia Optimus systems not correctly switching
// to the discrete NVidia chipset when starting up.  Tell them
// that we want to run on the NVidia card by exporting this global
// variable.
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

namespace
{
	vsMaterial *s_previousMaterial = nullptr;
	vsShaderValues *s_previousShaderValues = nullptr;


}
uint32_t currentlyBoundTexture[MAX_TEXTURE_SLOTS] = {0};


SDL_Window *g_sdlWindow = nullptr;
static SDL_GLContext m_sdlGlContext;
static SDL_GLContext m_loadingGlContext;
static vsMutex m_loadingGlContextMutex;

bool g_crashOnTextureStateUsageWarning = false;

void vsRenderDebug( const vsString &message )
{
	vsLog("%s", message.c_str());
}

// The following code for adapting from GLenums to strings
// was written by Timo Suoranta, from:
// > https://github.com/tksuoran/RenderStack/blob/master/libraries/renderstack_graphics/source/configuration.cpp
//
// It is used here under the terms of the zlib/libpng license.
//
static const char * desc_debug_source(GLenum source)
{
	switch (source)
	{
		case GL_DEBUG_SOURCE_API            : return "api";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM  : return "window system";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
		case GL_DEBUG_SOURCE_THIRD_PARTY    : return "third party";
		case GL_DEBUG_SOURCE_APPLICATION    : return "application";
		case GL_DEBUG_SOURCE_OTHER          : return "other";
		default: return "?";
	}
}

static const char * desc_debug_type(GLenum type)
{
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR              : return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behavior";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR : return "undefined behavior";
		case GL_DEBUG_TYPE_PORTABILITY        : return "portability";
		case GL_DEBUG_TYPE_PERFORMANCE        : return "performance";
		case GL_DEBUG_TYPE_OTHER              : return "other";
		case GL_DEBUG_TYPE_MARKER             : return "marker";
		case GL_DEBUG_TYPE_PUSH_GROUP         : return "push group";
		case GL_DEBUG_TYPE_POP_GROUP          : return "pop group";
		default: return "?";
	}
}

static const char * desc_debug_severity(GLenum severity)
{
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH  : return "high";
		case GL_DEBUG_SEVERITY_MEDIUM: return "medium";
		case GL_DEBUG_SEVERITY_LOW   : return "low";
		default: return "?";
	}
}

void vsOpenGLDebugMessage( GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar *message,
		const GLvoid *userParam)
{
	// NVidia debug message spam.
	if (id == 0x00020071) return; // memory usage
								  // if (id == 0x00020084) return; // Texture state usage warning: Texture 0 is base level inconsistent. Check texture size.
	if (id == 0x00020061) return; // Framebuffer detailed info: The driver allocated storage for renderbuffer 1.
	if (id == 0x00020004) return; // Usage warning: Generic vertex attribute array ... uses a pointer with a small value (...). Is this intended to be used as an offset into a buffer object?
	if (id == 0x00020072) return; // Buffer performance warning: Buffer object ... (bound to ..., usage hint is GL_STATIC_DRAW) is being copied/moved from VIDEO memory to HOST memory.
	if (id == 0x00020074) return; // Buffer usage warning: Analysis of buffer object ... (bound to ...) usage indicates that the GPU is the primary producer and consumer of data for this buffer object.  The usage hint s upplied with this buffer object, GL_STATIC_DRAW, is inconsistent with this usage pattern.  Try using GL_STREAM_COPY_ARB, GL_STATIC_COPY_ARB, or GL_DYNAMIC_COPY_ARB instead.

	// Intel debug message spam.
	if (id == 0x00000008) return; // API_ID_REDUNDANT_FBO performance warning has been generated. Redundant state change in glBindFramebuffer API call, FBO 0, "", already bound.

	// Program/shader being recompiled spam.
	if (id == 0x00020092) return;

	vsLog("GL: id 0x%x, source: %s, type: %s, severity %s, %s", id, desc_debug_source(source), desc_debug_type(type), desc_debug_severity(severity), message);

	if (id == 0x00020084 && g_crashOnTextureStateUsageWarning )
	{
		vsAssert(0, "Texture state usage warning");
	}
}

namespace
{
	vsString FormatKB(float kb)
	{
		if ( kb < 1024 )
			return vsFormatString("%0.2f KB", kb);
		else if ( kb < 1024 * 1024 )
		{
			float mb = kb / (1024.f );
			return vsFormatString("%0.2f MB", (float)mb);
		}

		float gb = kb / (1024.f * 1024.f );
		return vsFormatString("%0.2f GB", (float)gb);
	}
	// vsString FormatBytes(uint64_t bytes)
	// {
	// 	float kb = (float)bytes / 1024.f;
	// 	return FormatKB(kb);
	// }
};

static void printAttributes ()
{
#if !TARGET_OS_IPHONE
	// Print out attributes of the context we created

	vsLog("OpenGL Context:");
	vsLog("  Vendor: %s", glGetString(GL_VENDOR));
	vsLog("  Renderer: %s", glGetString(GL_RENDERER));
	vsLog("  Version: %s", glGetString(GL_VERSION));
	vsLog("  GLEW: %s", glewGetString(GLEW_VERSION));
	if ( glGetString(GL_SHADING_LANGUAGE_VERSION) )
	{
		vsLog("  Shading Language Version:  %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	}
	else
	{
		vsLog("  Shader Langugage Version:  None");
	}

	vsLog("== Begin OpenGL limits ==");
	struct attr
	{
		GLenum name;
		const char* label;
	};
	static const attr a[] =
	{
		{GL_MAX_3D_TEXTURE_SIZE, "Max 3D texture size"},
		{GL_MAX_ARRAY_TEXTURE_LAYERS, "Max array texture layers"},
		{GL_MAX_CLIP_DISTANCES, "Max clip distances"},
		{GL_MAX_COLOR_TEXTURE_SAMPLES, "Max samples in a color multisample texture"},
		{GL_MAX_COMBINED_ATOMIC_COUNTERS, "Maximum atomic counters"},
		{GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, "Maximum fragment shader uniform components"},
		{GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, "Maximum geometry shader uniform components"},
		{GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, "Maximum combined texture image units"},
		{GL_MAX_COMBINED_UNIFORM_BLOCKS, "Maximum combined uniform blocks"},
		{GL_MAX_CUBE_MAP_TEXTURE_SIZE, "Maximum cube map dimensions"},
		{GL_MAX_DEPTH_TEXTURE_SAMPLES, "Maximum samples in a multisample depth or depth-stencil texture"},
		{GL_MAX_DRAW_BUFFERS, "Maximum simultaneous draw buffers"},
		{GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, "Maximum simultaneous draw buffers with dual-source blending"},
		{GL_MAX_ELEMENTS_INDICES, "Recommended maximum number of vertex array indices"},
		{GL_MAX_ELEMENTS_VERTICES, "Recommended maximum number of vertex array vertices"},
		{GL_MAX_FRAGMENT_INPUT_COMPONENTS, "Max fragment shader input components"},
		{GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, "Max fragment shader uniform components"},
		{GL_MAX_TEXTURE_BUFFER_SIZE, "Maximum texels in the texel array of a texture buffer"},
		{GL_MAX_TEXTURE_IMAGE_UNITS, "Maximum supported texture image units in a fragment shader"},
		{GL_MAX_TEXTURE_SIZE, "Maximum texture size (rough estimate)"},
		{GL_MAX_UNIFORM_BUFFER_BINDINGS, "Maximum uniform buffer binding points"},
		{GL_MAX_UNIFORM_BLOCK_SIZE, "Maximum uniform block size"},
		{GL_MAX_UNIFORM_LOCATIONS, "Maximum uniform locations"},
		{GL_MAX_VARYING_COMPONENTS, "Maximum varying components"},
		{GL_MAX_VARYING_FLOATS, "Maximum floating point varying components"},
		{GL_MAX_VERTEX_ATOMIC_COUNTERS, "Maximum atomic counters in vertex shaders"},
		{GL_MAX_VERTEX_ATTRIBS, "Maximum vertex attributes in vertex shader"},
		{GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, "Maximum active shader storage blocks in a vertex shader"},
		{GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, "Maximum texture image units in vertex shader"},
		{GL_MAX_VERTEX_UNIFORM_COMPONENTS, "Maximum uniform components in vertex shader"},
		{GL_MAX_VERTEX_OUTPUT_COMPONENTS, "Maximum output components in vertex shader"},
		{GL_MAX_VERTEX_UNIFORM_BLOCKS, "Maximum uniform blocks per vertex shader"},
		{GL_MAX_VIEWPORTS, "Maximum simultaneous viewports"},
		{GL_MAX_SAMPLES, "Maximum MSAA samples"},
		{GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, "NVidia-only:  Currently available video memory"},
		{GL_TEXTURE_FREE_MEMORY_ATI, "ATI-only:  Currently available video memory"}
	};
	int nAttr = sizeof(a) / sizeof(struct attr);
	//
	GLint value;
	for (int i = 0; i < nAttr; i++)
	{
		glGetIntegerv( a[i].name, &value );

		if ( a[i].name == GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX )
			vsLog("%s: %s", a[i].label, FormatKB(value) );
		else
			vsLog("%s: %d", a[i].label, value );
	}

	// now clear the GL error state;  one of the texture memory stats will
	// probably have generated an "invalid enum" error, since those texture
	// memory queries are vendor-specific OpenGL extensions.
	glGetError();

	vsLog("== End OpenGL limits ==");

	if ( GL_EXT_texture_filter_anisotropic )
		vsLog("anisotropic texture filters: SUPPORTED");
	else
		vsLog("anisotropic texture filters: UNSUPPORTED");
#endif // TARGET_OS_IPHONE
}



//static bool s_vertexBuffersSupported = false;

vsRenderer_OpenGL3::vsRenderer_OpenGL3(int width, int height, int depth, int flags, int bufferCount):
	vsRenderer(width, height, depth, flags),
	m_flags(flags),
	m_currentLocalToWorld(nullptr),
	m_currentLocalToWorldBuffer(nullptr),
	m_currentColors(nullptr),
	m_currentColorsBuffer(nullptr),
	m_window(nullptr),
	m_scene(nullptr),
	m_currentRenderTarget(nullptr),
	m_currentMaterial(nullptr),
	m_currentMaterialInternal(nullptr),
	m_currentShader(nullptr),
	m_currentShaderValues(nullptr),
	m_currentVertexArray(nullptr),
	m_currentNormalArray(nullptr),
	m_currentTexelArray(nullptr),
	m_currentColorArray(nullptr),
	m_currentVertexBuffer(nullptr),
	m_currentNormalBuffer(nullptr),
	m_currentTexelBuffer(nullptr),
	m_currentColorBuffer(nullptr),
	m_lastShaderId(0),
	m_bufferCount(bufferCount)
{
	vsLog("SDL Compiled Version: %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
	SDL_version sdlVersion;
	SDL_GetVersion(&sdlVersion);
	vsLog("SDL Runtime Version: %d.%d.%d", sdlVersion.major, sdlVersion.minor, sdlVersion.patch);


	int displayCount = SDL_GetNumVideoDisplays();
	if (displayCount < 1)
	{
		fprintf(stderr, "%s\nExiting...\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}
	vsLog("Found %d displays:", displayCount);
	for(int i = 0; i < displayCount; i++)
	{
		SDL_Rect bounds;
		if(SDL_GetDisplayBounds(i, &bounds) == 0)
			vsLog("Display #%d %s (%dx%d)", i, SDL_GetDisplayName(i), bounds.w , bounds.h );
	}

	int x = SDL_WINDOWPOS_UNDEFINED;
	int y = SDL_WINDOWPOS_UNDEFINED;

#if !TARGET_OS_IPHONE
	//const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
	int videoFlags;

	videoFlags = SDL_WINDOW_OPENGL;
#ifdef HIGHDPI_SUPPORTED
	if ( flags & Flag_HighDPI )
	{
		videoFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
		vsLog("videoFlag added: AllowHighDPI");
	}
#endif

	if ( flags & Flag_Fullscreen )
	{
		if ( flags & Flag_FullscreenWindow )
		{
			videoFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			m_windowType = WindowType_FullscreenWindow;
			vsLog("videoFlag added: Fullscreen Desktop");
		}
		else
		{
			videoFlags |= SDL_WINDOW_FULLSCREEN;
			m_windowType = WindowType_Fullscreen;
			vsLog("videoFlag added: Fullscreen");
		}
	}
	else
	{
		m_windowType = WindowType_Window;
		videoFlags |= SDL_WINDOW_RESIZABLE;
		vsLog("videoFlag added: Resizable");
	}

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	// SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	// no depth buffer on our output target -- we don't render into it directly.
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#if defined(VS_GL_DEBUG)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif // VS_GL_DEBUG

	// int shareVal;
	vsLog("SDL_CreateWindow %dx%dx%d video mode, flags %d", width, height, depth, videoFlags);
	g_sdlWindow = SDL_CreateWindow("", x, y, width, height, videoFlags);
	if ( !g_sdlWindow ){
		vsString errorMsg( SDL_GetError() );
		vsLog("Couldn't set %dx%dx%d video mode: %s;  trying to fallback to 1024x768 window.",
				width, height, depth, errorMsg );

		// Let's try plain simple window mode.
		videoFlags = SDL_WINDOW_OPENGL;
		flags = 0;
		g_sdlWindow = SDL_CreateWindow("", x, y, 1024, 768, videoFlags);
		if ( !g_sdlWindow )
		{
			vsAssertF(0, "Couldn't set %dx%dx%d video mode: %s\n",
					width, height, depth, errorMsg );
			exit(1);
		}
	}
	if ( flags & Flag_FullscreenWindow )
	{
		// with FullscreenWindow, we don't use the specified width/height values,
		// but instead SET them based upon window we created

		SDL_GetWindowSize( g_sdlWindow, &width, &height );
		// SDL_SetWindowSize( g_sdlWindow, width, height );

		// int index = SDL_GetWindowDisplayIndex( g_sdlWindow );
		// SDL_SetWindowPosition( g_sdlWindow,
		// 		SDL_WINDOWPOS_CENTERED_DISPLAY(index),
		// 		SDL_WINDOWPOS_CENTERED_DISPLAY(index)
		// 		);

		// lets check which display we're on.
		// if ( displayCount > 1 )
		// {
		// 	// TODO:  Let's maybe make this data-driven, somehow.  Via a 'preferences'
		// 	// object, maybe?  Or alternately, the host game could tell us where to
		// 	// put the window, maybe?
		//
		// 	flags |= Flag_Fullscreen;
		// 	x = SDL_WINDOWPOS_CENTERED_DISPLAY(1);
		// 	y = SDL_WINDOWPOS_CENTERED_DISPLAY(1);
		// 	SDL_DisplayMode mode;
		// 	SDL_Rect bounds;
		// 	SDL_GetDesktopDisplayMode(1, &mode);
		// 	if ( SDL_GetDisplayBounds(1, &bounds) == 0 )
		// 	{
		// 		x = bounds.x;
		// 		y = bounds.y;
		// 		width = bounds.w;
		// 		height = bounds.h;
		// 	}
		// }
		// #endif
	}


	m_viewportWidth = m_width = width;
	m_viewportHeight = m_height = height;
	if ( m_windowType == WindowType_Fullscreen )
	{
		SDL_SetWindowGrab(g_sdlWindow,SDL_TRUE);
	}

	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
	m_loadingGlContext = SDL_GL_CreateContext(g_sdlWindow);
	if ( !m_loadingGlContext )
	{
		vsLog("Failed to create OpenGL context for loading??");
		vsAssertF(0, "Failed to create an OpenGL 3.3 context. OpenGL 3.3 support is required for this game.  SDL2 error message: %s", SDL_GetError() );
		exit(1);
	}

	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	m_sdlGlContext = SDL_GL_CreateContext(g_sdlWindow);
	if ( !m_sdlGlContext )
	{
		vsLog("Failed to create OpenGL context??");
		vsAssertF(0, "Failed to create an OpenGL 3.3 context. OpenGL 3.3 support is required for this game.  SDL2 error message: %s", SDL_GetError() );
		exit(1);
	}

	printAttributes();

	vsLog("Post printAttributes");
	m_widthPixels = width;
	m_heightPixels = height;
	m_currentViewportPixels.Set(
			vsVector2D::Zero,
			vsVector2D( m_widthPixels, m_heightPixels )
			);
#ifdef HIGHDPI_SUPPORTED
	vsLog("HighDPI Supported?");
	if ( flags & Flag_HighDPI )
		SDL_GL_GetDrawableSize(g_sdlWindow, &m_widthPixels, &m_heightPixels);
	vsLog("Post HighDPI Supported?");
#endif
	vsLog("SetWindowTitlePre");
	// SDL_SetWindowMinimumSize(g_sdlWindow, 1024, 768);
	SDL_SetWindowTitle( g_sdlWindow, vsSystem::Instance()->GetTitle().c_str() );
	vsLog("SetWindowTitlePost");

	vsLog("GetDoubleBuffered");
	int isDoubleBuffered = 0;
	SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &isDoubleBuffered );
	if (!isDoubleBuffered)
	{
		vsLog("FAILED TO SET UP DOUBLE BUFFERING??");
	}
	else
		vsLog("DOUBLE-BUFFERED");
	// shareVal = SDL_GL_GetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, &shareVal );
	// if ( shareVal )
	// {
	// 	vsLog("Supports loading context!");
	// }
	// else
	// {
	// 	vsAssert(0, "No shared context supported");
	// }
	m_viewportWidthPixels = m_widthPixels;
	m_viewportHeightPixels = m_heightPixels;
	if ( m_width != m_widthPixels ||
			m_height != m_heightPixels )
	{
		vsLog("High DPI Rendering enabled");
		vsLog("High DPI backing store is: %dx%d", m_widthPixels, m_heightPixels);
	}


	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	vsAssert(GLEW_OK == err, vsFormatString("GLEW error: %s", glewGetErrorString(err)).c_str());

	GLint major = 0;
	GLint minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	if ( major < 3 || (major == 3 && minor < 3) )
	{
		vsString errorString = vsFormatString("No support for OpenGL 3.3 (maximum version supported: %d.%d).  Cannot run.  Try updating your display drivers?", major, minor);
		bool supportsOpenGL33 = false;
		vsAssert( supportsOpenGL33, errorString );
	}

	m_antialias = (flags & Flag_Antialias) != 0;
	m_vsync = (flags & Flag_VSync) != 0;

	if ( SDL_GL_SetSwapInterval(flags & Flag_VSync ? 1 : 0) == -1 )
		vsLog("Couldn't set vsync");

#if defined(VS_GL_DEBUG)
	if ( glDebugMessageCallback )
	{
		vsLog("DebugMessageCallback:  SUPPORTED");
		glDebugMessageCallback( (GLDEBUGPROC)&vsOpenGLDebugMessage, nullptr );
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	int swapInterval = SDL_GL_GetSwapInterval();
	if ( swapInterval > 0 )
		vsLog( "VSync: ENABLED" );
	else if ( swapInterval == 0 )
		vsLog( "VSync: DISABLED" );
	else
		vsLog( "VSync: UNKNOWN (the active OpenGL driver provides no support for querying swap interval)" );

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

	// SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &val );
	// if ( !val )
	// 	vsLog("WARNING:  Failed to get stencil buffer bits");

#endif // !TARGET_OS_IPHONE

	GL_CHECK("Initialising OpenGL rendering");
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
#if !TARGET_OS_IPHONE
	glClearDepth( 1.0f );  // arbitrary large value
#endif // !TARGET_OS_IPHONE
	GL_CHECK("Initialising OpenGL rendering");

	glBlendFunc(GL_SRC_ALPHA,GL_ONE);							// Set The Blending Function For Additive
	glEnable(GL_BLEND);											// Enable Blending
	GL_CHECK("Initialising OpenGL rendering");

	m_state.SetBool( vsRendererState::Bool_DepthTest, true );
	glDepthFunc( GL_LEQUAL );
	GL_CHECK("Initialising OpenGL rendering");

	glViewport( 0, 0, (GLsizei)m_widthPixels, (GLsizei)m_heightPixels );
	GL_CHECK("Initialising OpenGL rendering");

	m_defaultShaderSuite = new vsShaderSuite;
	m_defaultShaderSuite->InitShaders("default_v.glsl", "default_f.glsl", vsShaderSuite::OwnerType_System);
	GL_CHECK("Initialising OpenGL rendering");


	// TEMP VAO IMPLEMENTATION
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	GL_CHECK("Initialising OpenGL rendering");

	ResizeRenderTargetsToMatchWindow();

	GL_CHECK("Initialising OpenGL rendering");

	// now set our OpenGL state to our expected defaults.
	m_state.Force();

	// Disable the "minimize on focus loss" hint entirely.
	//
	// Old laptops with old graphic drivers often seem to have problems with
	// minimizing/restoring OpenGL applications, so let's just not
	// automatically ask them to do that.
	//
	//  - Trevor (20/6/2024)
	//
	// #if !defined(__APPLE_CC__)
	// 	// For some reason, address sanitizer mode is picking up illegal memory
	// 	// accesses from inside SDL_SetHint on OSX.  For now, let's just disable
	// 	// this call since it isn't particularly important;  can follow-up with
	// 	// the SDL folks and check for proper fixes once we're all up and working
	// 	// again!
	// 	{
	// 		bool minimizeOnFocusLoss = ( displayCount == 1 );
	// 		char doit = (minimizeOnFocusLoss) ? 1 : 0;
	// 		SDL_bool retval = SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, &doit );
	// 		if ( !retval )
	// 			vsLog("Trying to set minimize on focus loss hint failed");
	// 		else
	// 		{
	// 			if ( doit )
	// 				vsLog("Hinted minimise on focus loss: TRUE (as there is only one monitor)");
	// 			else
	// 				vsLog("Hinted minimise on focus loss: FALSE (as there are %d monitors)", displayCount);
	// 		}
	// 	}
	// #endif // __APPLE_CC__

	DetermineRefreshRate();
#ifdef VS_TRACY
	// TracyGpuContext;
#endif // VS_TRACY

}

vsRenderer_OpenGL3::~vsRenderer_OpenGL3()
{
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &m_vao);

	vsDelete( m_defaultShaderSuite );

	SDL_GL_DeleteContext( m_sdlGlContext );
	SDL_GL_DeleteContext( m_loadingGlContext );
	SDL_DestroyWindow( g_sdlWindow );
	g_sdlWindow = nullptr;
}

void
vsRenderer_OpenGL3::Deinit()
{
	// we need to destroy our render targets here, so this can be called
	// before the texture manager is shut down.
	vsDelete(m_window);
	vsDelete(m_scene);
}

void
vsRenderer_OpenGL3::DetermineRefreshRate()
{
	SDL_DisplayMode mode;
	int err = SDL_GetWindowDisplayMode(g_sdlWindow, &mode);
	if ( err == 0 )
	{
		vsLog("Display refresh rate: %d", mode.refresh_rate);
		m_refreshRate = mode.refresh_rate;
	}
	else
	{
		vsLog("Error getting display mode: %s", SDL_GetError());
	}
}

void
vsRenderer_OpenGL3::ResizeRenderTargetsToMatchWindow()
{
	GL_CHECK_SCOPED("vsRenderer_OpenGL3::ResizeRenderTargetsToMatchWindow");

	int width = GetWidthPixels();
	int height = GetHeightPixels();

	if ( m_window )
	{
		// We don't *actually* have to resize this.  It's not a *real* render target.
		m_window->Resize(width, height);
	}
	else
	{
		vsDelete( m_window );

		// Create Window Surface
		vsSurface::Settings settings;
		settings.depth = false;
		settings.width = GetWidthPixels();
		settings.height = GetHeightPixels();
		m_window = new vsRenderTarget( vsRenderTarget::Type_Window, settings );
	}

	if ( m_scene )
	{
		m_scene->Resize(width, height);
	}
	else
	{
		vsDelete( m_scene );

		// Create 3D Scene Surface
		// we want to be big enough to hold our full m_window resolution, and set our viewport to match the window.

		vsSurface::Settings settings;
		settings.bufferSettings[2].format = vsSurface::Format_HalfFloat;
		settings.width = GetWidthPixels();
		settings.height = GetHeightPixels();
		settings.depth = true;
		settings.mipMaps = false;
		settings.stencil = true;
		settings.buffers = m_bufferCount;

		if ( m_antialias )
		{
			m_scene = new vsRenderTarget( vsRenderTarget::Type_Multisample, settings );
		}
		else
		{
			m_scene = new vsRenderTarget( vsRenderTarget::Type_Texture, settings );
		}
	}
	SetRenderTarget(m_scene);
	m_lastShaderId = 0;
	glUseProgram(0);
	glClearDepth( 1.0 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	SetViewportWidthPixels( m_scene->GetViewportWidth() );
	SetViewportHeightPixels( m_scene->GetViewportHeight() );

	// [TODO] We're in an awkward spot here where we're using the term 'viewport'
	// (and 'scene') to mean too many things.
	//
	// Above this comment, 'm_scene' is the render target that represents our
	// window.  It's the framebuffer device where we display what we render.
	// And its dimensions in pixels are its "ViewportWidth" and
	// "ViewportHeight".  (Where this is distinct from 'Width' and 'Height' because
	// of the bad old days where textures had to be powers of 2 in size, and so
	// the actual backing storage might have one width while we were only
	// drawing into a differently sized chunk of it)
	//
	// Below this comment, a 'viewport' is a portion of a render target that we
	// want to render into.  And a 'scene' is a collection of vsEntities that
	// should be drawn, along with a camera, a viewport, and some other values.
	// I need to clean up this verbiage (probably by renaming the first set of
	// things;  perhaps the 'm_scene' should become 'm_display', and its
	// 'ViewportWidth' should just be 'Width'?  Or something like that?
	m_currentViewportPixels.Set(
			vsVector2D::Zero,
			vsVector2D( m_widthPixels, m_heightPixels )
			);
}

bool
vsRenderer_OpenGL3::CheckVideoMode()
{
	// if ( m_windowType == WindowType_FullscreenWindow )
	// {
	// 	SDL_SetWindowFullscreen(g_sdlWindow,SDL_FALSE); // swap out of fullscreen for a moment;  Linux builds don't swap cleanly.
	// 	SDL_Delay(1);
	//
	// 	int index = SDL_GetWindowDisplayIndex( g_sdlWindow );
	// 	SDL_SetWindowPosition( g_sdlWindow,
	// 			SDL_WINDOWPOS_CENTERED_DISPLAY(index),
	// 			SDL_WINDOWPOS_CENTERED_DISPLAY(index)
	// 			);
	//
	// 	// SDL_Rect bounds;
	// 	// if ( SDL_GetDisplayBounds(index, &bounds) == 0 )
	// 	// {
	// 	// 	float width = bounds.w;
	// 	// 	float height = bounds.h;
	// 	// 	SDL_SetWindowSize(g_sdlWindow, width, height);
	// 	// }
	// 	SDL_SetWindowFullscreen(g_sdlWindow,SDL_WINDOW_FULLSCREEN_DESKTOP); // swap out of fullscreen for a moment;  Linux builds don't swap cleanly.
	// }
	// 	int nowWidth, nowHeight;
	// 	SDL_GetWindowSize(g_sdlWindow, &nowWidth, &nowHeight);
	//
	// 	if ( nowWidth != m_width || nowHeight != m_height )
	// 	{
	// 		m_viewportWidth = m_width = m_widthPixels = nowWidth;
	// 		m_viewportHeight = m_height = m_heightPixels = nowHeight;
	// #ifdef HIGHDPI_SUPPORTED
	// 		if ( m_flags & Flag_HighDPI )
	// 			SDL_GL_GetDrawableSize(g_sdlWindow, &m_widthPixels, &m_heightPixels);
	// #endif
	// 		m_viewportWidthPixels = m_widthPixels;
	// 		m_viewportHeightPixels = m_heightPixels;
	// 		ResizeRenderTargetsToMatchWindow();
	// 	}
#ifdef HIGHDPI_SUPPORTED
	if ( m_flags & Flag_HighDPI )
	{
		int nowWidthPixels, nowHeightPixels;
		SDL_GL_GetDrawableSize(g_sdlWindow, &nowWidthPixels, &nowHeightPixels);
		if ( nowWidthPixels != m_widthPixels || nowHeightPixels != m_heightPixels )
		{
			UpdateVideoMode( m_width, m_height, true, m_windowType, m_bufferCount, m_antialias, m_vsync );
			return true;
		}
	}
#endif
	return false;
}

void
vsRenderer_OpenGL3::UpdateVideoMode(int width, int height, int depth, WindowType windowType, int bufferCount, bool antialias, bool vsync)
{
	vsLog("UPDATE VIDEO MODE:  %dx%dx%d, windowType %d", width, height, depth, windowType);
	UNUSED(depth);
	GL_CHECK_SCOPED("vsRenderer_OpenGL3::UpdateVideoMode");
	//vsAssert(0, "Not yet implemented");
	m_width = m_viewportWidth = width;
	m_height = m_viewportHeight = height;
	if ( m_antialias != antialias || m_bufferCount != bufferCount )
	{
		m_bufferCount = bufferCount;
		m_antialias = antialias;

		// 'm_scene' no longer matches the requested format;  rebuild it!
		vsDelete( m_scene );
	}
	m_vsync = vsync;
	bool changedWindowType = (m_windowType != windowType);

	// first, set window type.
	// if ( changedWindowType )
	{
		switch( windowType )
		{
			case WindowType_Window:
				SDL_SetWindowFullscreen(g_sdlWindow, 0);
				SDL_SetWindowGrab(g_sdlWindow,SDL_FALSE);
#ifdef HAS_SDL_SET_RESIZABLE
				if ( m_flags & Flag_Resizable )
				{
					SDL_SetWindowResizable(g_sdlWindow,SDL_TRUE);
				}
#endif
				vsLog("Setting window size %d, %d", width, height);
				SDL_SetWindowSize(g_sdlWindow, width, height);
				if ( changedWindowType )
					SDL_SetWindowPosition(g_sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
				break;
			case WindowType_Fullscreen:
				{
					if ( m_windowType == WindowType_Fullscreen ) // we're already in fullscreen!
					{
						SDL_SetWindowFullscreen(g_sdlWindow,SDL_FALSE); // swap out of fullscreen for a moment;  Linux builds don't swap cleanly.
						SDL_Delay(1);
					}
					SDL_DisplayMode target, closest;
					target.w = width;
					target.h = height;
					target.format = 0;
					target.refresh_rate = 0;
					target.driverdata = 0;

					if ( SDL_GetClosestDisplayMode(0, &target, &closest) == nullptr )
						vsLog("No suitable display mode for %dx%d found!", width, height);
					else
					{
						if ( SDL_SetWindowDisplayMode(g_sdlWindow, &closest) != 0 )
						{
							vsLog("SDL_SetWindowDisplayMode failed:  %s", SDL_GetError() );
							vsAssert(0, "Failed set display mode!");
						}
					}

					vsLog("Setting fullscreen window size %d, %d", m_width, m_height);
					m_viewportWidth = m_width = closest.w;
					m_viewportHeight = m_height = closest.h;
					SDL_SetWindowSize(g_sdlWindow, m_width, m_height);
					SDL_SetWindowPosition(g_sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

					SDL_SetWindowFullscreen(g_sdlWindow, SDL_WINDOW_FULLSCREEN);
					SDL_SetWindowGrab(g_sdlWindow,SDL_TRUE);
					break;
				}
			case WindowType_FullscreenWindow:
				SDL_SetWindowFullscreen(g_sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
				SDL_SetWindowGrab(g_sdlWindow,SDL_FALSE);

				int nowWidth, nowHeight;
				SDL_GL_GetDrawableSize(g_sdlWindow, &nowWidth, &nowHeight);
				vsLog("Fetching window size: %d, %d", nowWidth, nowHeight);
				m_viewportWidth = m_width = m_widthPixels = nowWidth;
				m_viewportHeight = m_height = m_heightPixels = nowHeight;
				break;
			default:
				break;
		}
	}

	// if ( m_windowType == WindowType_Window )
	// {
	// }
	// else if ( m_windowType == WindowType_Fullscreen )
	// else if ( m_windowType == WindowType_FullscreenWindow )
	// {
	// 	SDL_DisplayMode desktopMode;
	// 	if ( SDL_GetDesktopDisplayMode(0, &desktopMode) == 0 )
	// 	{
	// 		SDL_SetWindowDisplayMode(g_sdlWindow, &desktopMode);
	// 	}
	// 	else
	// 	{
	// 		vsLog("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError() );
	// 	}
	// 	SDL_SetWindowSize(g_sdlWindow, desktopMode.w, desktopMode.h);
	// }
	// if ( vsInput::Exists() )
	// {
	// 	vsLog("Before input pump");
	// 	vsInput::Instance()->Update(0.f);
	// 	vsLog("After input pump");
	// }
	//
	// now, set SIZE.

	m_windowType = windowType;

	// 	{
	// 		int nowWidth, nowHeight;
	// 		SDL_GetWindowSize(g_sdlWindow, &nowWidth, &nowHeight);
	// 		m_viewportWidth = m_width = m_widthPixels = nowWidth;
	// 		m_viewportHeight = m_height = m_heightPixels = nowHeight;
#ifdef HIGHDPI_SUPPORTED
	if ( m_flags & Flag_HighDPI )
		SDL_GL_GetDrawableSize(g_sdlWindow, &m_widthPixels, &m_heightPixels);
#endif
	m_viewportWidthPixels = m_widthPixels;
	m_viewportHeightPixels = m_heightPixels;
	// 		ResizeRenderTargetsToMatchWindow();
	// 	}
	if ( SDL_GL_SetSwapInterval(vsync ? 1 : 0) == -1 )
		vsLog("Couldn't set vsync");

	NotifyResized( m_width, m_height );
}

void
vsRenderer_OpenGL3::NotifyResized( int width, int height )
{
	// if ( m_windowType == WindowType_Window )
	{
		// int nowWidth, nowHeight;
		// SDL_GetWindowSize(g_sdlWindow, &nowWidth, &nowHeight);
		SDL_GL_GetDrawableSize(g_sdlWindow, &m_widthPixels, &m_heightPixels);

		vsLog("SDL window resize event showing window size as %dx%d, SDL_GetWindowSize() shows size as %dx%d", width, height, m_widthPixels, m_heightPixels);
		m_viewportWidth = m_width = m_widthPixels;
		m_viewportHeight = m_height = m_heightPixels;
		// vsLog("SDL window resize event showing window size as %dx%d, SDL_GetWindowSize() shows size as %dx%d", width, height, nowWidth, nowHeight);
		// vsAssert( width == nowWidth && height == nowHeight, "Whaa?" );

		// 		m_viewportWidth = m_width = m_widthPixels = nowWidth;
		// 		m_viewportHeight = m_height = m_heightPixels = nowHeight;
		// #ifdef HIGHDPI_SUPPORTED
		// 		if ( m_flags & Flag_HighDPI )
		// 			SDL_GL_GetDrawableSize(g_sdlWindow, &m_widthPixels, &m_heightPixels);
		// #endif
		m_viewportWidthPixels = m_widthPixels;
		m_viewportHeightPixels = m_heightPixels;
		ResizeRenderTargetsToMatchWindow();
	}
	DetermineRefreshRate();
}

void
vsRenderer_OpenGL3::Present()
{
	PROFILE_GL("Present");
	{
		PROFILE_GL("Flush");
		glFlush();
	}
	{
		PROFILE_GL("Swap");
#if !TARGET_OS_IPHONE
#ifdef __apple_cc__
		// on OSX we must explicitly set the draw framebuffer to 0 before swap.
		// Ref:
		// http://renderingpipeline.com/2012/05/nsopenglcontext-flushbuffer-might-not-do-what-you-think/
		m_window->Bind();
#endif
		SDL_GL_SwapWindow(g_sdlWindow);
#endif

#ifdef VS_TRACY
		// TracyGpuCollect;
		FrameMark;
#endif
	}

	{
		PROFILE_GL("FinishPresent");

		// vsTimerSystem::Instance()->EndGPUTime();
	}
	// {
	// 	int nowWidth, nowHeight;
	// 	SDL_GetWindowSize(g_sdlWindow, &nowWidth, &nowHeight);
	// 	vsLog("Size:  %dx%d", nowWidth, nowHeight);
	// 	vsLog("Viewport:  %dx%d", m_viewportWidthPixels, m_viewportHeightPixels);
	// }

	vsDynamicBatchManager::Instance()->FrameRendered();

}

void
vsRenderer_OpenGL3::FlushRenderState()
{
	PROFILE_GL("FlushRenderState");
	// For these immediate-mode style "arrays embedded directly in the display list"
	// situations, we need to make sure that we have enough space to push all the
	// data directly;  that we won't run out partway through.  This means that we
	// couldn't bind the arrays immediately as they came in, but instead needed to
	// hold on to them until now, right before we draw.  So let's make sure we
	// have space for all the data, then bind it all at once!
	if ( m_currentColorArray || m_currentNormalArray || m_currentTexelArray || m_currentVertexArray )
	{
		PROFILE_GL("ImmediateStyleArray");
		vsRenderBuffer::EnsureSpaceForVertexColorTexelNormal(
				m_currentVertexArrayCount,
				m_currentColorArrayCount,
				m_currentTexelArrayCount,
				m_currentNormalArrayCount
				);
	}
	if ( m_currentColorArray )
	{
		PROFILE_GL("BindColorArray");
		vsRenderBuffer::BindColorArray( &m_state, m_currentColorArray, m_currentColorArrayCount );
		m_state.SetBool( vsRendererState::ClientBool_ColorArray, true );
	}
	if ( m_currentNormalArray )
	{
		PROFILE_GL("BindNormalArray");
		vsRenderBuffer::BindNormalArray( &m_state, m_currentNormalArray, m_currentNormalArrayCount );
		m_state.SetBool( vsRendererState::ClientBool_NormalArray, true );
	}
	if ( m_currentTexelArray )
	{
		PROFILE_GL("BindTextureArray");
		vsRenderBuffer::BindTexelArray( &m_state, m_currentTexelArray, m_currentTexelArrayCount );
		m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );
	}
	if ( m_currentVertexArray )
	{
		PROFILE_GL("BindVertexArray");
		vsRenderBuffer::BindVertexArray( &m_state, m_currentVertexArray, m_currentVertexArrayCount );
		m_state.SetBool( vsRendererState::ClientBool_VertexArray, true );
	}
	// GL_CHECK("EnsureSpaceForVertex");
	// GL_CHECK("PreFlush");
	{
		PROFILE_GL("StateFlush");
		m_state.Flush();
	}
	// GL_CHECK("PostStateFlush");
	if ( m_currentShader )
	{
		PROFILE_GL("Shader");
		bool needsReset = false;

		uint32_t shaderOptionsValue = m_currentMaterial->GetShaderOptions()->value &
			m_currentMaterial->GetShaderOptions()->mask;
		{
			PROFILE("ShaderOptions");
			uint32_t shaderOptionsSet = 0;
			for( int i = m_optionsStack.ItemCount()-1; i >= 0; i-- )
			{
				const vsShaderOptions &s = m_optionsStack[i];

				shaderOptionsValue |= s.value & s.mask & ~shaderOptionsSet;
				shaderOptionsSet |= s.mask;
			}

			{
				PROFILE("VariantBits");
				shaderOptionsValue |= vsShader::GetVariantBitsFor( m_currentMaterial->GetShaderValues() );
				shaderOptionsValue |= vsShader::GetVariantBitsFor( m_currentMaterialInternal->GetShaderValues() );
				shaderOptionsValue |= vsShader::GetVariantBitsFor( m_currentShaderValues );
			}
		}

		// only ask for bits that are actually supported by this shader
		shaderOptionsValue &= m_currentShader->GetVariantBitsSupported();

		if ( m_lastShaderId != m_currentShader->GetShaderId() )
			needsReset = true;
		else if ( shaderOptionsValue != m_currentShader->GetCurrentVariantBits() )
			needsReset = true;

		if ( needsReset )
		{
			PROFILE_GL("ShaderNeedsReset");
			m_currentShader->SetForVariantBits( shaderOptionsValue );
			glUseProgram( m_currentShader->GetShaderId() );
			m_currentShader->Prepare( m_currentMaterial, m_currentShaderValues, m_currentRenderTarget );
			m_lastShaderId = m_currentShader->GetShaderId();
			s_previousMaterial = m_currentMaterial;
		}
		else if ( m_currentMaterial != s_previousMaterial || m_currentShaderValues != s_previousShaderValues )
		{
			PROFILE_GL("ShaderPrepare");
			m_currentShader->Prepare( m_currentMaterial, m_currentShaderValues, m_currentRenderTarget );
			s_previousMaterial = m_currentMaterial;
			s_previousShaderValues = m_currentShaderValues;
		}
		// GL_CHECK("PostPrepare");

		// bind our textures now, using any overridden ones from the vsShaderValues object!
		{
			PROFILE_GL("BindTextures");
			for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
			{
				vsTexture *t = nullptr;
				if ( m_currentShaderValues && m_currentShaderValues->HasTextureOverride(i) )
					t = m_currentShaderValues->GetTextureOverride(i);
				else
					t = m_currentMaterialInternal->GetTexture(i);

				if ( t )
				{
					// glEnable(GL_TEXTURE_2D);
					if ( t->GetResource()->IsTextureBuffer() )
					{
						vsRenderBuffer * buffer = t->GetResource()->GetTextureBuffer();
						if ( currentlyBoundTexture[i] != buffer->GetBufferID() )
						{
							glActiveTexture(GL_TEXTURE0 + i);
							currentlyBoundTexture[i] = buffer->GetBufferID();
							GL_CHECK_SCOPED("BufferTexture");
							t->GetResource()->PrepareToBind();
							glBindTexture( GL_TEXTURE_BUFFER, t->GetResource()->GetTexture() );
							buffer->BindAsTexture();
						}
					}
					else
					{
						uint32_t tval = t->GetResource()->GetTexture();
						t->GetResource()->PrepareToBind();
						if ( currentlyBoundTexture[i] != tval )
						{
							glActiveTexture(GL_TEXTURE0 + i);
							currentlyBoundTexture[i] = tval;
							if ( tval == 0 )
							{
								// [TODO] Have a replacement blank or checkerboard texture here.
								glBindTexture( GL_TEXTURE_2D, 0 );
								vsLog("Tried to bind invalid texture.");
								vsLog("Material: %s", m_currentMaterial->GetName() );
								vsLog("Texture slot %d", i);
								vsLog("Texture name %s", t->GetResource()->GetName());
								// vsAssert( tval != 0, "0 texture??" );
							}
							else
							{
								glBindTexture( GL_TEXTURE_2D, tval );
							}

							const bool clampU = t->GetClampU();
							const bool clampV = t->GetClampV();
							// const bool linearSampling = t->GetLinearSampling();
							// const bool mipmap = t->GetUseMipmap();
							t->GetResource()->SetClampedU( clampU );
							t->GetResource()->SetClampedV( clampV );

							if ( t->GetResource()->IsStateDirty() )
							{
								// if ( !t->GetResource()->IsSamplingLocked() )
								// {
								glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampU ? GL_CLAMP_TO_EDGE : GL_REPEAT );
								glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampV ? GL_CLAMP_TO_EDGE : GL_REPEAT );
								// }
								t->GetResource()->ClearDirtyFlag();
							}

							// if ( m_currentMaterialInternal->m_clampU )
							// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_currentMaterialInternal->m_clampU ? GL_CLAMP_TO_EDGE : GL_REPEAT );
							// if ( m_currentMaterialInternal->m_clampV )
							// 	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_currentMaterialInternal->m_clampV ? GL_CLAMP_TO_EDGE : GL_REPEAT );
						}
					}
				}
				else
				{
					if ( currentlyBoundTexture[i] != 0 )
					{
						currentlyBoundTexture[i] = 0;
						glActiveTexture(GL_TEXTURE0 + i);
						glBindTexture( GL_TEXTURE_2D, 0);
					}
				}
			}
		}

		{
			PROFILE_GL("ShaderValues");
			m_currentShader->SetFog( m_currentMaterialInternal->m_fog, m_currentFogColor, m_currentFogDensity );
			m_currentShader->SetTextures( m_currentMaterialInternal->m_texture );
			if ( m_currentLocalToWorldBuffer )
				m_currentShader->SetLocalToWorld( m_currentLocalToWorldBuffer );
			else if ( m_currentLocalToWorldCount > 0 )
				m_currentShader->SetLocalToWorld( m_currentLocalToWorld, m_currentLocalToWorldCount );
			else
				m_currentShader->SetLocalToWorld( &m_transformStack[0], 1 );

			if ( m_currentMaterial->GetResource()->m_glow )
			{
				// debugging!
				// vsLog("DEBUGGING GLOW RENDER:");
				// vsLog("CurrentColor: %f,%f,%f,%f", m_currentColor.r, m_currentColor.g, m_currentColor.b, m_currentColor.a);
				// vsLog("CurrentColorsBuffer: %s", m_currentColorsBuffer ? "TRUE" : "FALSE");
				// vsLog("CurrentColors: %s", m_currentColors ? "TRUE" : "FALSE");
			}

			m_currentShader->SetColor( m_currentColor );
			if ( m_currentColorsBuffer )
				m_currentShader->SetInstanceColors( m_currentColorsBuffer );
			else if ( m_currentColors )
				m_currentShader->SetInstanceColors( m_currentColors, m_currentLocalToWorldCount );
			else
				m_currentShader->SetInstanceColors( &c_white, 1 );
			m_currentShader->SetWorldToView( m_currentWorldToView );
			m_currentShader->SetViewToProjection( m_currentViewToProjection );
			m_currentShader->SetViewport( m_currentViewportPixels.Extents() );
			int i = 0;
			// for ( int i = 0; i < MAX_LIGHTS; i++ )
			{
				vsVector3D halfVector;
				m_currentShader->SetLight( i, m_lightStatus[i].ambient, m_lightStatus[i].diffuse,
						m_lightStatus[i].specular, m_lightStatus[i].position,
						halfVector);
			}
			m_currentShader->ValidateCache( m_currentMaterial );
		}
	}
	else
	{
		vsAssert(0, "Trying to flush render state with no shader set?");
		glUseProgram( 0 );
	}

	if ( m_currentRenderTarget )
	{
		PROFILE_GL("InvalidateResolve");
		m_currentRenderTarget->InvalidateResolve();
	}
	// glPrimitiveRestartIndex(65535);
}

void
vsRenderer_OpenGL3::RenderDisplayList( vsDisplayList *list )
{
	PROFILE_GL("RenderDisplayList");
	GL_CHECK("RenderDisplayList");
	// m_currentMaterial = nullptr;
	// m_currentMaterialInternal = nullptr;
	// m_currentShader = nullptr;
	// m_currentShaderValues = nullptr;
	// m_lastShaderId = 0;

#ifdef VS_TRACY
	int drawCount = 0;
	int immediateDrawCount = 0;
	int materialInternalSets = 0;
	int duppedMaterialInternals = 0;
	std::unordered_set<vsMaterialInternal*> materialInternalsUsed;
#endif // VS_TRACY
	m_currentCameraPosition = vsVector3D::Zero;

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
	// ClearState();

	while(op)
	{
		GL_CHECK("ProcOp");
// #define LOG_OPS
#ifdef LOG_OPS
		vsLog("%s", vsDisplayList::GetOpCodeString(op->type).c_str());
#endif // LOG_OPS
		switch( op->type )
		{
			case vsDisplayList::OpCode_SetLinear:
				{
					if ( op->data.i )
						glEnable( GL_FRAMEBUFFER_SRGB );
					else
						glDisable( GL_FRAMEBUFFER_SRGB );
					break;
				}
			case vsDisplayList::OpCode_SetColor:
				{
					m_currentColor = op->data.GetColor();
					m_currentColors = nullptr;
					m_currentColorsBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetColors:
				{
					// m_currentColor = c_white;
					m_currentColors = (vsColor*)op->data.p;
					m_currentColorsBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetColorsBuffer:
				{
					// m_currentColor = c_white;
					m_currentColors = nullptr;
					m_currentColorsBuffer = (vsRenderBuffer*)op->data.p;
					break;
				}
			case vsDisplayList::OpCode_SetMaterial:
				{
					vsMaterial *material = (vsMaterial *)op->data.p;
					vsAssert(material, "SetMaterial called with no material?");
					if ( m_currentMaterialInternal != material->GetResource() )
					{
#ifdef VS_TRACY
						if ( materialInternalsUsed.find( material->GetResource() ) != materialInternalsUsed.end() )
							duppedMaterialInternals++;
						else
							materialInternalsUsed.insert( material->GetResource() );

						materialInternalSets++;
#endif // VS_TRACY
						SetMaterialInternal( material->GetResource() );
					}
					SetMaterial( material );
					m_currentColors = nullptr;
					m_currentColorsBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetRenderTarget:
				{
					PROFILE_GL("SetRenderTarget");
					vsRenderTarget *target = (vsRenderTarget*)op->data.p;
					SetRenderTarget(target);
					break;
				}
			case vsDisplayList::OpCode_ClearRenderTarget:
				{
					PROFILE_GL("ClearRenderTarget");
					m_lastShaderId = 0;
					glUseProgram(0);
					m_state.SetBool( vsRendererState::Bool_DepthMask, true ); // when we're clearing a render target, make sure we're writing to depth!
					m_state.SetBool( vsRendererState::Bool_StencilTest, true ); // when we're clearing a render target, make sure we're not testing stencil bits!
					m_state.Flush();
					m_currentRenderTarget->Clear();
					break;
				};
			case vsDisplayList::OpCode_ClearRenderTargetColor:
				{
					PROFILE_GL("ClearRenderTargetColor");
					m_lastShaderId = 0;
					glUseProgram(0);
					m_state.SetBool( vsRendererState::Bool_DepthMask, true ); // when we're clearing a render target, make sure we're writing to depth!
					m_state.SetBool( vsRendererState::Bool_StencilTest, true ); // when we're clearing a render target, make sure we're not testing stencil bits!
					m_state.Flush();
					m_currentRenderTarget->ClearColor( op->data.color );
					break;
				};
				// case vsDisplayList::OpCode_ResolveRenderTarget:
				// 	{
				// 		PROFILE_GL("ResolveRenderTarget");
				// 		// Since resolving a render target can involve a blit,
				// 		// flush render state first.
				// 		m_state.Flush();
				// 		// vsRenderTarget *target = (vsRenderTarget*)op->data.p;
				// 		// if ( target )
				// 		// 	target->Resolve();
				// 		// else // nullptr target means main render target.
				// 		// 	m_scene->Resolve();
				// 		break;
				// 	}
			case vsDisplayList::OpCode_BlitRenderTarget:
				{
					PROFILE_GL("Blit");
					m_state.Flush(); // flush our renderer state before blitting!
					vsRenderTarget *from = (vsRenderTarget*)op->data.p;
					vsRenderTarget *to = (vsRenderTarget*)op->data.p2;
					from->BlitTo(to);
					to->InvalidateResolve();
					break;
				}
			case vsDisplayList::OpCode_BlitRenderTargetRect:
				{
					PROFILE_GL("Blit");
					m_state.Flush(); // flush our renderer state before blitting!
					vsRenderTarget *from = (vsRenderTarget*)op->data.p;
					vsRenderTarget *to = (vsRenderTarget*)op->data.p2;
					vsBox2D fromRect = op->data.box2D;
					vsBox2D toRect = op->data.box2D2;
					from->BlitRect(to, fromRect, toRect);
					to->InvalidateResolve();

					break;
				}
			case vsDisplayList::OpCode_PushTransform:
				{
					vsTransform2D t = op->data.GetTransform();

					vsMatrix4x4 localToWorld = m_transformStack[m_currentTransformStackLevel] * t.GetMatrix();
					m_transformStack[++m_currentTransformStackLevel] = localToWorld;

					m_currentLocalToWorld = &m_transformStack[m_currentTransformStackLevel];
					m_currentLocalToWorldCount = 1;
					m_currentLocalToWorldBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_PushTranslation:
				{
					vsVector3D &v = op->data.vector;

					vsMatrix4x4 m;
					m.SetTranslation(v);
					vsMatrix4x4 localToWorld = m_transformStack[m_currentTransformStackLevel] * m;
					m_transformStack[++m_currentTransformStackLevel] = localToWorld;
					m_currentLocalToWorld = &m_transformStack[m_currentTransformStackLevel];
					m_currentLocalToWorldCount = 1;
					m_currentLocalToWorldBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_PushMatrix4x4:
				{
					vsMatrix4x4 m = op->data.GetMatrix4x4();
					vsMatrix4x4 localToWorld = m_transformStack[m_currentTransformStackLevel] * m;
					m_transformStack[++m_currentTransformStackLevel] = localToWorld;
					m_currentLocalToWorld = &m_transformStack[m_currentTransformStackLevel];
					m_currentLocalToWorldCount = 1;
					m_currentLocalToWorldBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetMatrix4x4:
				{
					const vsMatrix4x4& m = op->data.matrix4x4;
					m_transformStack[++m_currentTransformStackLevel] = m;
					m_currentLocalToWorld = &m_transformStack[m_currentTransformStackLevel];
					m_currentLocalToWorldCount = 1;
					m_currentLocalToWorldBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetMatrices4x4:
				{
					vsMatrix4x4 *m = (vsMatrix4x4*)op->data.p;
					int count = op->data.i;
					m_transformStack[++m_currentTransformStackLevel] = m[0];
					m_currentLocalToWorld = m;
					m_currentLocalToWorldCount = count;
					m_currentLocalToWorldBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetMatrices4x4Buffer:
				{
					vsRenderBuffer *b = (vsRenderBuffer*)op->data.p;
					m_transformStack[++m_currentTransformStackLevel] = vsMatrix4x4::Identity;
					m_currentLocalToWorld = nullptr;
					m_currentLocalToWorldCount = b->GetActiveMatrix4x4ArraySize();
					m_currentLocalToWorldBuffer = b;
					break;
				}
			case vsDisplayList::OpCode_SetShaderValues:
				{
					vsShaderValues *sv = (vsShaderValues*)op->data.p;
					m_currentShaderValues = sv;
					break;
				}
			case vsDisplayList::OpCode_ClearShaderValues:
				{
					m_currentShaderValues = nullptr;
					break;
				}
			case vsDisplayList::OpCode_PushShaderOptions:
				{
					m_optionsStack.AddItem( op->data.shaderOptions );
					break;
				}
			case vsDisplayList::OpCode_PopShaderOptions:
				{
					m_optionsStack.SetArraySize( m_optionsStack.ItemCount() - 1 );
					break;
				}
			case vsDisplayList::OpCode_SnapMatrix:
				{
					vsMatrix4x4 m = m_transformStack[m_currentTransformStackLevel];
					vsVector4D &t = m.w;
					t.x = (float)vsFloor(t.x + 0.5f);
					t.y = (float)vsFloor(t.y + 0.5f);
					t.z = (float)vsFloor(t.z + 0.5f);
					m_transformStack[++m_currentTransformStackLevel] = m;
					m_currentLocalToWorld = &m_transformStack[m_currentTransformStackLevel];
					m_currentLocalToWorldCount = 1;
					m_currentLocalToWorldBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetWorldToViewMatrix4x4:
				{
					m_currentWorldToView = op->data.GetMatrix4x4();
					break;
				}
			case vsDisplayList::OpCode_PopTransform:
				{
					vsAssert(m_currentTransformStackLevel > 0, "Renderer transform stack underflow??");
					m_currentTransformStackLevel--;
					m_currentLocalToWorld = &m_transformStack[m_currentTransformStackLevel];
					m_currentLocalToWorldCount = 1;
					m_currentLocalToWorldBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_SetCameraTransform:
				{
					break;
				}
			case vsDisplayList::OpCode_SetProjectionMatrix4x4:
				{
					vsMatrix4x4 &m = op->data.GetMatrix4x4();
					m_currentViewToProjection = m;
					break;
				}
			case vsDisplayList::OpCode_VertexArray:
				{
					m_currentVertexArray = (vsVector3D*)op->data.p;
					m_currentVertexArrayCount = op->data.i;
					m_currentVertexBuffer = nullptr;
					break;
				}
			case vsDisplayList::OpCode_VertexBuffer:
				{
					m_currentVertexBuffer = (vsRenderBuffer *)op->data.p;
					m_currentVertexArray = nullptr;
					m_currentVertexArrayCount = 0;
					m_currentVertexBuffer->BindVertexBuffer( &m_state );
					break;
				}
			case vsDisplayList::OpCode_NormalArray:
				{
					m_currentNormalArray = (vsVector3D*)op->data.p;
					m_currentNormalArrayCount = op->data.i;
					break;
				}
			case vsDisplayList::OpCode_NormalBuffer:
				{
					m_currentNormalBuffer = (vsRenderBuffer *)op->data.p;
					m_currentNormalBuffer->BindNormalBuffer( &m_state );
					m_currentNormalArray = nullptr;
					m_currentNormalArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_NormalArray, true );
					break;
				}
			case vsDisplayList::OpCode_ClearVertexArray:
				{
					m_currentVertexBuffer = nullptr;
					m_currentVertexArray = nullptr;
					m_currentVertexArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_VertexArray, false );
					break;
				}
			case vsDisplayList::OpCode_ClearNormalArray:
				{
					m_currentNormalBuffer = nullptr;
					m_currentNormalArray = nullptr;
					m_currentNormalArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_NormalArray, false );
					break;
				}
			case vsDisplayList::OpCode_TexelArray:
				{
					m_currentTexelArray = (vsVector2D*)op->data.p;
					m_currentTexelArrayCount = op->data.i;
					vsRenderBuffer::BindTexelArray( &m_state, op->data.p, op->data.i );
					m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );
					break;
				}
			case vsDisplayList::OpCode_TexelBuffer:
				{
					m_currentTexelBuffer = (vsRenderBuffer *)op->data.p;
					m_currentTexelArray = nullptr;
					m_currentTexelArrayCount = 0;
					m_currentTexelBuffer->BindTexelBuffer( &m_state );
					break;
				}
			case vsDisplayList::OpCode_ClearTexelArray:
				{
					m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
					break;
				}
			case vsDisplayList::OpCode_ColorArray:
				{
					m_currentColorArray = (vsColor*)op->data.p;
					m_currentColorArrayCount = op->data.i;
					break;
				}
			case vsDisplayList::OpCode_ColorBuffer:
				{
					m_currentColorBuffer = (vsRenderBuffer *)op->data.p;
					m_currentColorBuffer->BindColorBuffer( &m_state );
					m_currentColorArray = 0;
					m_currentColorArrayCount = 0;
					break;
				}
			case vsDisplayList::OpCode_ClearColorArray:
				{
					if ( m_currentColorBuffer )
					{
						//m_currentColorBuffer->UnbindColorBuffer();
						m_currentColorBuffer = nullptr;
					}
					m_currentColorArray = nullptr;
					m_currentColorArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_ColorArray, false );
					break;
				}
			case vsDisplayList::OpCode_ClearArrays:
				{
					m_currentColorArray = nullptr;
					m_currentColorBuffer = nullptr;
					m_currentColorArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_ColorArray, false );

					m_currentTexelBuffer = nullptr;
					m_currentTexelArray = nullptr;
					m_currentTexelArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );

					m_currentNormalBuffer = nullptr;
					m_currentNormalArray = nullptr;
					m_currentNormalArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_NormalArray, false );

					m_currentVertexBuffer = nullptr;
					m_currentVertexArray = nullptr;
					m_currentVertexArrayCount = 0;
					m_state.SetBool( vsRendererState::ClientBool_VertexArray, false );
					m_state.SetBool( vsRendererState::ClientBool_OtherArray, false );
					break;
				}
			case vsDisplayList::OpCode_BindBuffer:
				{
					PROFILE_GL("BindBuffer");
					m_currentColorArray = nullptr;
					m_currentColorBuffer = nullptr;
					m_currentColorArrayCount = 0;
					m_currentTexelBuffer = nullptr;
					m_currentTexelArray = nullptr;
					m_currentTexelArrayCount = 0;
					m_currentNormalBuffer = nullptr;
					m_currentNormalArray = nullptr;
					m_currentNormalArrayCount = 0;
					m_currentVertexBuffer = nullptr;
					m_currentVertexArray = nullptr;
					m_currentVertexArrayCount = 0;

					vsRenderBuffer *buffer = (vsRenderBuffer *)op->data.p;
					buffer->Bind( &m_state );
					break;
				}
			case vsDisplayList::OpCode_UnbindBuffer:
				{
					PROFILE_GL("UnbindBuffer");
					vsRenderBuffer *buffer = (vsRenderBuffer *)op->data.p;
					buffer->Unbind( &m_state );
					break;
				}
			case vsDisplayList::OpCode_LineListArray:
				{
					PROFILE("LineListArray");
					FlushRenderState();
					vsRenderBuffer::DrawElementsImmediate( GL_LINES, op->data.p, op->data.GetUInt(), m_currentLocalToWorldCount );
#ifdef VS_TRACY
					immediateDrawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_LineStripArray:
				{
					PROFILE("LineStripArray");
					FlushRenderState();
					vsRenderBuffer::DrawElementsImmediate( GL_LINE_STRIP, op->data.p, op->data.GetUInt(), m_currentLocalToWorldCount );
#ifdef VS_TRACY
					immediateDrawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_TriangleListArray:
				{
					// PROFILE_GL("TriangleListArray");
					PROFILE("TriangleListArray");
					FlushRenderState();

					vsRenderBuffer::DrawElementsImmediate( GL_TRIANGLES, op->data.p, op->data.GetUInt(), m_currentLocalToWorldCount );
#ifdef VS_TRACY
					immediateDrawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_TriangleStripArray:
				{
					// PROFILE_GL("TriangleStripArray");
					PROFILE("TriangleStripArray");
					FlushRenderState();
					vsRenderBuffer::DrawElementsImmediate( GL_TRIANGLE_STRIP, op->data.p, op->data.GetUInt(), m_currentLocalToWorldCount );
#ifdef VS_TRACY
					immediateDrawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_TriangleStripBuffer:
				{
					PROFILE("TriangleStripBuffer");

					vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
					// if ( ib->UsesPrimitiveRestart() )
					// 	m_state.SetBool(vsRendererState::Bool_PrimitiveRestartFixedIndex,true);
					FlushRenderState();
					ib->TriStripBuffer(m_currentLocalToWorldCount);
#ifdef VS_TRACY
					drawCount++;
#endif
					// m_state.SetBool(vsRendererState::Bool_PrimitiveRestartFixedIndex,false);
					break;
				}
			case vsDisplayList::OpCode_TriangleListBuffer:
				{
					PROFILE("TriangleListBuffer");
					// PROFILE_GL("TriangleListBuffer");
					FlushRenderState();
					vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
					ib->TriListBuffer(m_currentLocalToWorldCount);
#ifdef VS_TRACY
					drawCount++;
#endif
					// m_currentShader->ValidateCache( m_currentMaterial );
					break;
				}
			case vsDisplayList::OpCode_TriangleFanBuffer:
				{
					PROFILE("TriangleFanBuffer");
					vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
					// if ( ib->UsesPrimitiveRestart() )
					// 	m_state.SetBool(vsRendererState::Bool_PrimitiveRestartFixedIndex,true);
					FlushRenderState();
					ib->TriFanBuffer(m_currentLocalToWorldCount);
#ifdef VS_TRACY
					drawCount++;
#endif
					// m_state.SetBool(vsRendererState::Bool_PrimitiveRestartFixedIndex,false);
					break;
				}
			case vsDisplayList::OpCode_LineListBuffer:
				{
					PROFILE("LineListBuffer");
					FlushRenderState();
					vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
					ib->LineListBuffer(m_currentLocalToWorldCount);
#ifdef VS_TRACY
					drawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_LineStripBuffer:
				{
					PROFILE("LineStripBuffer");
					FlushRenderState();
					vsRenderBuffer *ib = (vsRenderBuffer *)op->data.p;
					ib->LineStripBuffer(m_currentLocalToWorldCount);
#ifdef VS_TRACY
					drawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_TriangleFanArray:
				{
					PROFILE("TriangleFanArray");
					FlushRenderState();
					vsRenderBuffer::DrawElementsImmediate( GL_TRIANGLE_FAN, op->data.p, op->data.GetUInt(), m_currentLocalToWorldCount );
#ifdef VS_TRACY
					immediateDrawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_PointsArray:
				{
					PROFILE("PointsArray");
					FlushRenderState();
					vsRenderBuffer::DrawElementsImmediate( GL_POINTS, op->data.p, op->data.GetUInt(), m_currentLocalToWorldCount );
#ifdef VS_TRACY
					immediateDrawCount++;
#endif
					break;
				}
			case vsDisplayList::OpCode_Light:
				{
					PROFILE("Light");
					if ( m_lightCount < MAX_LIGHTS - 1 )
					{
						vsLight &l = op->data.light;
						if ( l.m_type == vsLight::Type_Ambient )
						{
							m_lightStatus[m_lightCount].type = 1;
						}
						if ( l.m_type == vsLight::Type_Directional )
						{
							m_lightStatus[m_lightCount].type = 2;
							m_lightStatus[m_lightCount].position = l.m_direction;
						}
						if ( l.m_type == vsLight::Type_Point )
						{
							m_lightStatus[m_lightCount].type = 3;
							m_lightStatus[m_lightCount].position = l.m_position;
						}
						m_lightStatus[m_lightCount].ambient = l.m_ambient;
						m_lightStatus[m_lightCount].diffuse = l.m_color;
						m_lightStatus[m_lightCount].specular = l.m_specular;

						m_lightCount++;
					}
					break;
				}
			case vsDisplayList::OpCode_ClearLights:
				{
					for ( int i = 0; i < m_lightCount; i++ )
					{
						m_lightStatus[i].type = 0;
					}
					m_lightCount = 0;
					break;
				}
			case vsDisplayList::OpCode_Fog:
				{
					vsColor fogColor = op->data.fog.GetColor();
					m_currentFogColor = fogColor;
					m_currentFogDensity = op->data.fog.GetDensity();
					break;
				}
			case vsDisplayList::OpCode_ClearFog:
				{
#if 0
					glFogf(GL_FOG_DENSITY, 0.f );
#endif // 0
					break;
				}
			case vsDisplayList::OpCode_FlatShading:
				{
					// glShadeModel( GL_FLAT );
					break;
				}
			case vsDisplayList::OpCode_SmoothShading:
				{
					// glShadeModel( GL_SMOOTH );
					break;
				}
				// Disabling 'EnableStencil' and 'DisableStencil' opcodes;  moving them to materials.
			case vsDisplayList::OpCode_EnableStencil:
				{
					// m_state.SetBool( vsRendererState::Bool_StencilTest, true );
					// glStencilFunc(GL_EQUAL, 0x1, 0x1);
					break;
				}
			case vsDisplayList::OpCode_DisableStencil:
				{
					// m_state.SetBool( vsRendererState::Bool_StencilTest, false );
					// glStencilFunc(GL_ALWAYS, 0x1, 0x1);
					break;
				}
			case vsDisplayList::OpCode_ClearStencil:
				{
					m_lastShaderId = 0;
					glUseProgram(0);
					glClearStencil(0);
					glClear(GL_STENCIL_BUFFER_BIT);
					break;
				}
			case vsDisplayList::OpCode_ClearDepth:
				{
					m_lastShaderId = 0;
					m_state.SetBool( vsRendererState::Bool_DepthMask, true );
					m_state.Flush();
					glUseProgram(0);
					glClearDepth(1.f);
					glClear(GL_DEPTH_BUFFER_BIT);
					break;
				}
			case vsDisplayList::OpCode_EnableScissor:
				{
					m_state.SetBool( vsRendererState::Bool_ScissorTest, true );
					const vsBox2D& box = op->data.box2D;
					GLsizei x = (GLsizei)(box.GetMin().x * m_currentViewportPixels.Width());
					GLsizei y = (GLsizei)(box.GetMin().y * m_currentViewportPixels.Height());
					GLsizei wid = (GLsizei)(box.Width() * m_currentViewportPixels.Width());
					GLsizei hei = (GLsizei)(box.Height() * m_currentViewportPixels.Height());

					glScissor( x, y, wid, hei );
					break;
				}
			case vsDisplayList::OpCode_DisableScissor:
				{
					m_state.SetBool( vsRendererState::Bool_ScissorTest, false );
					break;
				}
			case vsDisplayList::OpCode_SetViewport:
				{
					{
						int currentTargetWidth = m_currentRenderTarget->GetViewportWidth();
						int currentTargetHeight = m_currentRenderTarget->GetViewportHeight();

						const vsBox2D& box = op->data.box2D;
						m_currentViewportPixels.Set(
								vsVector2D( box.GetMin().x * currentTargetWidth, box.GetMin().y * currentTargetHeight ),
								vsVector2D( box.GetMax().x * currentTargetWidth, box.GetMax().y * currentTargetHeight )
								);

						glViewport( (GLsizei)( m_currentViewportPixels.GetMin().x ),
								(GLsizei)( m_currentViewportPixels.GetMin().y ),
								(GLsizei)( m_currentViewportPixels.Width() ),
								(GLsizei)( m_currentViewportPixels.Height() ) );
					}
					break;
				}
			case vsDisplayList::OpCode_ClearViewport:
				{
					int currentTargetWidth = m_currentRenderTarget->GetViewportWidth();
					int currentTargetHeight = m_currentRenderTarget->GetViewportHeight();
					glViewport( 0, 0, (GLsizei)currentTargetWidth, (GLsizei)currentTargetHeight );
					m_currentViewportPixels.Set(
							vsVector2D::Zero,
							vsVector2D( currentTargetWidth, currentTargetHeight )
							);
					break;
				}
			case vsDisplayList::OpCode_Debug:
				{
					if ( op->data.string == "screenshot" )
					{
						static int foo = 0;
						vsImage img(m_currentRenderTarget->Resolve(0));
						img.SavePNG_FullAlpha(vsFormatString("screenshot-%d.png", foo++));
					}
					else
						vsRenderDebug( op->data.string );
					break;
				}
			default:
				vsAssert(false, "Unknown opcode type in display list!");	// error;  unknown opcode type in the display list!
		}
		// GL_CHECK("RenderOp");
		{
			PROFILE("PopOp");
			op = list->PopOp();
		}
	}
	ClearState();

#ifdef VS_TRACY
	TracyPlot("draws", int64_t(drawCount + immediateDrawCount));
	TracyPlot("materialSets", int64_t(materialInternalSets));
	TracyPlot("dupMaterialSets", int64_t(duppedMaterialInternals));
#endif
}

void
vsRenderer_OpenGL3::SetMaterial(vsMaterial *material)
{
	m_currentMaterial = material;
}

void
vsRenderer_OpenGL3::SetMaterialInternal(vsMaterialInternal *material)
{
	PROFILE("SetMaterialInternal");
	vsAssert( material, "SetMaterialInternal called with nullptr material?" );
	if ( material == m_currentMaterialInternal )
	{
		return;
	}
	else
	{
		PROFILE_GL("SetMaterial");
		m_currentMaterialInternal = material;

		if ( m_currentSettings.writeColor )
		{
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
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

		if ( material->m_stencilRead || material->m_stencilWrite )
		{
			m_state.SetBool( vsRendererState::Bool_StencilTest, true );
		}
		else
		{
			m_state.SetBool( vsRendererState::Bool_StencilTest, false );
		}

		if ( material->m_stencilRead )
		{
			glStencilFunc(GL_EQUAL, 0x1, 0x1);
		}
		else
		{
			glStencilFunc(GL_ALWAYS, 0x1, 0x1);
		}
		if ( material->m_stencilWrite )
		{
			glStencilMask(0xff);
			switch ( material->m_stencilOp )
			{
				case StencilOp_None:
					glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
					break;
				case StencilOp_One:
					glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
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
					vsAssert(0, vsFormatString("Unhandled stencil type: %d", material->m_stencilOp));
			}
		}
		else
			glStencilMask(0x0);

		vsAssert( m_currentMaterialInternal != nullptr, "In SetMaterial() with no material?" );
		m_currentShader = nullptr;
		if ( material->m_shader )
		{
			m_currentShader = material->m_shader;
		}
		else
		{
			switch( material->m_drawMode )
			{
				case DrawMode_Add:
				case DrawMode_Subtract:
				case DrawMode_Normal:
				case DrawMode_PremultipliedAlpha:
				case DrawMode_Absolute:
					if ( material->m_texture[0] )
					{
						if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::NormalTex) )
							m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::NormalTex);
						else
							m_currentShader = m_defaultShaderSuite->GetShader(vsShaderSuite::NormalTex);
					}
					else
					{
						if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Normal) )
							m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Normal);
						else
							m_currentShader = m_defaultShaderSuite->GetShader(vsShaderSuite::Normal);
					}
					break;
				case DrawMode_Lit:
					if ( material->m_texture[0] )
					{
						if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::LitTex) )
							m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::LitTex);
						else
							m_currentShader = m_defaultShaderSuite->GetShader(vsShaderSuite::LitTex);
					}
					else
					{
						if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Lit) )
							m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Lit);
						else
							m_currentShader = m_defaultShaderSuite->GetShader(vsShaderSuite::Lit);
					}
					break;
				default:
					vsAssert(0,"Unknown drawmode??");
			}
		}

		/*static bool debugWireframe = false;
		  if ( debugWireframe )
		  {
		  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		  }
		  else
		  {
		  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		  }*/

		// let's do this texture binding late, so disabling it here!  We'll
		// do it when we're handling shader values instead!

		// for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
		// {
		// 	vsTexture *t = material->GetTexture(i);
		// 	if ( t )
		// 	{
		// 		// glEnable(GL_TEXTURE_2D);
		// 		if ( t->GetResource()->IsTextureBuffer() )
		// 		{
		// 			vsRenderBuffer * buffer = t->GetResource()->GetTextureBuffer();
		// 			if ( currentlyBoundTexture[i] != buffer->GetBufferID() )
		// 			{
		// 				glActiveTexture(GL_TEXTURE0 + i);
		// 				currentlyBoundTexture[i] = buffer->GetBufferID();
		// 				GL_CHECK_SCOPED("BufferTexture");
		// 				t->GetResource()->PrepareToBind();
		// 				glBindTexture( GL_TEXTURE_BUFFER, t->GetResource()->GetTexture() );
		// 				buffer->BindAsTexture();
		// 			}
		// 		}
		// 		else
		// 		{
		// 			uint32_t tval = t->GetResource()->GetTexture();
		// 			if ( currentlyBoundTexture[i] != tval )
		// 			{
		// 				glActiveTexture(GL_TEXTURE0 + i);
		// 				currentlyBoundTexture[i] = tval;
		// 				if ( tval == 0 )
		// 				{
		// 					// [TODO] Have a replacement blank or checkerboard texture here.
		// 					glBindTexture( GL_TEXTURE_2D, 0 );
		// 					vsLog("Tried to bind invalid texture.");
		// 					vsLog("Material: %s", material->GetName() );
		// 					vsLog("Texture slot %d", i);
		// 					vsLog("Texture name %s", t->GetResource()->GetName());
		// 					// vsAssert( tval != 0, "0 texture??" );
		// 				}
		// 				else
		// 				{
		// 					t->GetResource()->PrepareToBind();
		// 					glBindTexture( GL_TEXTURE_2D, tval);
		// 					if ( material->m_clampU )
		// 						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, material->m_clampU ? GL_CLAMP_TO_EDGE : GL_REPEAT );
		// 					if ( material->m_clampV )
		// 						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, material->m_clampV ? GL_CLAMP_TO_EDGE : GL_REPEAT );
		// 				}
		// 			}
		// 		}
		// 	}
		// 	else
		// 	{
		// 		if ( currentlyBoundTexture[i] != 0 )
		// 		{
		// 			currentlyBoundTexture[i] = 0;
		// 			glActiveTexture(GL_TEXTURE0 + i);
		// 			glBindTexture( GL_TEXTURE_2D, 0);
		// 		}
		// 	}
		// }

		// vsTexture *st = material->GetShadowTexture();
		// if ( st )
		// {
		// 	glActiveTexture(GL_TEXTURE0+8);
		// 	glBindTexture( GL_TEXTURE_2D, st->GetResource()->GetTexture() );
		// }
		// vsTexture *bt = material->GetBufferTexture();
		// if ( bt )
		// {
		// 	GL_CHECK_SCOPED("BufferTexture");
		// 	glActiveTexture(GL_TEXTURE0+9);
		// 	glBindTexture( GL_TEXTURE_BUFFER, bt->GetResource()->GetTexture() );
		// 	vsRenderBuffer * buffer = bt->GetResource()->GetTextureBuffer();
		// 	buffer->BindAsTexture();
		// }

		if ( material->m_alphaTest )
		{
			// m_state.SetFloat( vsRendererState::Float_AlphaThreshhold, material->m_alphaRef );
		}

		if ( material->m_zRead || material->m_zWrite )
		{
			m_state.SetBool( vsRendererState::Bool_DepthTest, true );
			m_state.SetBool( vsRendererState::Bool_DepthMask, material->m_zWrite );
			if ( material->m_zRead )
			{
				glDepthFunc( GL_LEQUAL );
			}
			else
			{
				glDepthFunc( GL_ALWAYS );
			}
		}
		else
		{
			m_state.SetBool( vsRendererState::Bool_DepthTest, false );
		}

		if ( material->m_depthBiasConstant == 0.f && material->m_depthBiasFactor == 0.f )
		{
			m_state.SetBool( vsRendererState::Bool_PolygonOffsetFill, false );
		}
		else
		{
			m_state.SetBool( vsRendererState::Bool_PolygonOffsetFill, true );
			m_state.SetFloat2( vsRendererState::Float2_PolygonOffsetConstantAndFactor, material->m_depthBiasConstant, material->m_depthBiasFactor );
		}

		// m_state.SetBool( vsRendererState::Bool_AlphaTest, material->m_alphaTest );
		// m_state.SetBool( vsRendererState::Bool_Fog, material->m_fog );

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

		m_state.SetBool( vsRendererState::Bool_Blend, material->m_blend );
		switch( material->m_drawMode )
		{
			case DrawMode_Add:
				{
#if !TARGET_OS_IPHONE
					glBlendEquation(GL_FUNC_ADD);
#endif
					// glBlendFunc(GL_SRC_ALPHA,GL_ONE);					// additive
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	// opaque
																								// m_state.SetBool( vsRendererState::Bool_Lighting, false );
																								// m_state.SetBool( vsRendererState::Bool_ColorMaterial, false );
					break;
				}
			case DrawMode_Subtract:
				{
#if !TARGET_OS_IPHONE
					glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
#endif
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					// m_state.SetBool( vsRendererState::Bool_Lighting, false );
					// m_state.SetBool( vsRendererState::Bool_ColorMaterial, false );
					break;
				}
			case DrawMode_Multiply:
				{
					glBlendEquation(GL_FUNC_ADD);
					glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
					break;
				}
			case DrawMode_MultiplyAbsolute:
				{
					glBlendEquation(GL_FUNC_ADD);
					glBlendFunc(GL_DST_COLOR, GL_ZERO);
					break;
				}
			case DrawMode_Absolute:
				{
#if !TARGET_OS_IPHONE
					glBlendEquation(GL_FUNC_ADD);
#endif
					glBlendFunc(GL_ONE,GL_ZERO);	// absolute
					break;
				}
			case DrawMode_Normal:
				{
#if !TARGET_OS_IPHONE
					glBlendEquation(GL_FUNC_ADD);
					// glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);	// opaque
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	// opaque
																												// glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	// opaque
#else
					glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);	// opaque
#endif
					// m_state.SetBool( vsRendererState::Bool_Lighting, false );
					// m_state.SetBool( vsRendererState::Bool_ColorMaterial, false );
					break;
				}
			case DrawMode_PremultipliedAlpha:
				{
					glBlendEquation(GL_FUNC_ADD);
					glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	// opaque
																											// glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	// opaque
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
					// 				m_state.SetBool( vsRendererState::Bool_Lighting, true );
					// 				m_state.SetBool( vsRendererState::Bool_ColorMaterial, true );
					// #if !TARGET_OS_IPHONE
					// 				glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) ;
					// #endif
					// 				float materialAmbient[4] = {0.f, 0.f, 0.f, 1.f};
					//
					// 				glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 50.f );
					// 				glLightModelfv( GL_LIGHT_MODEL_AMBIENT, materialAmbient);
					break;
				}
			default:
				vsAssert(0, "Unknown draw mode requested!");
		}

		if ( material->m_hasColor )
		{
			m_currentColor = material->m_color;
			// const vsColor &c = material->m_color;
			// glColor4f( c.r, c.g, c.b, c.a );

			if ( material->m_drawMode == DrawMode_Lit )
			{
				// const vsColor &specColor = material->m_specularColor;
				// glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, (float*)&specColor );
			}
		}
		else
		{
			m_currentColor = c_white;
		}
	}
}

GLuint
vsRenderer_OpenGL3::Compile(const vsString &vert, const vsString &frag )
{
	GLuint program;
	program = glCreateProgram();

	Compile(program, vert, frag, true );

	return program;
}

static void PrintAnnotatedSource( const vsString& str_in )
{
	vsString str(str_in);
	int lineNumber = 1;
	size_t lnpos = 0;
	while ( (lnpos = str.find('\n')) != vsString::npos )
	{
		vsString line = str.substr(0, lnpos);
		str.erase(0,lnpos+1);
		vsLog("%.4d: %s", lineNumber++, line.c_str());
	}
}

void
vsRenderer_OpenGL3::Compile(GLuint program, const vsString &vert_in, const vsString &frag_in, bool requireSuccess )
{
	GLuint vertShader = -1;
	GLuint fragShader = -1;
	GLchar buf[1024] = "\0";
	GLint success = true;

	vertShader = glCreateShader(GL_VERTEX_SHADER);
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar* vert = vert_in.c_str();
	const GLchar* frag = frag_in.c_str();

	glShaderSource(vertShader, 1, &vert, nullptr);
	glCompileShader(vertShader);
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
	// if (!success)
	{
		// vsLog("%s", vert);
		glGetShaderInfoLog(vertShader, sizeof(buf), 0, buf);
		if ( buf[0] != 0 )
		{
// #ifdef _DEBUG
			PrintAnnotatedSource(vert);
// #endif
			vsLog("%s",buf);
		}

		vsAssert(success || !requireSuccess,"Unable to compile vertex shader.\n");
	}

	if ( success )
	{
		glShaderSource(fragShader, 1, &frag, nullptr);
		glCompileShader(fragShader);
		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
		// if (!success)
		{
			glGetShaderInfoLog(fragShader, sizeof(buf), 0, buf);
			if ( buf[0] != 0 )
			{
// #ifdef _DEBUG
				PrintAnnotatedSource(frag);
// #endif
				vsLog("%s",buf);
			}
			vsAssert(success || !requireSuccess,"Unable to compile fragment shader.\n");
		}
	}

	if ( success )
	{
		glAttachShader(program, vertShader);
		glAttachShader(program, fragShader);

		glBindAttribLocation(program, 0, "vertex");
		glBindAttribLocation(program, 1, "texcoord");
		glBindAttribLocation(program, 2, "normal");
		glBindAttribLocation(program, 3, "color");
		glBindAttribLocation(program, 4, "instanceColorAttrib");
		glBindAttribLocation(program, 5, "localToWorldAttrib");

		glLinkProgram(program);
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			vsLog("Shader link error:");
			glGetProgramInfoLog(program, sizeof(buf), 0, buf);
			vsLog(buf);
			vsLog("Failing vertex shader:");
			PrintAnnotatedSource(vert);
			vsLog("Failing fragment shader:");
			PrintAnnotatedSource(frag);

			vsAssert(success || !requireSuccess,"Unable to link shaders.\n");
		}
		glDetachShader(program,vertShader);
		glDetachShader(program,fragShader);
	}
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);
}

void
vsRenderer_OpenGL3::DestroyShader(GLuint shader)
{
	glDeleteProgram(shader);
}


vsImage *
vsRenderer_OpenGL3::Screenshot()
{
	// bind our main window, which isn't multisampled (since glReadPixels()
	// doesn't support reading pixels from a framebuffer with MSAA enabled).
	//
	SetRenderTarget(m_window);

	const size_t bytesPerPixel = 3;	// RGB
	const size_t imageSizeInBytes = bytesPerPixel * size_t(m_widthPixels) * size_t(m_heightPixels);

	uint8_t* pixels = new uint8_t[imageSizeInBytes];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	GL_CHECK("glPixelStorei");
	glReadPixels(0, 0, m_widthPixels, m_heightPixels, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	GL_CHECK("glReadPixels");


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

			image->SetPixel(x,y, vsColor::FromBytes(rVal, gVal, bVal, 255) );
		}
	}

	vsDeleteArray( pixels );

	SetRenderTarget(m_scene);

	return image;
}

vsImage *
vsRenderer_OpenGL3::Screenshot_Async()
{
	// Take an asynchronous screenshot.  This is like what we do for regular
	// Screenshot(), but we'll transfer the pixel data asynchronously using a PBO.
	// Client is responsible for holding on to the vsImage and waiting for it to
	// signal that it has all the data.
	//
	vsImage *image = new vsImage;
	image->AsyncReadRenderTarget( m_scene, 0 );

	return image;
}

vsImage*
vsRenderer_OpenGL3::ScreenshotBack()
{
	vsTexture *texture = m_scene->Resolve(0);
	vsImage *image = new vsImage(texture);
	return image;
}

vsImage *
vsRenderer_OpenGL3::ScreenshotDepth()
{
#if !TARGET_OS_IPHONE
	int imageSize = sizeof(float) * m_widthPixels * m_heightPixels;

	float* pixels = new float[imageSize];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	GL_CHECK("glPixelStorei");
	glReadPixels(0, 0, m_widthPixels, m_heightPixels, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
	GL_CHECK("glReadPixels");


	vsImage *image = new vsImage( m_widthPixels, m_heightPixels );

	for ( int y = 0; y < m_heightPixels; y++ )
	{
		int rowStart = y * m_widthPixels;

		for ( int x = 0; x < m_widthPixels; x++ )
		{
			int ind = rowStart + x;

			float val = pixels[ind];

			image->SetPixel(x,y, vsColor(val, val, val, 1.0f) );
		}
	}

	vsDeleteArray( pixels );

	return image;
#else
	return nullptr;
#endif
}

vsImage *
vsRenderer_OpenGL3::ScreenshotAlpha()
{
	int imageSize = sizeof(float) * m_widthPixels * m_heightPixels;

	float* pixels = new float[imageSize];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, m_widthPixels, m_heightPixels, GL_ALPHA, GL_FLOAT, pixels);
	GL_CHECK("glReadPixels");


	vsImage *image = new vsImage( m_widthPixels, m_heightPixels );

	for ( int y = 0; y < m_heightPixels; y++ )
	{
		int rowStart = y * m_widthPixels;

		for ( int x = 0; x < m_widthPixels; x++ )
		{
			int ind = rowStart + x;

			float val = pixels[ind];

			image->SetPixel(x,y, vsColor(val, val, val, 1.0f) );
		}
	}

	vsDeleteArray( pixels );

	return image;
}

void
vsRenderer_OpenGL3::SetRenderTarget( vsRenderTarget *target )
{
	// [TODO]  The OpenGL code should be in HERE, not in the vsRenderTarget!

	if ( !target )
		target = m_scene;

	if ( target != m_currentRenderTarget )
	{
		target->Bind();
		m_currentRenderTarget = target;

		// and reset the viewport
		m_currentViewportPixels.Set(
				vsVector2D::Zero,
				vsVector2D( m_currentRenderTarget->GetViewportWidth(), m_currentRenderTarget->GetViewportHeight() )
				);
	}
}

void
vsRenderer_OpenGL3::SetLoadingContext()
{
	m_loadingGlContextMutex.Lock();
	SDL_GL_MakeCurrent( g_sdlWindow, m_loadingGlContext);
	GL_CHECK("SetLoadingContext");
}

void
vsRenderer_OpenGL3::ClearLoadingContext()
{
	FenceLoadingContext();
	GL_CHECK("ClearLoadingContext");
	SDL_GL_MakeCurrent( g_sdlWindow, nullptr);
	m_loadingGlContextMutex.Unlock();
}

bool
vsRenderer_OpenGL3::IsLoadingContext()
{
	return (m_loadingGlContext == SDL_GL_GetCurrentContext());
}

void
vsRenderer_OpenGL3::FenceLoadingContext()
{
	vsLog("> GL fencing");

	GL_CHECK("ClearLoadingContext");
	GLsync fenceId = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	if ( fenceId == 0 )
	{
		vsLog("Error:  glFenceSync() returned '0' as a fence name!  Misbehaviour will probably follow since I can't wait for a '0' fence to complete.");
		return;
	}
	GLenum result;
	int waitedSeconds = 0;
	bool fenceCleared = false;
	while(!fenceCleared)
	{
		result = glClientWaitSync(fenceId, GL_SYNC_FLUSH_COMMANDS_BIT, GLuint64(5000000000)); //5 Second timeout
		switch( result )
		{
			case GL_TIMEOUT_EXPIRED:
				waitedSeconds += 5;
				vsLog("Waiting on GL fence timed out after %d seconds.  Resuming wait...", waitedSeconds);
				break;
			case GL_WAIT_FAILED:
				vsLog("ERROR:  GL_WAIT_FAILED returned from glClientWaitSync()!");
				break;
			case GL_ALREADY_SIGNALED:
				// you had me at hello
				fenceCleared = true;
				break;
			case GL_CONDITION_SATISFIED:
				// we waited a bit but now we're done!
				fenceCleared = true;
				break;
			default:
				vsLog("Unknown result value '%u' returned from glClientWaitSync", result);
				fenceCleared = true;
				break;
		}
	}
	glDeleteSync(fenceId);
	vsLog("> GL fence completed");
}

vsShader*
vsRenderer_OpenGL3::DefaultShaderFor( vsMaterialInternal *mat )
{
	vsShader *result = nullptr;
	switch( mat->m_drawMode )
	{
		case DrawMode_Add:
		case DrawMode_Subtract:
		case DrawMode_Multiply:
		case DrawMode_MultiplyAbsolute:
		case DrawMode_Normal:
		case DrawMode_PremultipliedAlpha:
		case DrawMode_Absolute:
			if ( mat->m_texture[0] )
				result = m_defaultShaderSuite->GetShader(vsShaderSuite::NormalTex);
			else
				result = m_defaultShaderSuite->GetShader(vsShaderSuite::Normal);
			break;
		case DrawMode_Lit:
			if ( mat->m_texture[0] )
				result = m_defaultShaderSuite->GetShader(vsShaderSuite::LitTex);
			else
				result = m_defaultShaderSuite->GetShader(vsShaderSuite::Lit);
			break;
		default:
			vsAssert(0,"Unknown drawmode??");
	}
	return result;
}

void
vsRenderer_OpenGL3::ClearState()
{
	glUseProgram(0);
	SetRenderTarget(m_scene);
	glClearColor(0.0f,0.f,0.f,0.f);
	glClearDepth(1.f);
	glClearStencil(0);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	m_state.SetBool( vsRendererState::Bool_Blend, true );
	m_state.SetBool( vsRendererState::Bool_DepthMask, true );
	m_state.SetBool( vsRendererState::Bool_CullFace, true );
	m_state.SetBool( vsRendererState::Bool_DepthTest, true );
	m_state.SetBool( vsRendererState::Bool_Multisample, true );
	m_state.SetBool( vsRendererState::Bool_PolygonOffsetFill, false );
	m_state.SetBool( vsRendererState::Bool_StencilTest, false );
	m_state.SetBool( vsRendererState::Bool_ScissorTest, false );
	m_state.SetBool( vsRendererState::ClientBool_VertexArray, false );
	m_state.SetBool( vsRendererState::ClientBool_NormalArray, false );
	m_state.SetBool( vsRendererState::ClientBool_ColorArray, false );
	m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
	m_state.Flush();

	m_currentColor = c_white;
	m_currentColorArray = nullptr;
	m_currentColorBuffer = nullptr;
	m_currentColors = nullptr;
	m_currentColorsBuffer = nullptr;
	m_currentFogDensity = 0.001f;
	m_currentLocalToWorld = nullptr;
	m_currentLocalToWorldBuffer = nullptr;
	m_currentLocalToWorldCount = 0;
	m_currentMaterial = nullptr;
	m_currentMaterialInternal = nullptr;
	m_currentNormalArray = nullptr;
	m_currentShader = nullptr;
	m_currentShaderValues = nullptr;
	m_currentTexelArray = nullptr;
	m_currentTexelBuffer = nullptr;
	m_currentTransformStackLevel = 0;
	m_currentVertexArray = nullptr;
	m_currentVertexArrayCount = 0;
	m_currentColorArrayCount = 0;
	m_currentTexelArrayCount = 0;
	m_currentNormalArrayCount = 0;
	m_currentVertexBuffer = nullptr;
	m_lightCount = 0;
	m_usingNormalArray = false;
	m_usingTexelArray = false;

	m_transformStack[m_currentTransformStackLevel] = vsMatrix4x4::Identity;

	s_previousMaterial = nullptr;
	s_previousShaderValues = nullptr;
	m_lastShaderId = -1;
	m_optionsStack.Clear();
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D,0);
		currentlyBoundTexture[i] = 0;
	}
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	glClearColor(0.0f,0.f,0.f,0.f);
	glClearDepth(1.f);
	glClearStencil(0);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	// glStencilFunc(GL_ALWAYS, 0x1, 0x1);


	m_optionsStack.Clear();

}
