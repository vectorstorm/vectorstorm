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

#include "VS_BuiltInFont.h"
#include "VS_MaterialManager.h"
#include "VS_Preferences.h"
#include "VS_Random.h"
#include "VS_Screen.h"
#include "VS_SingletonManager.h"
#include "VS_TextureManager.h"

#include "VS_OpenGL.h"
#include "Core.h"

#include <time.h>
#include <SDL2/SDL_filesystem.h>

#if defined(_WIN32)
//#include <shellapi.h>
#include <winsock2.h>
#else
#include <sys/param.h>
#include <sys/sysctl.h>
#include <unistd.h>
#endif

#if !TARGET_OS_IPHONE
#include <SDL2/SDL_mouse.h>
#endif

#include <physfs.h>

vsSystem * vsSystem::s_instance = NULL;

extern vsHeap *g_globalHeap;	// there exists this global heap;  we need to use this when changing video modes etc.



#define VS_VERSION ("0.0.1")


vsSystem::vsSystem(const vsString& companyName, const vsString& title, int argc, char* argv[], size_t totalMemoryBytes):
	m_showCursor( true ),
	m_showCursorOverridden( false ),
	m_focused( true ),
	m_visible( false ),
	m_exitGameKeyEnabled( true ),
	m_exitApplicationKeyEnabled( true ),
	m_orientation( Orientation_Normal ),
	m_title( title ),
	m_screen( NULL )
{
	g_globalHeap = new vsHeap("global",totalMemoryBytes);

	s_instance = this;
	new vsSingletonManager;

	// Perform some basic initialisation
	vsRandom::Init();

	vsLog("VectorStorm engine version %s",VS_VERSION);

	InitPhysFS( argc, argv, companyName, title );

	vsLog("Loading preferences...");
	m_preferences = new vsSystemPreferences;

#if !TARGET_OS_IPHONE

	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ){
		fprintf(stderr, "Couldn't initialise SDL: %s\n", SDL_GetError() );
		exit(1);
	}
	atexit(SDL_Quit);

	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
#if defined(USE_SDL_SOUND)
	SDL_InitSubSystem(SDL_INIT_AUDIO);
#endif // USE_SDL_SOUND
#endif

#if defined(_WIN32)
	static bool startedUp = false;
	if ( !startedUp )
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2,2), &wsaData);
		startedUp = true;
	}
#endif
}

vsSystem::~vsSystem()
{
	vsDelete( m_preferences );
	vsDelete( m_screen );

	delete vsSingletonManager::Instance();

	DeinitPhysFS();

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

	vsLog("Initialising [%dx%d] resolution...", res->width, res->height);

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
	m_screen = new vsScreen( 960, 640, 32, false, false );
#else
	m_screen = new vsScreen( res->width, res->height, 32, m_preferences->GetFullscreen(), m_preferences->GetBloom() ? 2 : 1, m_preferences->GetVSync(), m_preferences->GetAntialias(), m_preferences->GetHighDPI() );
#endif

	vsBuiltInFont::Init();
}

void
vsSystem::InitGameData()
{
	m_materialManager = new vsMaterialManager;
}

