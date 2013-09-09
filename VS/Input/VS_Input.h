/*
 *  VS_Input.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_INPUT_H
#define VS_INPUT_H

#include "Core/CORE_GameSystem.h"
#include "VS/Math/VS_Vector.h"
#include "Utils/VS_Singleton.h"

#if defined __APPLE__
#include "TargetConditionals.h"
#endif

#if !TARGET_OS_IPHONE
#include <SDL2/SDL.h>
#endif

enum ControlID		// IF YOU CHANGE THIS ENUM, UPDATE THE "cidName" STRINGS USED IN LOAD/SAVE!
{
	CID_UpDownLAxis,
	CID_LeftRightLAxis,
	CID_LUp,
	CID_LDown,
	CID_LLeft,
	CID_LRight,

	CID_UpDownRAxis,
	CID_LeftRightRAxis,
	CID_RUp,
	CID_RDown,
	CID_RLeft,
	CID_RRight,

	CID_Up,
	CID_Down,
	CID_Left,
	CID_Right,

	CID_Exit,
	CID_ExitApplication,

	CID_A,
	CID_B,
	CID_X,
	CID_Y,
	CID_TriggerR,
	CID_TriggerL,
	CID_Start,
	CID_Back,

	CID_ZoomIn,
	CID_ZoomOut,

	CID_MouseLeftButton,
	CID_MouseMiddleButton,
	CID_MouseRightButton,

	CID_MouseWheel,
	CID_MouseWheelUp,
	CID_MouseWheelDown,

	CID_Touch0,
	CID_Touch1,

	CID_MAX
};

enum ControlType
{
	CT_None,
	CT_Axis,
	CT_Button,
	CT_Hat,
	CT_MouseButton,

	CT_MAX = CT_MouseButton	// largest legal value
};

enum ControlDirection
{
	CD_Positive,
	CD_Negative,

	CD_Hat_Up,
	CD_Hat_Right,
	CD_Hat_Down,
	CD_Hat_Left,

	CD_MAX = CD_Hat_Left	// largest legal value
};

struct DeviceControl
{
public:
	ControlType		type;
	int				cid;

	ControlDirection	dir;	// outputs range from 0..-1, instead of 0..1.  Useful for analog axes.

	DeviceControl() { type = CT_None; cid = 0; dir = CD_Positive; }
	void	Set(ControlType type_in, int cid_in, ControlDirection dir_in = CD_Positive) { type = type_in; cid = cid_in; dir = dir_in; }
};

class vsInput : public coreGameSystem, public vsSingleton<vsInput>
{
#if !TARGET_OS_IPHONE
	SDL_Joystick	*m_joystick;
#endif

	int				m_joystickAxes;
	int				m_joystickButtons;
	int				m_joystickHats;

	float			*m_axisCenter;
	float			*m_axisThrow;		// how far can this control go from center?

	DeviceControl	m_controlMapping[CID_MAX];
	DeviceControl	m_pollResult;
	bool			m_mappingsChanged;

	float			m_keyControlState[CID_MAX];

	float			m_controlState[CID_MAX];
	float			m_lastControlState[CID_MAX];

	vsVector2D		m_mousePos;
	vsVector2D		m_mouseMotion;					// how much has the mouse moved this frame?
	bool			m_captureMouse;
	bool			m_suppressFirstMotion;			// suppress our first motion amount after warping, just to be safe.
	int				m_capturedMouseX;
	int				m_capturedMouseY;

	bool			m_preparingToPoll;
	bool			m_pollingForDeviceControl;			// are we waiting for an arbitrary control input?  (typically for the purposes of mapping device controls to our virtual controller)

	vsString		m_stringModeString;
	bool			m_stringMode;						// if true, interpret all keyboard keys as entering a string.

	void			ReadAxis( int axisID, ControlDirection dir, int cid );
	void			ReadButton( int buttonID, int cid );

	float			ReadHat(int hatID, ControlDirection dir);
	void			ReadHat( int hatID, ControlDirection dir, int cid );

	float			ReadAxis( int axisID );
	float			ReadButton( int buttonID );

	float			ReadMouseButton( int axisID );
	void			ReadMouseButton( int axisID, int cid );

	float			ReadAxis_Raw( int axisID );

	bool			WasDown( ControlID id );

	void			Save();
	void			Load();

	vsVector2D		Correct2DInputForOrientation( const vsVector2D &input );

public:

	vsInput();
	virtual ~vsInput();

	virtual void	Init();
	virtual void	Deinit();

	virtual void	Update( float timeStep );

	vsVector2D		GetLeftStick() { return vsVector2D( GetState(CID_LeftRightLAxis), GetState(CID_UpDownLAxis) ); }
	vsVector2D		GetRightStick() { return vsVector2D( GetState(CID_LeftRightRAxis), GetState(CID_UpDownRAxis) ); }

	bool			HasTouch(int touchID);
	int				GetMaxTouchCount();
	vsVector2D		GetTouchPosition(int touchID, int scene = 0);

	bool			MouseIsOnScreen();
	vsVector2D		GetWindowMousePosition(); // returns the mouse position in pixels, relative to the window.
	vsVector2D		GetMousePosition(int scene = 0); // returns the mouse position relative to the rendering context of the specified scene.
	vsVector2D		GetMouseMotion(int scene = 0);
	void			CaptureMouse( bool capture );
	bool			IsMouseCaptured() { return m_captureMouse; }

	float			GetState( ControlID id ) { return m_controlState[id]; }
	bool			IsUp( ControlID id ) { return !IsDown( id ); }
	bool			IsDown( ControlID id );
	bool			WasPressed( ControlID id );
	bool			WasReleased( ControlID id );

	void			SetStringMode(bool mode);
	bool			InStringMode() { return m_stringMode; }
	void			SetStringModeString( const vsString &s ) { m_stringModeString = s; }
	vsString		GetStringModeString() { return m_stringModeString; }

	void			StartPollingForDeviceControl() { m_preparingToPoll = true; }
	bool			IsPolling() { return m_pollingForDeviceControl || m_preparingToPoll; }
	DeviceControl *	GetPollResult() { return &m_pollResult; }

	DeviceControl *	GetControlMapping( ControlID id ) { return &m_controlMapping[id]; }
	void			SetControlMapping( ControlID id, DeviceControl *dc ) { m_controlMapping[id] = *dc; m_mappingsChanged = true; }
};

#endif // VS_INPUT_H
