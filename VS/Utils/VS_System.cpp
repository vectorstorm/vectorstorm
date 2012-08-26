/*
 *  VS_System.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_System.h"

#include "VS_Heap.h"

#include "VS_DisplayList.h"
#include "VS_Font.h"
#include "VS_MaterialManager.h"
#include "VS_Preferences.h"
#include "VS_Random.h"
#include "VS_Screen.h"
#include "VS_SingletonManager.h"
#include "VS_TextureManager.h"

#include "VS_OpenGL.h"

#include <time.h>

#if defined(_WIN32)
//#include <shellapi.h>
#else
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if !TARGET_OS_IPHONE
#include <SDL/SDL_mouse.h>
#endif

bool				vsSystem::s_initted			= false;
vsSystem *			vsSystem::s_instance		= NULL;

extern vsHeap *g_globalHeap;	// there exists this global heap;  we need to use this when changing video modes etc.



#define VS_VERSION ("0.0.1")


static void initAttributes ()
{
#if !TARGET_OS_IPHONE
    // Setup attributes we want for the OpenGL context

    int value;

    // Don't set color bit sizes (SDL_GL_RED_SIZE, etc)
	//   TODO:  Investigate this.  SDL_OpenGL Seems to pick reasonable default values?

    // 2D Vector graphics don't require a depth buffer, so don't bother allocating one.
	//   If we actually were doing something 3D, we'd want the below lines during our
	//   initialisation.
#if 1
    value = 32;
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, value);
#endif

    // Request double-buffered OpenGL
    value = 1;
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, value);

    // Request 8 bits of red, green, blue, and alpha
    value = 8;
    SDL_GL_SetAttribute (SDL_GL_RED_SIZE, value);
    SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, value);
    SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, value);
    SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, value);

#endif // !TARGET_OS_IPHONE
}

static void printAttributes ()
{
#if !TARGET_OS_IPHONE
    // Print out attributes of the context we created
    int nAttr;
    int i;

	vsLog("OpenGL Context:");
	vsLog("  Vendor: %s", glGetString(GL_VENDOR));
	vsLog("  Renderer: %s", glGetString(GL_RENDERER));
	vsLog("  Version: %s", glGetString(GL_VERSION));
	if ( glGetString(GL_SHADING_LANGUAGE_VERSION) )
	{
		vsLog("  Shading Language Version:  %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	}
	else
	{
		vsLog("  Shader Langugage Version:  None");
	}

    SDL_GLattr  attr[] = { SDL_GL_RED_SIZE, SDL_GL_BLUE_SIZE, SDL_GL_GREEN_SIZE,
	SDL_GL_ALPHA_SIZE, SDL_GL_BUFFER_SIZE, SDL_GL_DEPTH_SIZE };

    const char *desc[] = { "Red size: %d bits", "Blue size: %d bits", "Green size: %d bits",
		"Alpha size: %d bits", "Color buffer size: %d bits", "Depth buffer size: %d bits" };

    nAttr = sizeof(attr) / sizeof(int);

    for (i = 0; i < nAttr; i++)
	{
        int value;
        SDL_GL_GetAttribute (attr[i], &value);
        vsLog(vsFormatString(desc[i], value));
    }
#endif // TARGET_OS_IPHONE
}



vsSystem::vsSystem(size_t totalMemoryBytes):
	m_showCursor( true ),
	m_showCursorOverridden( false ),
	m_focused( true ),
	m_orientation( Orientation_Normal ),
	m_screen( NULL )
{
	g_globalHeap = new vsHeap(totalMemoryBytes);

	s_instance = this;
	new vsSingletonManager;

	// Perform some basic initialisation
	vsRandom::Init();
	vsLog("VectorStorm engine version %s\n",VS_VERSION);

	vsLog("Loading preferences...\n");
	m_preferences = new vsSystemPreferences;

#if !TARGET_OS_IPHONE

	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ){
		fprintf(stderr, "Couldn't initialise SDL: %s\n", SDL_GetError() );
		exit(1);
	}
	atexit(SDL_Quit);

	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_InitSubSystem(SDL_INIT_AUDIO);
#endif
}

vsSystem::~vsSystem()
{
	vsDelete( m_preferences );
	vsDelete( m_screen );

	delete vsSingletonManager::Instance();

#if !TARGET_OS_IPHONE
	SDL_Quit();
#endif
	s_instance = NULL;
}

void
vsSystem::Init()
{
	m_preferences->CheckResolutions();
	Resolution *res = m_preferences->GetResolution();

	vsLog("Initialising [%dx%d] resolution...\n", res->width, res->height);

#if !TARGET_OS_IPHONE
	m_showCursor = !m_preferences->GetFullscreen();
	SDL_ShowCursor( m_showCursor );
#endif
	// Set requested GL context attributes
	//initAttributes ();
//#define IPHONELIKE
	m_textureManager = new vsTextureManager;
#if !defined(TARGET_OS_IPHONE) && defined(IPHONELIKE)
//	m_screen = new vsScreen( 1920, 1080, 32, false );
//	m_screen = new vsScreen( 1280, 720, 32, false );
	m_screen = new vsScreen( 960, 640, 32, false );
#else
	m_screen = new vsScreen( res->width, res->height, 32, m_preferences->GetFullscreen() );
#endif

	vsBuiltInFont::Init();
	// Get GL context attributes
	printAttributes ();
}

void
vsSystem::InitGameData()
{
	m_materialManager = new vsMaterialManager;
	//m_displayListManager = new vsDisplayListManager;
}

void
vsSystem::DeinitGameData()
{
	//vsDelete( m_displayListManager );
	vsDelete( m_materialManager );
	m_textureManager->CollectGarbage();
}

void
vsSystem::Deinit()
{
	m_preferences->Save();

	vsDelete( m_screen );
	vsDelete( m_textureManager );
}

void
vsSystem::ShowCursor(bool show)
{
	m_showCursor = show;
	m_showCursorOverridden = true;
#if !TARGET_OS_IPHONE
	SDL_ShowCursor( m_showCursor );
#endif //TARGET_OS_IPHONE
}

void
vsSystem::UpdateVideoMode()
{
	Resolution *res = m_preferences->GetResolution();
	UpdateVideoMode(res->width, res->height);
}

void
vsSystem::UpdateVideoMode(int width, int height)
{
	// this function is called when we change video mode while the game is running, post-initialisation.
	// Since we're going to be restarting our OpenGL context, we need to recompile all of our compiled display lists!
	// So before we tear down OpenGL, let's uncompile them all.

	vsDisplayList::UncompileAll();
	vsRenderBuffer::UnmapAll();

	vsTextureManager::Instance()->CollectGarbage(); // flush any unused client-side textures now, so they don't accidentally go away and go into the global heap.

	// Since this function is being called post-initialisation, we need to switch back to our system heap.
	// (So that potentially changing video mode doesn't get charged to the currently active game,
	// and then treated as a memory leak)

	vsHeap::Push(g_globalHeap);

	vsLog("Initialising [%dx%d] resolution...\n", width, height);

	if ( !m_showCursorOverridden )
	{
		m_showCursor = !m_preferences->GetFullscreen();
	}
#if !TARGET_OS_IPHONE
	SDL_ShowCursor( m_showCursor );
#endif
	GetScreen()->UpdateVideoMode( width, height, 32, m_preferences->GetFullscreen() );
    //vsTextureManager::Instance()->CollectGarbage(); // flush any render target textures now

	// Set GL context attributes
	initAttributes ();

#if !TARGET_OS_IPHONE
	// Get GL context attributes
	printAttributes ();

	SDL_WM_SetCaption("VectorStorm Engine",NULL);
#endif
	vsHeap::Pop(g_globalHeap);

	// And now that we're back, let's re-compile all our display lists.

	vsDisplayList::CompileAll();
	vsRenderBuffer::MapAll();
}

void
vsSystem::SetWindowCaption(const vsString &caption)
{
#if !TARGET_OS_IPHONE
	SDL_WM_SetCaption(caption.c_str(),NULL);
#endif
}

time_t
vsSystem::GetTime()
{
	return time(NULL);
}

void
vsSystem::MakeVsTime(vsTime *t, time_t rawTime)
{
	tm *timeStruct = localtime( &rawTime );

	t->second = timeStruct->tm_sec;
	t->minute = timeStruct->tm_min;
	t->hour =	timeStruct->tm_hour;
	t->day =	timeStruct->tm_mday;
	t->month =	timeStruct->tm_mon;
	t->year =	timeStruct->tm_year + 1900;
	t->wday =	(Weekday)timeStruct->tm_wday;
}

vsString
vsSystem::MakeTimeString(uint32_t time)
{
	return vsString("Foo");
}

void
vsSystem::Launch( const vsString &target )
{
#if defined(_WIN32)
//	ShellExecute(NULL, "open", target.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
	system( vsFormatString("open %s", target.c_str()).c_str() );
#endif
}

int
vsSystem::GetNumberOfCores()
{
	int numCPU = 1;
#if defined(_WIN32)
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	numCPU = sysinfo.dwNumberOfProcessors;
#elif defined(__APPLE_CC__)
	int mib[4];
	size_t len = sizeof(numCPU);

	/* set the mib for hw.ncpu */
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

	/* get the number of CPUs from the system */
	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if( numCPU < 1 )
	{
		mib[1] = HW_NCPU;
		sysctl( mib, 2, &numCPU, &len, NULL, 0 );

		if( numCPU < 1 )
		{
			numCPU = 1;
		}
	}
