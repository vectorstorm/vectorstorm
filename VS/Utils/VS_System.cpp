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
#include "VS_File.h"
#include "VS_ShaderCache.h"
#include "VS_ShaderUniformRegistry.h"
#include "VS_Backtrace.h"
#include "VS_Config.h"
#include "VS_Thread.h"
#include "VS_Input.h"

#include "VS_OpenGL.h"
#include "Core.h"

#include <time.h>
#include <SDL2/SDL_filesystem.h>

#if defined(MSVC)
// MSVC
#include <intrin.h>
#else
#if !__ARM_NEON__
	// no cpuid on M1 chips
#include <cpuid.h>
#endif
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
#include <SDL2/SDL_messagebox.h>

#include "Files/VS_PhysFS.h"

vsSystem * vsSystem::s_instance = nullptr;

extern vsHeap *g_globalHeap;	// there exists this global heap;  we need to use this when changing video modes etc.


namespace
{
	vsArray<vsString> s_migrateDirectories;
}


#define VS_VERSION ("0.0.1")

#ifdef _WIN32
// This SetProcessDPIAware handling comes from:
// https://github.com/kumar8600/win32_SetProcessDpiAware
#ifndef DPI_ENUMS_DECLARED
typedef enum PROCESS_DPI_AWARENESS
{
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
#endif

typedef BOOL (WINAPI * SETPROCESSDPIAWARE_T)(void);
typedef HRESULT (WINAPI * SETPROCESSDPIAWARENESS_T)(PROCESS_DPI_AWARENESS);

bool win32_SetProcessDpiAware(void) {
    HMODULE shcore = LoadLibraryA("Shcore.dll");
    SETPROCESSDPIAWARENESS_T SetProcessDpiAwareness = nullptr;
    if (shcore) {
        SetProcessDpiAwareness = (SETPROCESSDPIAWARENESS_T) GetProcAddress(shcore, "SetProcessDpiAwareness");
    }
    HMODULE user32 = LoadLibraryA("User32.dll");
    SETPROCESSDPIAWARE_T SetProcessDPIAware = nullptr;
    if (user32) {
        SetProcessDPIAware = (SETPROCESSDPIAWARE_T) GetProcAddress(user32, "SetProcessDPIAware");
    }

    bool ret = false;
    if (SetProcessDpiAwareness) {
        ret = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) == S_OK;
    } else if (SetProcessDPIAware) {
        ret = SetProcessDPIAware() != 0;
    }

    if (user32) {
        FreeLibrary(user32);
    }
    if (shcore) {
        FreeLibrary(shcore);
    }
    return ret;
}

#endif

#ifdef _WIN32
#include <ole2.h>
#include <SDL2/SDL_syswm.h>
#include <shlobj.h>

class DropTargetWindows : public IDropTarget
{
public:
	DropTargetWindows()
		: m_lRefCount(1)
	{

	}

	virtual ~DropTargetWindows()
	{

	}

	virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override
	{
		SetDragEffect(pdwEffect);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override
	{
		SetDragEffect(pdwEffect);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragLeave() override
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override
	{
		if (vsInput::Instance() && vsInput::Instance()->GetDropHandler())
		{
			// construct a FORMATETC object
			FORMATETC fmtetcText = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM stgmed;

			// See if the dataobject contains any TEXT stored as a HGLOBAL
			if(pDataObj->QueryGetData(&fmtetcText) == S_OK)
			{
				if(pDataObj->GetData(&fmtetcText, &stgmed) == S_OK)
				{
					// we asked for the data as a HGLOBAL, so access it appropriately
					PVOID data = GlobalLock(stgmed.hGlobal);

					// Get the actual text data
					vsString text = (char *)data;

					// Pass the text onto vsInput::DropHandler
					vsInput::Instance()->GetDropHandler()->Text(text);

					GlobalUnlock(stgmed.hGlobal);

					// release the data using the COM API
					ReleaseStgMedium(&stgmed);
				}
			}

			FORMATETC fmtetcFile = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			if(pDataObj->QueryGetData(&fmtetcFile) == S_OK)
			{
				if(pDataObj->GetData(&fmtetcFile, &stgmed) == S_OK)
				{
					char filepath[FILENAME_MAX];

					if (DragQueryFile((HDROP)stgmed.hGlobal, 0, filepath, FILENAME_MAX))
					{
						// Pass the text onto vsInput::DropHandler
						vsInput::Instance()->GetDropHandler()->File(filepath);
					}

					// release the data using the COM API
					ReleaseStgMedium(&stgmed);
				}
			}
		}

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject) override
	{
		if(iid == IID_IDropTarget || iid == IID_IUnknown)
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		else
		{
			*ppvObject = 0;
			return E_NOINTERFACE;
		}
	}

	virtual ULONG STDMETHODCALLTYPE AddRef() override
	{
		return InterlockedIncrement(&m_lRefCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release() override
	{
		LONG count = InterlockedDecrement(&m_lRefCount);

		if(count == 0)
		{
			delete this;
			return 0;
		}
		else
		{
			return count;
		}
	}

private:
	void SetDragEffect(DWORD *pdwEffect)
	{
		if (vsInput::Instance() && vsInput::Instance()->GetDropHandler())
		{
			*pdwEffect = DROPEFFECT_COPY;
		}
		else
		{
			*pdwEffect = DROPEFFECT_NONE;
		}
	}

	LONG	m_lRefCount;
};

#endif


vsSystem::vsSystem(const vsString& companyName, const vsString& title, const vsString& profileName, int argc, char* argv[], size_t totalMemoryBytes, size_t minBuffers):
	m_showCursor( true ),
	m_showCursorOverridden( false ),
	m_focused( true ),
	m_visible( false ),
	m_exitGameKeyEnabled( true ),
	m_exitApplicationKeyEnabled( true ),
	m_minBuffers(minBuffers),
	m_orientation( Orientation_Normal ),
	m_title( title ),
	m_screen( nullptr ),
#ifdef _WIN32
	m_dropTargetWindows( nullptr ),
#endif
	m_dataIsPristine( false )
{
#if defined(_WIN32)
	// extern bool SetProcessDPIAware();
	win32_SetProcessDpiAware();
#endif
	g_globalHeap = new vsHeap("global",totalMemoryBytes);

	s_instance = this;
	new vsSingletonManager;

	// Perform some basic initialisation
	vsRandom::Init();
	vsThread_Init();

	vsLog("VectorStorm engine version %s",VS_VERSION);

	vsFileCache::Startup();
	vsShaderCache::Startup();
	vsShaderUniformRegistry::Startup();

#if !TARGET_OS_IPHONE

	vsLog("Initialising SDL");
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ){
		fprintf(stderr, "Couldn't initialise SDL: %s\n", SDL_GetError() );
		exit(1);
	}
	atexit(SDL_Quit);

	InitPhysFS( argc, argv, companyName, title, profileName );

	vsLog("Loading preferences...");
	m_preferences = new vsSystemPreferences;

	vsLog("Initialising SDL subsystems...");
#if defined( VS_GAMEPADS )
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
#endif // VS_GAMEPADS

#if defined(USE_SDL_SOUND)
	SDL_InitSubSystem(SDL_INIT_AUDIO);
#endif // USE_SDL_SOUND
#endif // !TARGET_OS_IPHONE

#if SDL_VERSION_ATLEAST( 2, 0, 22 )
	char autoCaptureMouse = 0;
	SDL_SetHint( SDL_HINT_MOUSE_AUTO_CAPTURE, &autoCaptureMouse );
	vsLog("Disabling SDL_HINT_MOUSE_AUTO_CAPTURE");
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
	vsShaderUniformRegistry::Shutdown();
	vsShaderCache::Shutdown();
	vsFileCache::Shutdown();

#if !TARGET_OS_IPHONE
	SDL_Quit();
#endif
	s_instance = nullptr;
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

#if defined(_WIN32)
	// Initialize OLE, which is where all the windows Drag & Drop stuff takes place
	OleInitialize(nullptr);

	// Get the hwnd from the SDL window
	extern SDL_Window *g_sdlWindow;
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(g_sdlWindow, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;

	// Create the DropTarget object and register it
	m_dropTargetWindows = new DropTargetWindows();
	CoLockObjectExternal(m_dropTargetWindows, TRUE, FALSE);

	RegisterDragDrop(hwnd, m_dropTargetWindows);
#endif
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

#if defined(_WIN32)
	// remove the strong lock
	CoLockObjectExternal(m_dropTargetWindows, FALSE, TRUE);

	// release our own reference
	m_dropTargetWindows->Release();
#endif
}

void
vsSystem::_MigrateFilesToProfileDirectory(const vsString& profile)
{
	vsLog("====== Migrating existing files to new profile: %s", profile);
	// when set, we already have our write directory set to the BASE path
	// outside any profiles.
	//
	vsFile::EnsureWriteDirectoryExists(profile);

	m_mountpoints.Clear();
	m_mountpoints.AddItem( Mount( PHYSFS_getWriteDir(), "user" ) );
	_DoRemountConfiguredPhysFSVolumes(); // get our base directory mounted;  we'll remount once a game activates.

	vsArray<vsString> looseFiles;
	vsFile::DirectoryFiles(&looseFiles, "user/");
	for ( int i = 0; i < looseFiles.ItemCount(); i++ )
	{
		vsString userfilename = vsFormatString("user/%s", looseFiles[i]);
		vsString profilefilename = vsFormatString("user/%s/%s", profile, looseFiles[i]);

		if ( looseFiles[i] == "log.txt" ) // don't copy this file;  it'd overwrite the log we're writing out now.
		{
			vsFile::Delete( userfilename );
			continue;
		}
		if ( looseFiles[i] == "steam_autocloud.vdf" ) // don't copy this file!
		{
			vsFile::Delete( userfilename );
			continue;
		}

		if ( vsFile::Exists( userfilename ) )
		{
			vsLog("Migrating file: %s", userfilename);
			if ( !vsFile::Move( userfilename, profilefilename ) )
			{
				vsString fullFrom = vsFile::GetFullFilename(userfilename);
				vsString msg = vsFormatString("> Attempted to move the file '%s' into the profile directory '%s' failed.", fullFrom, profile);
				vsLog("%s", msg);
				ShowErrorMessageBox("Failed to migrate file", msg);
			}
		}
	}

	for ( int i = 0; i < s_migrateDirectories.ItemCount(); i++ )
	{
		vsString userfilename = vsFormatString("user/%s", s_migrateDirectories[i]);
		vsString profilefilename = vsFormatString("user/%s/%s", profile, s_migrateDirectories[i]);

		if ( vsFile::DirectoryExists( userfilename ) )
		{
			vsLog("Migrating directory: %s", userfilename);
			if ( !vsFile::MoveDirectory( userfilename, profilefilename ) )
			{
				vsString fullFrom = vsFile::GetFullFilename(userfilename);
				vsString msg = vsFormatString("Attempting to move the file '%s' into the profile directory '%s' failed.", fullFrom, profile);
				vsLog("%s", msg);
				ShowErrorMessageBox("Failed to migrate directory", msg);

			}
		}
	}
	m_mountpoints.Clear();
	_DoRemountConfiguredPhysFSVolumes(); // get our base directory mounted;  we'll remount once a game activates.
}

void
vsSystem::InitPhysFS(int argc, char* argv[], const vsString& companyName, const vsString& title, const vsString& profile)
{
	PHYSFS_init(argv[0]);

	char* prefPathChar = SDL_GetPrefPath(companyName.c_str(), title.c_str());
	vsAssertF(prefPathChar, "InitPhysFS: Unable to figure out where to save files: %s", SDL_GetError());
	vsString writeDir(prefPathChar);
	SDL_free(prefPathChar);
	int success = PHYSFS_setWriteDir( writeDir.c_str() );
	if ( !success )
	{
		vsString errorMsg = PHYSFS_getLastErrorString();
		vsAssertF(success, "SetWriteDir failed!: %s", errorMsg );
	}
	if ( profile != vsEmptyString )
	{
		m_mountpoints.AddItem( Mount( PHYSFS_getWriteDir(), "user" ) );
		_DoRemountConfiguredPhysFSVolumes(); // get our base directory mounted;  we'll remount once a game activates.

		bool needsMigrate = false;
		vsArray<vsString> files, directories;
		vsFile::DirectoryFiles(&files, "user/");
		vsFile::DirectoryDirectories(&directories, "user/");
		if ( !files.IsEmpty() )
			needsMigrate = true;
		else
		{
			for ( int i = 0; i < directories.ItemCount(); i++ )
			{
				if ( s_migrateDirectories.Contains(directories[i]) )
				{
					needsMigrate = true;
					break;
				}
			}
		}

		if ( needsMigrate )
		{
			_MigrateFilesToProfileDirectory(profile);
		}

		vsFile::EnsureWriteDirectoryExists(profile);
		if ( profile != vsEmptyString )
			writeDir += profile + "/";
		success = PHYSFS_setWriteDir( writeDir.c_str() );
		vsLog("====== Initialising file system");
		if ( !success )
		{
			vsString errorMsg = PHYSFS_getLastErrorString();
			vsAssertF(success, "SetWriteDir failed!: %s", errorMsg );
		}

		m_mountpoints.Clear();
	}
	vsLog("WriteDir: %s", PHYSFS_getWriteDir());
	vsFile::LogDiskStats();
	size_t bytes = vsFile::DiskStats().availableBytes;
	if ( bytes < 1024 )
		vsLog(" [!] Low space available on write drive: %d bytes", bytes );
	else if ( bytes < 1024 * 1024 )
		vsLog(" [!] Low space available on write drive: %d kilobytes", bytes/1024 );
	else if ( bytes < 1024 * 1024 * 1024 )
		vsLog(" [!] Low space available on write drive: %0.1f megabytes", bytes/(1024.f*1024.f) );

	PHYSFS_Version compiled;
	PHYSFS_Version linked;

	PHYSFS_VERSION(&compiled);
	PHYSFS_getLinkedVersion(&linked);
	vsLog("PhysFS compiled version: %d.%d.%d", compiled.major, compiled.minor, compiled.patch);
	vsLog("PhysFS linked version: %d.%d.%d", linked.major, linked.minor, linked.patch);

	vsString baseDirectory = PHYSFS_getBaseDir();
	vsLog("BaseDir: %s", baseDirectory);

#if defined(__APPLE_CC__) && defined(VS_APPBUNDLE)
	// loading out of an app bundle, so use that data directory
	m_dataDirectory =  baseDirectory + "Contents/Resources/Data/";
#elif defined(_DEBUG) && defined(_WIN32)

	// Under Win32, Visual Studio likes to put debug and release builds into a directory
	// "Release" or "Debug" sitting under the main project directory.  That's convenient,
	// but it means that the executable location isn't in the same place as our Data
	// directory.  So we need to detect that situation, and if it happens, move our
	// data directory up by one.
	if ( baseDirectory.rfind("\\Debug\\") == baseDirectory.size()-7 )
		baseDirectory.erase(baseDirectory.rfind("\\Debug\\"));
	else if ( baseDirectory.rfind("\\Release\\") == baseDirectory.size()-9 )
		baseDirectory.erase(baseDirectory.rfind("\\Release\\"));

	m_dataDirectory = baseDirectory + "Data/";

#else
	// generic UNIX.  Assume data directory is right next to the executable.
	m_dataDirectory =  baseDirectory + "Data/";
#endif

	_FindMods();

#ifdef ZIPDATA
	m_mountpoints.AddItem( Mount(m_dataDirectory+"VS.zip") );
#else
	m_mountpoints.AddItem( Mount(m_dataDirectory+"VS/") );
#endif
	m_mountpoints.AddItem( Mount( PHYSFS_getWriteDir(), "user" ) );
	_DoRemountConfiguredPhysFSVolumes(); // get our base directory mounted;  we'll remount once a game activates.

#ifdef __APPLE_CC__
	g_crashReportFile = PHYSFS_getWriteDir();
	g_crashReportFile += "crash.rpt";
	vsLog("Setting crash logs to be saved to %s", g_crashReportFile);
#endif //__APPLE_CC__
}

void
vsSystem::SetCurrentGameName( const vsString& name, bool trace )
{
	m_currentGameDirectoryName = name;

	MountPhysFSVolumes(trace);
}

void
vsSystem::_DoRemountConfiguredPhysFSVolumes()
{
	vsLog("==vsSystem::_DoRemountConfiguredPhysFSVolumes");
	for ( int i = 0; i < m_mountedpoints.ItemCount(); i++ )
	{
		_DoUnmount( m_mountedpoints[i] );
	}
	m_mountedpoints.Clear();

	for ( int i = 0; i < m_mountpoints.ItemCount(); i++ )
	{
		if ( _DoMount( m_mountpoints[i], true ) )
		{
			m_mountedpoints.AddItem( m_mountpoints[i] );
		}
	}
	vsLog("==vsSystem::_DoRemountConfiguredPhysFSVolumes complete");
}

bool
vsSystem::_DoMount( const Mount& m, bool trace )
{
	bool success = PHYSFS_mount( m.filepath.c_str(), m.mount.c_str(), m.mod ? 0 : 1 );
	if ( !success )
	{
		PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();
		vsLog(">  not mounting %s to %s: %s (%d)", m.filepath, m.mount, PHYSFS_getErrorByCode(ec), ec );
		return false;
	}
	vsLog("Mounted '%s' to '%s'", m.filepath, m.mount);
	return true;
}

bool
vsSystem::_DoUnmount( const Mount& m )
{
	bool success = PHYSFS_unmount( m.filepath.c_str() );
	if ( !success )
	{
		PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();
		vsLog("Failed to unmount %s: %s (%d)", m.filepath, PHYSFS_getErrorByCode(ec), ec );
		return false;
	}
	vsLog("Unmounted %s", m.filepath);
	return true;
}

void
vsSystem::MountPhysFSVolumes( bool trace )
{
	// configure our file mount points in DESCENDING ORDER.  Each specified
	// directory is preferred over the next one!

	// first, figure out what mods exist.  [TODO] Someday we'll want to have a
	// way to enable/disable these, probably?
	vsArray<vsString> activeMods;
	{
		// mount our write directory at the root of our read files for long enough
		// to check for mod directories.
		m_mountpoints.Clear();
		m_mountpoints.AddItem( Mount( PHYSFS_getWriteDir() ) );
		_DoRemountConfiguredPhysFSVolumes();

		// allow explicit 'mod' files!
		vsString modsDirectory = "mod";
		vsArray<vsString> mods;
		vsFile::DirectoryDirectories( &mods, modsDirectory );
		for ( int i = 0; i < mods.ItemCount(); i++ )
		{
			activeMods.AddItem(vsFormatString("%smod/%s", PHYSFS_getWriteDir(), mods[i]));
			vsLog("MOD Active: %s", mods[i]);
		}
	}

	// Our mounts go in this order/priority, with highest priority at the
	// TOP of this list:
	//
	// 1:  Mods, mounted into "/".
	// 2:  The game's data, preferring loose files over .zip files, mounted into "/".
	// 3:  VS engine data, preferring loose files over .zip files, mounted into "/".
	// 4:  The write directory, mounted into "/user".
	//
	// When we request to load a file, PhysFS goes down this list until it finds a
	// matching filename from one of these sources.  When it finds one, it stops
	// looking through files, so files from Mods override ones from the game data
	// which overrides ones from the VS engine data.

	m_mountpoints.Clear();
	for ( int i = 0; i < activeMods.ItemCount(); i++ )
	{
		m_mountpoints.AddItem( Mount(activeMods[i]) );
	}


#ifdef ZIPDATA
	std::string archiveName = m_dataDirectory + m_currentGameDirectoryName + ".zip";
	m_mountpoints.AddItem(archiveName);
	m_mountpoints.AddItem( Mount(m_dataDirectory+"VS.zip") );
#else
	std::string d = m_dataDirectory + m_currentGameDirectoryName + "/";
	m_mountpoints.AddItem(d);
	m_mountpoints.AddItem( Mount(m_dataDirectory+"VS/") );
#endif

	// and finally, last of all, our 'write' directory, which we mount under the 'user' tree.
	m_mountpoints.AddItem( Mount(PHYSFS_getWriteDir(), "user") );

	_DoRemountConfiguredPhysFSVolumes();
}

void
vsSystem::UnmountPhysFSVolumes()
{
	m_mountpoints.Clear();
	_DoRemountConfiguredPhysFSVolumes();
}

void
vsSystem::EnableGameDirectory( const vsString &directory, bool trace )
{
	SetCurrentGameName(directory,trace);



	// std::string d = m_dataDirectory + PHYSFS_getDirSeparator() + directory;
	// std::string archiveName = d + ".zip";
	// // 1 parameter means APPEND;  each new mount has LOWER priority than the previous
	// PHYSFS_mount(archiveName.c_str(), nullptr, 1);
	// PHYSFS_mount(d.c_str(), nullptr, 1);
	// // char** searchPath = PHYSFS_getSearchPath();
	// // int pathId = 0;
	// // while ( searchPath[pathId] )
	// // {
	// // 	vsLog("Search path: %s",searchPath[pathId]);
	// // 	pathId++;
	// // }
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
	// std::string d = m_dataDirectory + "/" + directory;
	// PHYSFS_unmount(d.c_str());
	// PHYSFS_unmount((d+".zip").c_str());
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

	vsString type = m_preferences->GetFullscreenWindow() ? "fullscreen window" :
		m_preferences->GetFullscreen() ? "fullscreen" : "windowed";

	vsLog("Changing resolution to [%dx%d] (%s)...", width, height, type);

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
	//SDL_WM_SetCaption(caption.c_str(),nullptr);
#endif
}

time_t
vsSystem::GetTime()
{
	return time(nullptr);
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
	ShellExecute(nullptr, "open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE_CC__)
	system( vsFormatString("open %s", target.c_str()).c_str() );
#else
	// Linux, probably?
	int err = system( vsFormatString("xdg-open %s &", target.c_str()).c_str() );
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

#if __ARM_NEON__

	// On Mac M1 platforms, we can get the data we want from sysctl.

	const char* label = "machdep.cpu.brand_string";
	size_t bufferSize = 128;
	char buffer[bufferSize];
	int32_t ncpu;
	const char* ncpulabel = "hw.ncpu";
	size_t ncpuSize = sizeof(ncpu);
	if ( sysctlbyname( label, buffer, &bufferSize, nullptr, 0) == 0 &&
			sysctlbyname( ncpulabel, &ncpu, &ncpuSize, nullptr, 0) == 0 ) {
		return vsFormatString("%s (%d cores)", (char*)buffer, ncpu);
	}
	return "Unrecognised ARM";

// machdep.cpu.brand_string: Apple M1 Pro
// machdep.cpu.core_count: 10
// machdep.cpu.cores_per_package: 10
// machdep.cpu.logical_per_package: 10
// machdep.cpu.thread_count: 10

	// return vsString("arm");
#else
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

	unsigned int maxLevel = __get_cpuid_max(0x80000000, nullptr);
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
#endif
}

unsigned long long GetTotalSystemMemory()
{
#if defined(_WIN32)
	MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
#elif defined(__APPLE_CC__)
	uint64_t mem;
	size_t len = sizeof(mem);
	sysctlbyname("hw.memsize", &mem, &len, nullptr, 0);
	return mem;
#else
	long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
#endif
}

void
vsSystem::LogSystemDetails()
{
	vsLog("CPU:  %s", CPUDescription().c_str());
	vsLog("Number of hardware cores:  %d", GetNumberOfCores());
	vsLog("System RAM:  %0.2f gb", GetTotalSystemMemory() / (1024.f*1024.f*1024.f));
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

vsString
vsSystem::GetWriteDirectory() const
{
	return PHYSFS_getWriteDir();
}

void
vsSystem::MountBaseDirectory()
{
	_DoMount( Mount(PHYSFS_getBaseDir(), "base/"), false );
}

void
vsSystem::UnmountBaseDirectory()
{
	_DoUnmount( Mount(PHYSFS_getBaseDir()) );
}

void
vsSystem::_FindMods()
{
	// we get called during startup AFTER PhysFS has been initialised but BEFORE
	// anything has been mounted.  So let's look for user-modified files.
	//
	// First, we need to mount a couple of paths for us to look in.

	PHYSFS_mount(m_dataDirectory.c_str(), "probedata/", 1);
	PHYSFS_mount(PHYSFS_getWriteDir(), "probeuser/", 1);

	{
		// Now that we have some file access, let's look for whether we have
		// any loose files hanging around in our Data directory.
		//
		// And let's check for mod directories

		vsFile::DirectoryDirectories(&m_unpackedDataDirectories, "probedata/");

		if ( vsFile::DirectoryExists("probeuser/mod/") )
			vsFile::DirectoryDirectories(&m_modDirectories, "probeuser/mod/");
	}

	// and unmount the paths we mounted before
	PHYSFS_unmount(m_dataDirectory.c_str());
	PHYSFS_unmount(PHYSFS_getWriteDir());

}

void
vsSystem::TraceMods() const
{
	if ( m_unpackedDataDirectories.IsEmpty() && m_modDirectories.IsEmpty() )
	{
		vsLog("Pristine Data:  YES");
	}
	else
	{
		vsLog("Pristine Data:  NO");

		if ( !m_unpackedDataDirectories.IsEmpty() )
		{
			vsLog("> Unpacked data directories:");
			for ( int i = 0; i < m_unpackedDataDirectories.ItemCount(); i++ )
				vsLog(">   + %s", m_unpackedDataDirectories[i]);
			vsLog(">");
		}
		if ( !m_modDirectories.IsEmpty() )
		{
			vsLog("> Mods:");
			for ( int i = 0; i < m_modDirectories.ItemCount(); i++ )
				vsLog(">   + %s", m_modDirectories[i]);
			vsLog(">");
		}
	}
}

void
vsSystem::ShowErrorMessageBox(const vsString& title, const vsString& message)
{
	extern SDL_Window *g_sdlWindow;
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), g_sdlWindow);
}

void
vsSystem::AddMod( const vsString& directory, const vsString& mountPoint )
{
	Mount m(directory, mountPoint);
	m.mod = true;
	m_mountpoints.AddItem(m);
}

void
vsSystem::Remount()
{
	_DoRemountConfiguredPhysFSVolumes();
}

vsTransform3D
vsSystem::GetOrientationTransform_2D() const
{
	vsTransform3D result;
	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Normal:
			break;
		case Orientation_Six:
			result.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(180.f) ) );
			break;
		case Orientation_Three:
			result.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(90.f) ) );
			break;
		case Orientation_Nine:
			result.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(270.f) ) );
			break;
	}
	return result;
}

vsTransform3D
vsSystem::GetOrientationTransform_3D() const
{
	vsTransform3D result;
	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Normal:
			break;
		case Orientation_Six:
			result.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(180.f) ) );
			break;
		case Orientation_Three:
			result.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(270.f) ) );
			break;
		case Orientation_Nine:
			result.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(90.f) ) );
			break;
	}
	return result;
}

void
vsSystem::SetMigrateDirectories(const vsArray<vsString>& directories)
{
	s_migrateDirectories = directories;
}