void
vsSystem::DeinitGameData()
{
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
vsSystem::InitPhysFS(int argc, char* argv[], const vsString& companyName, const vsString& title)
{
	PHYSFS_init(argv[0]);
	int success = PHYSFS_setWriteDir( SDL_GetPrefPath(companyName.c_str(), title.c_str()) );
	vsLog("====== Initialising file system");
	if ( !success )
	{
		vsLog("SetWriteDir failed!", success);
		exit(1);
	}

	vsLog("UserDir: %s", PHYSFS_getUserDir());
	vsLog("BaseDir: %s", PHYSFS_getBaseDir());

#if defined(__APPLE_CC__)
	// loading out of an app bundle, so use that data directory
	m_dataDirectory =  std::string(PHYSFS_getBaseDir()) + "Contents/Resources/Data";
#elif defined(_WIN32)

	// Under Win32, Visual Studio likes to put debug and release builds into a directory
	// "Release" or "Debug" sitting under the main project directory.  That's convenient,
	// but it means that the executable location isn't in the same place as our Data
	// directory.  So we need to detect that situation, and if it happens, move our
	// data directory up by one.
	vsString baseDirectory = PHYSFS_getBaseDir();
	if ( baseDirectory.rfind("\\Debug\\") == baseDirectory.size()-7 )
		baseDirectory.erase(baseDirectory.rfind("\\Debug\\"));
	else if ( baseDirectory.rfind("\\Release\\") == baseDirectory.size()-9 )
		baseDirectory.erase(baseDirectory.rfind("\\Release\\"));
	m_dataDirectory = baseDirectory + "\\Data";
#else
	// generic UNIX.  Assume data directory is right next to the executable.
	m_dataDirectory =  std::string(PHYSFS_getBaseDir()) + "/Data";
#endif
	success = PHYSFS_mount(PHYSFS_getBaseDir(), NULL, 0);
	success = PHYSFS_mount(m_dataDirectory.c_str(), NULL, 0);
	if ( !success )
	{
		vsLog("Failed to mount %s", m_dataDirectory.c_str());
		exit(1);
	}
	success |= PHYSFS_mount(PHYSFS_getWriteDir(), NULL, 0);

	char** searchPath = PHYSFS_getSearchPath();
	int pathId = 0;
	while ( searchPath[pathId] )
	{
		vsLog("Search path: %s",searchPath[pathId]);
		pathId++;
	}
}

void
vsSystem::EnableGameDirectory( const vsString &directory )
{
	std::string d = m_dataDirectory + "/" + directory;
	PHYSFS_mount(d.c_str(), NULL, 1);
}

void
vsSystem::DisableGameDirectory( const vsString &directory )
{
	std::string d = m_dataDirectory + "/" + directory;
	PHYSFS_removeFromSearchPath(d.c_str());
}

void
vsSystem::DeinitPhysFS()
{
	PHYSFS_deinit();
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

	// vsRenderBuffer::UnmapAll();

	vsTextureManager::Instance()->CollectGarbage(); // flush any unused client-side textures now, so they don't accidentally go away and go into the global heap.

	// Since this function is being called post-initialisation, we need to switch back to our system heap.
	// (So that potentially changing video mode doesn't get charged to the currently active game,
	// and then treated as a memory leak)

	vsHeap::Push(g_globalHeap);

	vsLog("Initialising [%dx%d] resolution...", width, height);

	if ( !m_showCursorOverridden )
	{
		m_showCursor = !m_preferences->GetFullscreen();
	}
#if !TARGET_OS_IPHONE
	SDL_ShowCursor( m_showCursor );
#endif
	int bufferCount = 1;
	if ( m_preferences->GetBloom() )
		bufferCount = 2;
	GetScreen()->UpdateVideoMode(
			width,
			height,
			32,
			m_preferences->GetFullscreen(),
			bufferCount,
			m_preferences->GetAntialias()
			);
    //vsTextureManager::Instance()->CollectGarbage(); // flush any render target textures now

	vsHeap::Pop(g_globalHeap);

	// And now that we're back, let's re-compile all our display lists.

	// vsRenderBuffer::MapAll();
}

void
vsSystem::CheckVideoMode()
{
	// Since this function is being called post-initialisation, we need to switch back to our system heap.
	// (So that potentially changing video mode doesn't get charged to the currently active game,
	// and then treated as a memory leak)

	vsHeap::Push(g_globalHeap);

	GetScreen()->CheckVideoMode();

	vsHeap::Pop(g_globalHeap);
}

void
vsSystem::SetWindowCaption(const vsString &caption)
{
#if !TARGET_OS_IPHONE
	//SDL_WM_SetCaption(caption.c_str(),NULL);
#endif
}

time_t
vsSystem::GetTime()
{
	return time(NULL);
}

vsString
vsSystem::GetTimeAsString()
{
	time_t t = GetTime();
	return MakeTimeString(t);
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

void
vsSystem::MakeVsTime_UTC(vsTime *t, time_t rawTime)
{
	tm *timeStruct = gmtime( &rawTime );

	t->second = timeStruct->tm_sec;
	t->minute = timeStruct->tm_min;
	t->hour =	timeStruct->tm_hour;
	t->day =	timeStruct->tm_mday;
	t->month =	timeStruct->tm_mon;
	t->year =	timeStruct->tm_year + 1900;
	t->wday =	(Weekday)timeStruct->tm_wday;
}

vsString
vsSystem::MakeTimeString(time_t time)
{
	const int c_bufferLength = 128;
	char buffer[c_bufferLength];
	struct tm * p = localtime(&time);
	strftime(buffer, c_bufferLength, "%c", p);
	buffer[c_bufferLength-1] = '\0';
	return buffer;
}

void
vsSystem::Launch( const vsString &target )
{
#if defined(_WIN32)
	ShellExecute(NULL, "open", target.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE_CC__)
	system( vsFormatString("open \"%s\"", target.c_str()).c_str() );
#else
	// Linux, probably?
	int err = system( vsFormatString("xdg-open %s", target.c_str()).c_str() );
	if ( err )
		vsLog("system: error %d", err);
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
	 numCPU = sysconf( _SC_NPROCESSORS_ONLN );
#endif

	return numCPU;
}

vsSystemPreferences::vsSystemPreferences()
{
	m_preferences = new vsPreferences("Global");

	m_resolution = NULL;	// can't get this one until we can actually check what SDL supports, later on.
	m_fullscreen = m_preferences->GetPreference("Fullscreen", 0, 0, 1);
	m_vsync = m_preferences->GetPreference("VSync", 1, 0, 1);
	m_bloom = m_preferences->GetPreference("Bloom", 1, 0, 1);
	m_antialias = m_preferences->GetPreference("Antialias", 0, 0, 1);
	m_highDPI = m_preferences->GetPreference("HighDPI", 0, 0, 1);
	m_effectVolume = m_preferences->GetPreference("EffectVolume", 100, 0, 100);
	m_musicVolume = m_preferences->GetPreference("MusicVolume", 100, 0, 100);
	m_wheelSmoothing = m_preferences->GetPreference("WheelSmoothing", 1, 0, 1);
	m_mouseWheelScalePercent = m_preferences->GetPreference("MouseWheelScalePercent", 100, 0, 10000);

	// The Mac implementation of SDL_input shows a very responsive "mouse
	// wheel" when doing two-finger scrolling on a trackpad.  Not certain
	// whether this behaviour is similar when doing gesture-based scrolling on
	// a trackpad on a Windows machine.  For now, default trackpad scrolling
	// to 10% the speed of mouse scrolling, as that seems to approximately
	// match, to me.  But other folks should be able to configure this to
	// whatever behaviour they like on their own systems!
	m_trackpadWheelScalePercent = m_preferences->GetPreference("TrackpadWheelScalePercent", 10, 0, 10000);
}

vsSystemPreferences::~vsSystemPreferences()
{
	delete m_preferences;
	delete [] m_supportedResolution;
}

void
vsSystemPreferences::CheckResolutions()
{
#if !TARGET_OS_IPHONE
	vsLog("Checking supported resolutions...");

	SDL_DisplayMode *modes;
	int modeCount;
	int maxWidth = 0;
	int maxHeight = 0;
	/* Get available fullscreen/hardware modes */
	modeCount = SDL_GetNumDisplayModes(0);
	modes = new SDL_DisplayMode[modeCount];

	for ( int i = 0; i < modeCount; i++ )
	{
		SDL_GetDisplayMode(0, i, &modes[i]);
	}

	/* Check if there are any modes available */
	if(modeCount <= 0){
		vsLog("No modes available!");
		exit(-1);
	}

	/* Check if our resolution is restricted */
	{
		int actualModeCount = 0;

		m_supportedResolutionCount = modeCount;
		/* Print valid modes */
		vsLog("Available Modes");

		m_supportedResolution = new Resolution[m_supportedResolutionCount];

		for(int i=0;i<modeCount;i++)
		{
			bool alreadyAdded = false;
			for ( int j = 0; j < actualModeCount; j++ )
			{
				if ( modes[i].w == m_supportedResolution[j].width &&
						modes[i].h == m_supportedResolution[j].height )
				{
					alreadyAdded = true;
					break;
				}
			}
			if ( !alreadyAdded )
			{
				m_supportedResolution[actualModeCount].width = modes[i].w;
				m_supportedResolution[actualModeCount].height = modes[i].h;
				maxWidth = vsMax( maxWidth, m_supportedResolution[actualModeCount].width );
				maxHeight = vsMax( maxHeight, m_supportedResolution[actualModeCount].height );
				actualModeCount++;
			}
			m_supportedResolutionCount = actualModeCount;
		}

		for(int i = 0; i<m_supportedResolutionCount; i++ )
		{
			vsLog("%d:  %d x %d", i, m_supportedResolution[i].width, m_supportedResolution[i].height);
		}
	}

	m_resolutionX = m_preferences->GetPreference("ResolutionX", 1024, 0, maxWidth);
	m_resolutionY = m_preferences->GetPreference("ResolutionY", 576, 0, maxHeight);
	int selectedResolution = m_supportedResolutionCount-1;
	//int exactResolution = m_supportedResolutionCount-1;

	int desiredWidth = m_resolutionX->m_value;
	int desiredHeight = m_resolutionY->m_value;

	for ( int j = m_supportedResolutionCount-1; j >= 0; j-- )
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
	delete [] modes;
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

bool
vsSystemPreferences::GetAntialias()
{
	return !!(m_antialias->m_value);
}

void
vsSystemPreferences::SetAntialias(bool enabled)
{
	m_antialias->m_value = enabled;
}

bool
vsSystemPreferences::GetHighDPI()
{
	return !!(m_highDPI->m_value);
}

void
vsSystemPreferences::SetHighDPI(bool allow)
{
	m_highDPI->m_value = allow;
}

float
vsSystemPreferences::GetMouseWheelScaling()
{
	return m_mouseWheelScalePercent->m_value / 100.f;
}
void
vsSystemPreferences::SetMouseWheelScaling(float scaling)
{
	m_mouseWheelScalePercent->m_value = (int)(scaling * 100.f);
}

float
vsSystemPreferences::GetTrackpadWheelScaling()
{
	return m_trackpadWheelScalePercent->m_value / 100.f;
}

void
vsSystemPreferences::SetTrackpadWheelScaling(float scaling)
{
	m_trackpadWheelScalePercent->m_value = (int)(scaling * 100.f);
}

bool
vsSystemPreferences::GetWheelSmoothing()
{
	return !!(m_wheelSmoothing->m_value);
}

void
vsSystemPreferences::SetWheelSmoothing(bool allow)
{
	m_wheelSmoothing->m_value = allow;
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
vsSystemPreferences::SetVSync(bool vsync)
{
	m_vsync->m_value = vsync;
}

bool
vsSystemPreferences::GetVSync()
{
	return !!m_vsync->m_value;
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