#else

#endif

	return numCPU;
}

vsSystemPreferences::vsSystemPreferences()
{
	m_preferences = new vsPreferences("Global");

	m_resolution = NULL;	// can't get this one until we can actually check what SDL supports, later on.
	m_fullscreen = m_preferences->GetPreference("Fullscreen", 0, 0, 1);
	m_bloom = m_preferences->GetPreference("Bloom", 1, 0, 1);
	m_effectVolume = m_preferences->GetPreference("EffectVolume", 8, 0, 10);
	m_musicVolume = m_preferences->GetPreference("MusicVolume", 7, 0, 10);
}

vsSystemPreferences::~vsSystemPreferences()
{
	delete m_preferences;
}

void
vsSystemPreferences::CheckResolutions()
{
#if !TARGET_OS_IPHONE
	vsLog("Checking supported resolutions...\n");

	SDL_Rect **modes;
	int modeCount;
	int maxWidth = 0;
	int maxHeight = 0;
	/* Get available fullscreen/hardware modes */
	modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

	/* Check if there are any modes available */
	if(modes == (SDL_Rect **)0){
		vsLog("No modes available!\n");
		exit(-1);
	}

	/* Check if our resolution is restricted */
	if(modes == (SDL_Rect **)-1){
		vsLog("All resolutions available.\n");
	}
	else
	{
		m_supportedResolutionCount = 0;
		/* Print valid modes */
		vsLog("Available Modes\n");
		for(modeCount=0;modes[modeCount];++modeCount)
		{
			m_supportedResolutionCount++;
		}

		m_supportedResolution = new Resolution[m_supportedResolutionCount];

		for(int i=0;i<modeCount;i++)
		{
			m_supportedResolution[modeCount-(i+1)].width = modes[i]->w;
			m_supportedResolution[modeCount-(i+1)].height = modes[i]->h;

			if ( modes[i]->w > maxWidth )
				maxWidth = modes[i]->w;
			if ( modes[i]->h > maxHeight )
				maxHeight = modes[i]->h;
		}

		for(int i = 0; i<modeCount; i++ )
		{
			vsLog("%d:  %d x %d", i, m_supportedResolution[i].width, m_supportedResolution[i].height);
		}
	}

	m_resolutionX = m_preferences->GetPreference("ResolutionX", 800, 0, maxWidth);
	m_resolutionY = m_preferences->GetPreference("ResolutionY", 600, 0, maxHeight);
	int selectedResolution = modeCount-1;
	//int exactResolution = modeCount-1;

	int desiredWidth = m_resolutionX->m_value;
	int desiredHeight = m_resolutionY->m_value;

	for ( int j = 0; j < modeCount; j++ )
	{
		if ( m_supportedResolution[j].width <= desiredWidth )
		{
			selectedResolution = j;
		}
		if ( m_supportedResolution[j].width == desiredWidth && m_supportedResolution[j].height == desiredHeight )
		{
			//exactResolution = j;
			selectedResolution = j;
			break;
		}
	}
#else

	extern int resX;
	extern int resY;

	int maxWidth = 320;
	int maxHeight = 480;
	m_supportedResolution = new Resolution[1];
	m_supportedResolution[0].width = resX;
	m_supportedResolution[0].height = resY;
	int selectedResolution = 0;
	int modeCount = 1;

	m_resolutionX = m_preferences->GetPreference("ResolutionX", 320, 0, maxWidth);
	m_resolutionY = m_preferences->GetPreference("ResolutionY", 480, 0, maxHeight);

#endif
	m_resolution = m_preferences->GetPreference("Resolution", selectedResolution, 0, modeCount-1);
	m_resolutionX->m_value = m_supportedResolution[selectedResolution].width;
	m_resolutionY->m_value = m_supportedResolution[selectedResolution].height;
	m_resolution->m_value = selectedResolution;
}

