/*
 *  VS_System.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SYSTEM_H
#define VS_SYSTEM_H

#include "Utils/VS_Singleton.h"
#include "Utils/VS_Array.h"

class vsDynamicBatchManager;
class vsMaterialManager;
class vsPreferences;
class vsPreferenceObject;
class vsSystemPreferences;
class vsScreen;
class vsTextureManager;
struct SDL_Cursor;

enum Weekday
{
	Sunday,
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday
};

struct vsTime
{
	int second;
	int	minute;
	int hour;
	int day;
	int month;
	int year;
	Weekday wday;
};

enum Orientation
{
	Orientation_Normal,
	Orientation_Three,
	Orientation_Six,
	Orientation_Nine
};

enum CursorStyle
{
	CursorStyle_Arrow,
	CursorStyle_IBeam,
	CursorStyle_Wait,
	CursorStyle_Hand,
	CursorStyle_MAX
};

#ifdef _WIN32
	class DropTargetWindows;
#endif

class vsSystem
{
	static vsSystem *	s_instance;

	bool				m_showCursor;
	bool				m_showCursorOverridden;
	bool				m_focused;
	bool				m_visible;
	bool				m_exitGameKeyEnabled;
	bool				m_exitApplicationKeyEnabled;
	int					m_minBuffers; // how many color buffers on our main render target?

	SDL_Cursor *		m_cursor[CursorStyle_MAX];

	Orientation			m_orientation;

	vsTextureManager *	m_textureManager;
	vsMaterialManager *	m_materialManager;
	vsDynamicBatchManager *m_dynamicBatchManager;

	vsString			m_title;
	vsScreen *			m_screen;

	vsSystemPreferences *m_preferences;

	vsString m_dataDirectory;
	vsString m_currentGameDirectoryName;

#ifdef _WIN32
	DropTargetWindows *m_dropTargetWindows;
#endif

	bool m_dataIsPristine;

	struct Mount
	{
		vsString filepath;
		vsString mount;
		bool mod;

		Mount(): mod(false) {}

		Mount( const vsString& fp ):
			filepath(fp),
			mount("/"),
			mod(false)
		{
		}

		Mount( const vsString& fp, const vsString& m ):
			filepath(fp),
			mount(m),
			mod(false)
		{
		}
	};

	vsArray<Mount> m_mountedpoints;
	vsArray<Mount> m_mountpoints;
	vsArray<vsString> m_unpackedDataDirectories;
	vsArray<vsString> m_modDirectories;

	void InitPhysFS(int argc, char* argv[], const vsString& companyName, const vsString& title);
	void DeinitPhysFS();

	void _DoRemountConfiguredPhysFSVolumes();
	bool _DoMount( const Mount& m, bool trace );
	bool _DoUnmount( const Mount& m );
	void _FindMods();

	void SetCurrentGameName( const vsString& game, bool trace );
	void MountPhysFSVolumes(bool trace);
	void UnmountPhysFSVolumes();

	void PrepareModGuard();

public:

	static vsSystem *	Instance() { return s_instance; }

	vsSystem( const vsString& companyName, const vsString& title, int argc, char* argv[], size_t totalMemoryBytes = 1024*1024*64, size_t minBuffers = 1 );
	~vsSystem();

	void Init();
	void Deinit();

	const vsString& GetTitle() const { return m_title; }

	void		EnableGameDirectory( const vsString &directory, bool trace );
	void		DisableGameDirectory( const vsString &directory );

	void		InitGameData();	// game has exitted, kill anything else we know about that it might have been using.
	void		DeinitGameData();	// game has exitted, kill anything else we know about that it might have been using.

	time_t		GetTime();
	vsString	GetTimeAsString();
	vsString	MakeTimeString(time_t time);
	void		MakeVsTime( vsTime *t, time_t time );	// correct for local time zone
	void		MakeVsTime_UTC( vsTime *t, time_t time );	// give time in UTC

	void UpdateVideoMode();
	void UpdateVideoMode(int width, int height);
	void NotifyResized(int width, int height);
	void CheckVideoMode();

	void ToggleFullscreen();

	void	ShowCursor(bool show);
	void	HideCursor() { ShowCursor(false); }

	void	SetCursorStyle(CursorStyle style);

	void	SetWindowCaption(const vsString &caption);
	bool	AppHasFocus() { return m_focused; }
	void	SetAppHasFocus( bool focus ) { m_focused = focus; }
	bool	AppIsVisible() { return m_visible; }
	void	SetAppIsVisible( bool visible ) { m_visible = visible; } // called internally, to inform us when the window is visible.

	void	SetOrientation( Orientation orient ) { m_orientation = orient; }
	Orientation	GetOrientation() { return m_orientation; }
	vsTransform3D GetOrientationTransform_2D() const;
	vsTransform3D GetOrientationTransform_3D() const;


	static void Launch( const vsString &string );
	int		GetNumberOfCores();		// estimates the number of LOGICAL cores on this computer.  (includes hyperthreading, if available/enabled).  In effect, this returns the number of hardware threads.
	vsString CPUDescription(); // tries to build a string describing the system.

	void LogSystemDetails();

	vsString GetWriteDirectory() const; // returns filesystem path to the write directory

	static vsScreen *		GetScreen() { return Instance()->m_screen; }
	vsSystemPreferences *	GetPreferences() { return m_preferences; }

	// if set 'true', then the default "exit game" key ('q') will be enabled,
	// and will cause execution to return to the main game.  See
	// CORE_Registry.h for details.
	void EnableExitGameKey(bool enable) { m_exitGameKeyEnabled = enable; }
	// if set 'true', then the default "quit" key ('ESC') will be enabled,
	// and will cause the whole application to exit.  If false, you need
	// to handle exiting the application explicitly in game code.  Call
	// core::Exit() when you wish to trigger the application to exit.
	void EnableExitApplicationKey(bool enable) { m_exitApplicationKeyEnabled = enable; }

	bool IsExitGameKeyEnabled() const { return m_exitGameKeyEnabled; }
	bool IsExitApplicationKeyEnabled() const { return m_exitApplicationKeyEnabled; }

	// IsDataPristine returns true if we're loading out a zipfiles and have no
	// mods mounted.  Note that it does not attempt to validate the integrity
	// of the zipfiles themselves;  it ONLY verifies that we're not loading
	// game data from loose files.  (since I figure that nobody's going to
	// bother zipping their files back up after making modifications;  this
	// is only so that crash reports can note whether they were using any custom
	// data files to aid in debugging efforts, and should not be used for any
	// other purpose)
	int IsDataPristine() const { return m_dataIsPristine; }
	void TraceMods() const;

	// by default, we *don't* mount our base directory for reading, but we can
	// do so if the game needs it for some reason (for example, MT2's Windows
	// build can sometimes save out crash reports there, and this is required
	// in order to pick up those crash reports)
	void			MountBaseDirectory();
	void			UnmountBaseDirectory();

	void			ShowErrorMessageBox(const vsString& title, const vsString& message);

	void			AddMod( const vsString& directory, const vsString& mountPoint );
	void			Remount();
};


struct Resolution
{
	int width;
	int height;
};


	/** vsSystemPreferences
	 *		Accessor class to provide managed access to the global preferences data.
	 */
