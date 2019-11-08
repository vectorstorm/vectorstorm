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
#include "VS_DynamicBatchManager.h"
#include "VS_SingletonManager.h"
#include "VS_TextureManager.h"
#include "VS_FileCache.h"

#include "VS_OpenGL.h"
#include "Core.h"

#include <time.h>
#include <SDL2/SDL_filesystem.h>
#include <SDL2/SDL_image.h>

#if defined(MSVC)
// MSVC
#include <intrin.h>
#else
#include <cpuid.h>
#endif

#if defined(_WIN32)

//#include <shellapi.h>
#include <winsock2.h>

#elif defined(__APPLE_CC__)

// sys/sysctl is deprecated on Linux, but still needed on OSX to figure out
// number of processors.

#include <sys/param.h>
#include <sys/sysctl.h>
#include <unistd.h>

#else

#include <unistd.h>

#endif

#if !TARGET_OS_IPHONE
#include <SDL2/SDL_mouse.h>
#endif

#include <physfs.h>

vsSystem * vsSystem::s_instance = NULL;

extern vsHeap *g_globalHeap;	// there exists this global heap;  we need to use this when changing video modes etc.



#define VS_VERSION ("0.0.1")


vsSystem::vsSystem(const vsString& companyName, const vsString& title, int argc, char* argv[], size_t totalMemoryBytes, size_t minBuffers):
	m_showCursor( true ),
	m_showCursorOverridden( false ),
	m_focused( true ),
	m_visible( false ),
	m_exitGameKeyEnabled( true ),
	m_exitApplicationKeyEnabled( true ),
	m_minBuffers(minBuffers),
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

	vsFileCache::Startup();
	InitPhysFS( argc, argv, companyName, title );

	vsLog("Loading preferences...");
	m_preferences = new vsSystemPreferences;

#if !TARGET_OS_IPHONE

	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ){
		fprintf(stderr, "Couldn't initialise SDL: %s\n", SDL_GetError() );
		exit(1);
	}
	atexit(SDL_Quit);

	int initialisedFormats = IMG_Init( IMG_INIT_JPG | IMG_INIT_PNG );
	vsAssert( initialisedFormats & IMG_INIT_JPG, "Failed to initialise JPEG reading?");
	vsAssert( initialisedFormats & IMG_INIT_PNG, "Failed to initialise PNG reading?");

	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
#if defined(USE_SDL_SOUND)
	SDL_InitSubSystem(SDL_INIT_AUDIO);
#endif // USE_SDL_SOUND
#endif
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

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
	vsFileCache::Shutdown();

#if !TARGET_OS_IPHONE
	SDL_Quit();
#endif
	s_instance = NULL;
}

void
vsSystem::Init()
{
	m_cursor[CursorStyle_Arrow] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_ARROW );
	m_cursor[CursorStyle_IBeam] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_IBEAM );
	m_cursor[CursorStyle_Wait] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_WAIT );
	m_cursor[CursorStyle_Hand] = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_HAND );

	m_preferences->CheckResolutions();
	int width, height;
	if ( m_preferences->GetFullscreen() )
	{
		Resolution *res = m_preferences->GetResolution();
		width = res->width;
		height = res->height;
	}
	else
	{
		width = m_preferences->GetWindowResolutionX();
		height = m_preferences->GetWindowResolutionY();
	}

	if ( m_preferences->GetFullscreenWindow() )
		vsLog("Init:  Initialising fullscreen window");
	else
	{
		vsLog("Init:  Initialising [%dx%d] resolution (%s)...", width, height,
				m_preferences->GetFullscreen() ? "fullscreen" : "windowed");
	}

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
	vsRenderer::WindowType wt = vsRenderer::WindowType_Window;
	if ( m_preferences->GetFullscreen() )
	{
		if ( m_preferences->GetFullscreenWindow() )
			wt = vsRenderer::WindowType_FullscreenWindow;
		else
			wt = vsRenderer::WindowType_Fullscreen;
	}
	m_screen = new vsScreen( width, height, 32, wt, vsMax(m_minBuffers, m_preferences->GetBloom() ? 2 : 1), m_preferences->GetVSync(), m_preferences->GetAntialias(), m_preferences->GetHighDPI() );
#endif
	LogSystemDetails();

	vsBuiltInFont::Init();
}

void
vsSystem::InitGameData()
{
	m_materialManager = new vsMaterialManager;
	m_dynamicBatchManager = new vsDynamicBatchManager;
}

void
vsSystem::DeinitGameData()
{
	vsDelete( m_materialManager );
	m_textureManager->CollectGarbage();
	vsDelete( m_dynamicBatchManager );
}

