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
	void InitPhysFS(int argc, char* argv[], const vsString& companyName, const vsString& title);
	void DeinitPhysFS();

public:

	static vsSystem *	Instance() { return s_instance; }

	vsSystem( const vsString& companyName, const vsString& title, int argc, char* argv[], size_t totalMemoryBytes = 1024*1024*64, size_t minBuffers = 1 );
	~vsSystem();

	void Init();
	void Deinit();

	const vsString& GetTitle() const { return m_title; }

	void		EnableGameDirectory( const vsString &directory );
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

	static void Launch( const vsString &string );
	int		GetNumberOfCores();		// estimates the number of cores on this computer.
	vsString CPUDescription(); // tries to build a string describing the system.

	void LogSystemDetails();

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

