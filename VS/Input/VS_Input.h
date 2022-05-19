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
#include "VS/Math/VS_Box.h"
#include "Utils/VS_Singleton.h"
#include "Utils/VS_Array.h"
#include "Utils/VS_ArrayStore.h"

#if defined __APPLE__
#include "TargetConditionals.h"
#endif

#if !TARGET_OS_IPHONE
#include "SDL.h"
#endif

// Macros for defining custom axes to watch.  See vsInput::Load() for
// an example of their usage.  Note that these really only work properly
// if you disable the "VS_DEFAULT_VIRTUAL_CONTROLLER" build setting;  these
// are the macros you use to define your own, custom controller and axes.
#define DEFAULT_BIND_KEY(cid, description, scancode) \
{ \
	vsInput::Instance()->AddAxis(cid, #cid, description); \
	vsInput::Instance()->DefaultBindKey(cid, scancode); \
}
#define DEFAULT_BIND_MOUSE_BUTTON(cid, description, mouseButtonCode) \
{ \
	vsInput::Instance()->AddAxis(cid, #cid, description); \
	vsInput::Instance()->DefaultBindMouseButton(cid, mouseButtonCode); \
}
#define DEFAULT_BIND_MOUSE_WHEEL(cid, description, cdirection) \
{ \
	vsInput::Instance()->AddAxis(cid, #cid, description); \
	vsInput::Instance()->DefaultBindMouseWheel(cid, cdirection); \
}

#define SUBTRACTION_AXIS(cid, description, positive, negative) \
{ \
	vsInput::Instance()->AddAxis(cid, #cid, description); \
	vsInput::Instance()->SetAxisAsSubtraction(cid, positive, negative); \
}

#define DEFAULT_BIND_CONTROLLERAXIS(cid, description, controllerAxis, cdirection) \
{ \
	vsInput::Instance()->AddAxis(cid, #cid, description); \
	vsInput::Instance()->DefaultBindControllerAxis(cid, controllerAxis, cdirection); \
}

#define DEFAULT_BIND_CONTROLLERBUTTON(cid, description, controllerButton ) \
{ \
	vsInput::Instance()->AddAxis(cid, #cid, description); \
	vsInput::Instance()->DefaultBindControllerButton(cid, controllerButton ); \
}

#ifdef VS_DEFAULT_VIRTUAL_CONTROLLER

enum ControlID
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
	CID_Escape,

	CID_A,
	CID_B,
	CID_X,
	CID_Y,
	CID_TriggerR,
	CID_TriggerL,
	CID_Start,
	CID_Back,

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
#endif // VS_DEFAULT_VIRTUAL_CONTROLLER

// NOTE:  If you change ControlType, change the string array
// in the source file to match, or serialisation will break!
enum ControlType
{
	CT_None,
	CT_Axis,
	CT_Button,
	CT_Hat,
	CT_MouseButton,
	CT_MouseWheel,
	CT_Keyboard,
	CT_MAX
};

// NOTE:  If you change ControlDirection, change the string array
// in the source file to match, or serialisation will break!
enum ControlDirection
{
	CD_Positive,
	CD_Negative,

	CD_Hat_Up,
	CD_Hat_Right,
	CD_Hat_Down,
	CD_Hat_Left,
	CD_MAX
};

struct DeviceControl
{
public:
	ControlType		type;
	// 'id' is the axis/button/hat/etc id.  For 'Keyboard' devices, it is the scancode.
	// Also for 'Keyboard' devices, 'keymod' is any required keyboard modifiers.  (alt/etc)
	int				id;
	int				keymod;
	// For controller types, 'controllerId' is the controller id value.
	int				controllerId;

	ControlDirection	dir;	// outputs range from 0..-1, instead of 0..1.  Useful for analog axes.

	DeviceControl() { type = CT_None; id = 0; keymod = 0; controllerId = 0; dir = CD_Positive; }
	void	Set(ControlType type_in, int id_in, ControlDirection dir_in = CD_Positive) { type = type_in; id = id_in; dir = dir_in; }
	void	Set(ControlType type_in, int controllerId_in, int id_in, ControlDirection dir_in = CD_Positive) { type = type_in; controllerId = controllerId_in; id = id_in; dir = dir_in; }

	float	Evaluate(bool hasFocus);
};

struct vsInputAxis
{
	vsString name;
	vsString description;

	vsArray<DeviceControl> positive;
	// DeviceControl negative[MAX_CONTROL_BINDS];

	float lastValue;
	float currentValue;
	bool wasPressed;
	bool wasReleased;
	bool isLoaded;
	bool isCalculated;

	int positiveAxisId;
	int negativeAxisId;

	vsInputAxis();
	~vsInputAxis() { positive.Clear(); }
	void Update( bool hasFocus, bool hadFocus );
};

class vsController
{
#if !TARGET_OS_IPHONE
	SDL_Joystick	*m_joystick;
	SDL_GameController *m_controller;
#endif
	int				m_joystickAxes;
	int				m_joystickButtons;
	int				m_joystickHats;

	float			*m_axisCenter;
	float			*m_axisThrow;		// how far can this control go from center?

public:
	vsController( SDL_GameController *controller, int index );
	~vsController();

	bool			Matches( SDL_GameController *gc ) { return m_controller == gc; }

	float			ReadHat(int hatID, ControlDirection dir);

	float			ReadAxis( int axisID );
	float			ReadAxis( int axisID, ControlDirection dir );
	float			ReadButton( int buttonID );

	float			ReadAxis_Raw( int axisID );

};

class vsInput : public coreGameSystem, public vsSingleton<vsInput>
{
#define MAX_JOYSTICKS (4)
	vsController* m_controller[MAX_JOYSTICKS];

	// DeviceControl	m_controlMapping[CID_MAX];
	DeviceControl	m_pollResult;
	// bool			m_mappingsChanged;


	vsArray<vsInputAxis> m_axis;
	vsArray<vsInputAxis> m_loadedAxis;

	// float			m_keyControlState[CID_MAX];

	// float			m_controlState[CID_MAX];
	// float			m_lastControlState[CID_MAX];

	float m_wheelValue;

	vsVector2D		m_mousePos;
	vsVector2D		m_mouseMotion;					// how much has the mouse moved this frame?
	bool			m_captureMouse;
	bool			m_suppressFirstMotion;			// suppress our first motion amount after warping, just to be safe.
	bool			m_suppressResizeEvent;			// suppress our next resize event after a window mode change, just to be safe.
	int				m_capturedMouseX;
	int				m_capturedMouseY;
	int				m_fingersDown;
	float			m_fingersDownTimer;

	float			m_timeSinceAnyInput;

	bool			m_preparingToPoll;
	bool			m_pollingForDeviceControl;			// are we waiting for an arbitrary control input?  (typically for the purposes of mapping device controls to our virtual controller)

	void HandleStringModeKeyDown( const SDL_Event& event );
	void HandleKeyDown( const SDL_Event& event );
	void HandleKeyUp( const SDL_Event& event );
	void HandleMouseButtonEvent( const SDL_Event& event );

	bool AnythingIsDown();

public:
	enum ValidationType
	{
		Validation_None,
		Validation_Filename,
		Validation_Numeric,
		Validation_PositiveNumeric,
		Validation_Integer,
		Validation_PositiveInteger,
		Validation_PositiveIntegerPercent
	};
private:

	bool			m_stringMode;						// if true, interpret all keyboard keys as entering a string.
	bool			m_stringModeClearing;				// if true, we're waiting for all keyboard keys to be released before re-enabling control.
	bool			m_stringModePaused;					// if true, all keyboard key input is ignored in string mode, to allow a game to adjust cursor position without keypresses doing anything
	int				m_stringModeMaxLength;				// if positive, this is how many glyphs we can have in our string!

	bool m_hasFocus;
	bool m_hadFocus;

	class StringModeState
	{
	public:
		StringModeState() {}

		vsString string;
		int anchorGlyph;
		int floatingGlyph;
	};
	vsArrayStore<StringModeState> m_stringModeUndoStack;
	// When selecting text, one side or the other is the "anchor";  it's the
	// true cursor position.  When we click on text, it's wherever we clicked
	// before dragging.  When we create a selection using shift+arrow keys, It's
	// wherever the cursor was before using shift+arrows.  This side of the selection
	// never moves;  only the other end is moved around by shift+arrows.
	int				m_stringModeCursorAnchorGlyph;
	int				m_stringModeCursorFloatingGlyph;
	int				m_stringModeCursorFirstGlyph;		// we go from before the first glyph
	int				m_stringModeCursorLastGlyph;		// to before the last glyph  (legal to specify glyph values past the end of the string, to put cursor at the very end)
	vsString		m_stringModeString;

	ValidationType	m_stringValidationType;

	bool			m_mouseIsInWindow;
	bool			m_wheelSmoothing; // enable or disable wheel "smoothing" support.
	float			m_wheelSpeed;

	float			ReadMouseButton( int axisID );

	vsController *GetController();
	// void			ReadMouseButton( int axisID, int cid );

	bool			WasDown( int id );

	void			Save();
	void			Load();

	vsVector2D		Correct2DInputForOrientation( const vsVector2D &input );
	void ValidateString();

	bool m_backspaceMode; // set to 'true' if we've just hit backspace.  We don't trigger an undo state for second and subsequent backspaces.
	bool m_undoMode; // did we just undo?
	bool m_stringModeEditing; // set to 'true' if we've modified the string.  Set to 'false' when we move the cursor.
	void StringModeSaveUndoState();
	bool StringModeUndo();
	void StringModeCancel();

	void InitController(int i);
	void DestroyController(SDL_GameController *gc);

public:

	vsInput();
	virtual ~vsInput();

	virtual void	Init();
	virtual void	Deinit();

	virtual void	Update( float timeStep );

	// vsVector2D		GetLeftStick() { return vsVector2D( GetState(CID_LeftRightLAxis), GetState(CID_UpDownLAxis) ); }
	// vsVector2D		GetRightStick() { return vsVector2D( GetState(CID_LeftRightRAxis), GetState(CID_UpDownRAxis) ); }

	bool			HasTouch(int touchID);
	int				GetMaxTouchCount();
	vsVector2D		GetTouchPosition(int touchID, int scene = 0);

	float			TimeSinceAnyInput() const { return m_timeSinceAnyInput; }


	float			ReadHat(int hatID, ControlDirection dir);
	float			ReadAxis( int axisID );
	float			ReadButton( int buttonID );

	bool			MouseIsOnScreen();
	vsVector2D		GetWindowMousePosition(); // returns the mouse position in pixels, relative to the window.
	vsVector2D		GetMousePosition(int scene = 0); // returns the mouse position relative to the rendering context of the specified scene.
	vsVector2D		GetMouseMotion(int scene = 0);
	void			CaptureMouse( bool capture );
	bool			IsMouseCaptured() { return m_captureMouse; }

	bool			HasController() { return GetController() != nullptr; }

	float			GetState( int id ) { return m_axis[id].currentValue; } //m_controlState[id]; }
	bool			IsUp( int id ) { return !IsDown( id ); }
	bool			IsDown( int id );
	bool			WasPressed( int id );
	bool			WasReleased( int id );

	void			ConsumePress( int id ); // resets our 'pressed' and 'released' states for the given control

	void			SuppressResizeEvent() { m_suppressResizeEvent = true; }

	void			SetStringMode(bool mode, const vsBox2D& where, ValidationType = Validation_None);
	void			SetStringMode(bool mode, int maxLength, const vsBox2D& where, ValidationType vt = Validation_None);
	void			PauseStringMode(bool pause); // used if a game wants to temporarily suspend handling of keypresses, for example while manipulating a selection by the mouse
	bool			InStringMode() { return m_stringMode; }
	bool			IsKeyboardSuppressed() { return m_stringMode || m_stringModeClearing; }
	void			SetStringModeString( const vsString &s );
	vsString		GetStringModeString() { return m_stringModeString; }
	vsString		GetStringModeSelection();
	void			SetStringModeCursor( int anchorGlyph, bool endEdit ); // collapse the floating glyph.  'endEdit' means that we've reached an 'undo' point BEFORE this cursor movement.
	void			SetStringModeCursor( int anchorGlyph, int floatingGlyph, bool endEdit );
	int				GetStringModeCursorFirstGlyph();
	int				GetStringModeCursorLastGlyph();
	int				GetStringModeCursorAnchorGlyph();
	int				GetStringModeCursorFloatingGlyph();
	void			SetStringModeSelectAll( bool selectAll );
	bool			GetStringModeSelectAll();
	void HandleTextInput( const vsString& input );

	void			StartPollingForDeviceControl() { m_preparingToPoll = true; }
	void			CancelPollingForDeviceControl() { m_preparingToPoll = m_pollingForDeviceControl = false; }
	bool			IsPolling() { return m_pollingForDeviceControl || m_preparingToPoll; }
	DeviceControl *	GetPollResult() { return &m_pollResult; }

	// DeviceControl *	GetControlMapping( ControlID id ) { return &m_controlMapping[id]; }
	// void			SetControlMapping( ControlID id, DeviceControl *dc ) { m_controlMapping[id] = *dc; m_mappingsChanged = true; }

	void AddAxis( int cid, const vsString& name, const vsString& description );
	void DefaultBindKey( int cid, int scancode );
	void DefaultBindControllerAxis( int cid, int controllerAxis, ControlDirection cd );
	void DefaultBindControllerButton( int cid, int controllerButton );
	void DefaultBindMouseButton( int cid, int mouseButtonCode );
	void DefaultBindMouseWheel( int cid, ControlDirection cd );
	void SetAxisAsSubtraction( int cid, int positiveAxis, int negativeAxis);

	// Tells us to clear all of our axis and binding information.  Presumably the
	// game is about to initialise us with something different.
	void ClearAxes();

	void Rebind( int cid, const DeviceControl& dc );

	int GetAxisCount() const { return m_axis.ItemCount(); }
	const struct vsInputAxis& GetAxis(int i) { return m_axis[i]; }
	const struct vsInputAxis* GetAxis(const vsString& name);
	int GetAxisId(const vsString& name); // returns -1 for failure

	vsString GetBindDescription( const DeviceControl& dc );

	void WarpMouseTo( int scene, const vsVector2D& position );

	friend struct vsInputAxis;
	friend struct DeviceControl;
};

#endif // VS_INPUT_H