void
vsSystem::Deinit()
{
	m_preferences->Save();

	vsDelete( m_screen );
	vsDelete( m_textureManager );

	for ( int i = 0; i < CursorStyle_MAX; i++ )
		SDL_FreeCursor( m_cursor[i] );
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
#if defined(_WIN32)
	std::string d = m_dataDirectory + "\\" + directory;
#else
	std::string d = m_dataDirectory + "/" + directory;
#endif
	PHYSFS_mount(d.c_str(), NULL, 1);
}

#if PHYSFS_VER_MAJOR < 2 || (PHYSFS_VER_MAJOR == 2 && PHYSFS_VER_MINOR < 1)

	// We're in a PhysFS version before 2.1.0.  This means that
	// PHYSFS_unmount() doesn't exist yet;  it's still PHYSFS_removeFromSearchPath().
	// Let's make a PHYSFS_unmount() for us to use even under old PhysFS!
#define PHYSFS_unmount(x) PHYSFS_removeFromSearchPath(x)

#endif

void
vsSystem::DisableGameDirectory( const vsString &directory )
{
	std::string d = m_dataDirectory + "/" + directory;
	PHYSFS_unmount(d.c_str());
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
vsSystem::SetCursorStyle(CursorStyle style)
{
	SDL_SetCursor( m_cursor[style] );
}

void
vsSystem::UpdateVideoMode()
{
	if ( m_preferences->GetFullscreen() )
	{
		Resolution *res = m_preferences->GetResolution();
		UpdateVideoMode(res->width, res->height);
	}
	else
	{
		UpdateVideoMode(m_preferences->GetWindowResolutionX(), m_preferences->GetWindowResolutionY());
	}
}

void
vsSystem::NotifyResized(int width, int height)
{
	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially changing video
	// mode doesn't get charged to the currently active game, and then treated
	// as a memory leak)
	vsHeap::Push(g_globalHeap);

	GetScreen()->NotifyResized(width, height);

	if ( !m_preferences->GetFullscreen() )
	{
		m_preferences->SetWindowResolution( width, height );
	}

	vsHeap::Pop(g_globalHeap);
}

void
vsSystem::UpdateVideoMode(int width, int height)
{
	// this function is called when we change video mode while the game is
	// running, post-initialisation.

	// flush any unused client-side textures now, so they don't accidentally go
	// away and go into the global heap.
	vsTextureManager::Instance()->CollectGarbage();

	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially changing video
	// mode doesn't get charged to the currently active game, and then treated
	// as a memory leak)
	vsHeap::Push(g_globalHeap);

	vsLog("Changing resolution to [%dx%d] (%s)...", width, height,
			m_preferences->GetFullscreen() ? "fullscreen" : "windowed");

	if ( !m_preferences->GetFullscreen() )
	{
		m_preferences->SetWindowResolution( width, height );
	}

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
	bufferCount = vsMax( bufferCount, m_minBuffers );
	vsRenderer::WindowType wt = vsRenderer::WindowType_Window;
	if ( m_preferences->GetFullscreen() )
	{
		if ( m_preferences->GetFullscreenWindow() )
			wt = vsRenderer::WindowType_FullscreenWindow;
		else
			wt = vsRenderer::WindowType_Fullscreen;
	}
	GetScreen()->UpdateVideoMode(
			width,
			height,
			32,
			wt,
			bufferCount,
			m_preferences->GetAntialias(),
			m_preferences->GetVSync()
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
vsSystem::ToggleFullscreen()
{
	m_preferences->SetFullscreen( !m_preferences->GetFullscreen() );
	UpdateVideoMode();
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
	int numCPU = SDL_GetCPUCount();

	// note that SDL_GetCPUCount() returns the number of LOGICAL cores, not
	// the number of ACTUAL cores.  These days, most people talk about this
	// value as the number of "hardware threads".  So for example, my current
	// development machine is an quad-core i7 machine with hyperthreading
	// enabled, and this function returns the value 8;  I have four physical
	// cores which can each run two threads, so two LOGICAL cores.
	//
	// I should really rename this function to make it more obvious, but for
	// right now it's just easier to leave it this way.
	//
	// Future-Trevor:  If you ever want to go back to the old custom
	// platform-specific implementations for some reason (or to just see
	// their implementations for some other reason), those are in git
	// commit 5903c79d10a43c and earlier.

	return numCPU;
}

vsSystemPreferences::vsSystemPreferences()
{
	m_preferences = new vsPreferences("vectorstorm");

	m_resolution = 0;	// can't get this one until we can actually check what SDL supports, later on.
	m_fullscreen = m_preferences->GetPreference("Fullscreen", 1, 0, 1);
	m_fullscreenWindow = m_preferences->GetPreference("FullscreenWindow", 1, 0, 1);
	m_vsync = m_preferences->GetPreference("VSync", 1, 0, 1);
	m_bloom = m_preferences->GetPreference("Bloom", 1, 0, 1);
	m_dynamicBatching = m_preferences->GetPreference("DynamicBatching", 1, 0, 1);
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
	m_windowResolutionX = m_preferences->GetPreference("WindowResolutionX", 1280, 0, maxWidth);
	m_windowResolutionY = m_preferences->GetPreference("WindowResolutionY", 720, 0, maxHeight);
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
	m_resolution = selectedResolution;
	m_resolutionX->m_value = m_supportedResolution[selectedResolution].width;
	m_resolutionY->m_value = m_supportedResolution[selectedResolution].height;
	m_resolution = selectedResolution;
	delete [] modes;
}

vsString
vsSystem::CPUDescription()
{
	// We're going to retrieve the CPU brand string using the CPUID
	// function.  Different compilers do this different ways.

#if defined(MSVC)
#if 0
	// MSVC;  I'm informed that this code might work in Visual Studio?
	// Untested, though, so far.  Particular concern:  The method being used
	// here seems to be assuming that the CPU will support CPUID with
	// 0x80000002-4 queries, which is only true for late-model Pentium 4s and
	// later.  In the GCC section below, I check for that situation and try to
	// handle it.  Not sure what would happen if one used this implementation
	// on an old system.  And I'm not currently doing MSVC builds (or have
	// access to such an old machine for testing), so I can't really check it
	// right now.
	//
	// This code is provided in case somebody else wants to test it, or for
	// myself in the future, if someday I need MSVC builds for something for
	// some reason.

	int cpuInfo[4] = {-1};
	char CPUBrandString[0x40];

	memset(CPUBrandString, 0, sizeof(CPUBrandString));

	__cpuid(cpuInfo, 0x80000002);
	memcpy(CPUBrandString, cpuInfo, sizeof(cpuInfo));

	__cpuid(cpuInfo, 0x80000003);
	memcpy(CPUBrandString + 16, cpuInfo, sizeof(cpuInfo));

	__cpuid(cpuInfo, 0x80000004);
	memcpy(CPUBrandString + 32, cpuInfo, sizeof(cpuInfo));

	return vsString(CPUBrandString);
#endif // 0

	return vsString("CPUDescription() is currently unimplemented in MSVC");
#else // !MSVC
	// everybody else (GCC or GCC-like) uses the GCC interface to the CPUID
	// op.

	struct BrandString {
		unsigned int eax;
		unsigned int ebx;
		unsigned int ecx;
		unsigned int edx;

		vsString ToString() const
		{
			return vsString(reinterpret_cast<const char *>(this), 16);
		}
	};

	unsigned int maxLevel = __get_cpuid_max(0x80000000, NULL);
	if ( maxLevel < 0x80000004 )
		return vsString("CPU brand string not supported on this hardware.  Probably Pentium 4 or earlier.");
	unsigned int eax, ebx, ecx, edx;
	eax = ebx = ecx = edx = 0;

	vsString identification;
	for ( int i = 2; i <= 4; i++ )
	{
		__get_cpuid( 0x80000000 + i, &eax, &ebx, &ecx, &edx );
		BrandString vid = { eax, ebx, ecx, edx };
		identification += vid.ToString();
	}

	return identification;
#endif
}

void
vsSystem::LogSystemDetails()
{
	vsLog("CPU:  %s", CPUDescription().c_str());
	vsLog("Number of hardware cores:  %d", GetNumberOfCores());
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
	return m_resolution;
}

void
vsSystemPreferences::SetResolutionId(int id)
{
	m_resolution = id;
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

int
vsSystemPreferences::GetWindowResolutionX()
{
	return m_windowResolutionX->GetValue();
}

int
vsSystemPreferences::GetWindowResolutionY()
{
	return m_windowResolutionY->GetValue();
}

void
vsSystemPreferences::SetWindowResolution( int x, int y )
{
	m_windowResolutionX->SetValue(x);
	m_windowResolutionY->SetValue(y);
}

bool
vsSystemPreferences::GetFullscreen()
{
	return !!m_fullscreen->m_value;
}

void
vsSystemPreferences::SetFullscreenWindow(bool fullscreen)
{
	m_fullscreenWindow->m_value = fullscreen;
}

bool
vsSystemPreferences::GetFullscreenWindow()
{
	return !!m_fullscreenWindow->m_value;
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

bool
vsSystemPreferences::GetDynamicBatching()
{
	return m_dynamicBatching->m_value;
}

void
vsSystemPreferences::SetDynamicBatching(bool enabled)
{
	m_dynamicBatching->m_value = enabled;
}