class vsSystemPreferences
{
	vsPreferences *	m_preferences;

	int m_resolution;
	vsPreferenceObject *	m_resolutionX;
	vsPreferenceObject *	m_resolutionY;
	vsPreferenceObject *	m_windowResolutionX;
	vsPreferenceObject *	m_windowResolutionY;
	vsPreferenceObject *	m_fullscreen;
	vsPreferenceObject *	m_fullscreenWindow; // when fullscreen, use 'fullscreen window' mode.
	vsPreferenceObject *	m_vsync;
	vsPreferenceObject *	m_bloom;
	vsPreferenceObject *	m_dynamicBatching;
	vsPreferenceObject *	m_antialias;
	vsPreferenceObject *	m_highDPI;
	vsPreferenceObject *	m_wheelSmoothing;
	vsPreferenceObject *	m_mouseWheelScalePercent;
	vsPreferenceObject *	m_trackpadWheelScalePercent;

	vsPreferenceObject *	m_effectVolume;
	vsPreferenceObject *	m_musicVolume;

	Resolution*	m_supportedResolution;
	int			m_supportedResolutionCount;


public:

	vsSystemPreferences();
	~vsSystemPreferences();

	void			Save();

	void			CheckResolutions();
	int				GetSupportedResolutionCount() { return m_supportedResolutionCount; }
	Resolution *	GetSupportedResolutions() { return m_supportedResolution; }

	// Video preferences
	Resolution *	GetResolution();
	int				GetResolutionId();
	void			SetResolutionId(int val);

	int				GetWindowResolutionX();
	int				GetWindowResolutionY();
	void			SetWindowResolution( int x, int y );

	bool			GetFullscreen();
	void			SetFullscreen(bool fullscreen);

	bool			GetFullscreenWindow();
	void			SetFullscreenWindow(bool fullscreen);

	void			ToggleFullscreen();

	bool			GetVSync();
	void			SetVSync(bool vsync);

	bool			GetBloom();
	void			SetBloom(bool enabled);

	bool			GetDynamicBatching();
	void			SetDynamicBatching(bool enabled);

	bool			GetAntialias();
	void			SetAntialias(bool enabled);

	bool			GetHighDPI();
	void			SetHighDPI(bool allow);

	bool			GetWheelSmoothing();
	void			SetWheelSmoothing(bool smooth);

	// Sound preferences
	int				GetEffectVolume();
	void			SetEffectVolume(int volume);

	int				GetMusicVolume();
	void			SetMusicVolume(int volume);

	float			GetMouseWheelScaling();
	void			SetMouseWheelScaling(float scaling); // range: [0..100]

	float			GetTrackpadWheelScaling();
	void			SetTrackpadWheelScaling(float scaling); // range: [0..100]

};

#endif // VS_SYSTEM_H