Resolution *
vsSystemPreferences::GetResolution()
{
	int resolutionId = GetResolutionId();
	return &m_supportedResolution[resolutionId];
}

int
vsSystemPreferences::GetResolutionId()
{
	return m_resolution->m_value;
}

void
vsSystemPreferences::SetResolutionId(int id)
{
	m_resolution->m_value = id;
	m_resolutionX->m_value = m_supportedResolution[id].width;
	m_resolutionY->m_value = m_supportedResolution[id].height;
}

bool
vsSystemPreferences::GetBloom()
{
	return !!(m_bloom->m_value);
}

void
vsSystemPreferences::SetBloom(bool enabled)
{
	m_bloom->m_value = enabled;
}

void
vsSystemPreferences::Save()
{
	m_preferences->Save();
}

void
vsSystemPreferences::SetFullscreen(bool fullscreen)
{
	m_fullscreen->m_value = fullscreen;
}

bool
vsSystemPreferences::GetFullscreen()
{
	return !!m_fullscreen->m_value;
}

void
vsSystemPreferences::SetEffectVolume(int volume)
{
	m_effectVolume->m_value = volume;
}

int
vsSystemPreferences::GetEffectVolume()
{
	return m_effectVolume->m_value;
}

void
vsSystemPreferences::SetMusicVolume(int volume)
{
	m_musicVolume->m_value = volume;
}

int
vsSystemPreferences::GetMusicVolume()
{
	return m_musicVolume->m_value;
}


