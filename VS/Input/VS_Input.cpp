/*
 *  SYS_Input.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Input.h"

#include "Core.h"
#include "CORE_Game.h"

#include "VS_Camera.h"
#include "VS_Scene.h"
#include "VS_Screen.h"
#include "VS_System.h"

#include "VS_MaterialManager.h" // TEMP:  For triggering material reload.
#include "VS_Shader.h" // TEMP:  For triggering shader reload.

#include "VS_File.h"
#include "VS_Record.h"
#include "VS_Profile.h"

#include "VS_Preferences.h"
#include "Utils/utfcpp/utf8.h"
#include "Utils/uni-algo/src/cpp_uni_break_grapheme.h"
#include <iterator>


#include "VS_Heap.h"

#if TARGET_OS_IPHONE
#include "Wedge.h"
#else
#include <SDL2/SDL.h>
#endif

extern SDL_Window *g_sdlWindow;

namespace
{
	// the modifier keys we should check for key combos.
	// Use L/R GUI on Mac ("command" key), or Control on everything else.
	const int c_shortcutModifierKeys =
#if defined( __APPLE_CC__ )
		KMOD_LGUI | KMOD_RGUI;
#else
		KMOD_LCTRL | KMOD_RCTRL;
#endif // defined( __APPLE_CC__ )
}

vsInput::vsInput():
	m_stringMode(false),
	m_stringModeIsMultiline(false),
	m_hasFocus(true),
	m_hadFocus(true),
	m_stringValidationType(Validation_None),
	m_mouseIsInWindow(false),
	m_backspaceMode(false)
{
	m_captureMouse = false;
	m_suppressFirstMotion = false;
	m_suppressResizeEvent = false;
	m_cursorHandler = nullptr;
	m_dropHandler = nullptr;
}

vsInput::~vsInput()
{
}

void
vsInput::Init()
{
	// In the documentation, SDL2 implies that "text input" mode starts disabled,
	// but at least in the current OSX build, it actually starts enabled.  So during
	// startup, let's just check and see if it's turned on, and if so, turn it off
	// explicitly.
	if ( SDL_IsTextInputActive() )
	{
		vsLog("TextInput was enabled by default; turning it off until we need it!");
		SDL_StopTextInput();
	}

	// Ability to turn off "simulated mouse events" for touch events was added
	// in SDL 2.0.7
#if SDL_VERSION_ATLEAST( 2, 0, 7 )
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif

	m_fingersDown = 0;
	m_fingersDownTimer = 0.f;
	m_timeSinceAnyInput = std::numeric_limits<float>::max();
	m_preparingToPoll = false;
	m_pollingForDeviceControl = false;
	m_stringMode = false;
	m_stringModeIsMultiline = false;
	m_stringModeClearing = false;
	vsSystem::Instance()->GetPreferences();
	m_wheelSmoothing = true;
	m_wheelSpeed = 0.f;

	// for ( int i = 0; i < CID_MAX; i++ )
	// {
	// 	m_lastControlState[i] = 0.f;
	// 	m_controlState[i] = 0.f;
	// 	m_keyControlState[i] = 0.f;
	// }
    //
#if !TARGET_OS_IPHONE
	for ( int i = 0; i < MAX_JOYSTICKS; i++ )
		m_controller[i] = nullptr;

	// SDL_GameControllerAddMapping("030000005e040000a102000000010000,Xbox 360 Wireless Receiver,a:b1,b:b2,y:b3,x:b0,leftx:a0,lefty:a1");
	// int joystickCount = SDL_NumJoysticks();
    //
	// if ( joystickCount )
	// {
	// 	vsLog("Found %d joysticks:", joystickCount);
    //
	// 	for ( int i = 0; i < joystickCount; i++ )
	// 	{
	// 		SDL_Joystick* js = SDL_JoystickOpen(i);
	// 		const char* name = SDL_JoystickName(js);
    //
	// 		vsLog(" %d: %s", i, name ? name : "(unknown)");
    //
	// 		SDL_JoystickClose(js);
	// 	}
	// 	for ( int i = 0; i < joystickCount; i++ )
	// 	{
	// 		if ( SDL_IsGameController(i) )
	// 		{
	// 			SDL_GameController *gc = SDL_GameControllerOpen(i);
	// 			if ( gc )
	// 				m_controller[i] = new vsController(gc, i);
	// 		}
	// 		else
	// 		{
	// 			vsLog("Controller %d doesn't seem to be a controller??", i);
	// 		}
	// 	}
	// }
	// else
	// 	vsLog("No joystick found.  Using keyboard input.");


	Load();

	Update(0.0f);	// run a zero-time update, so we can correctly identify key presses vs. holds on the first frame.
#endif
}

vsController::vsController( SDL_GameController *controller, int i )
{
	m_controller = controller;

	const char *name = SDL_GameControllerNameForIndex(i);
	vsLog("Game controller %d found: %s", i, name);

	m_joystick = SDL_GameControllerGetJoystick(m_controller);
	if ( m_joystick )
	{
		m_joystickAxes = SDL_JoystickNumAxes(m_joystick);
		m_joystickButtons = SDL_JoystickNumButtons(m_joystick);
		m_joystickHats = SDL_JoystickNumHats(m_joystick);
		vsLog("Device has %d axes, %d buttons, %d hats, %d balls",
				SDL_JoystickNumAxes(m_joystick),
				SDL_JoystickNumButtons(m_joystick),
				SDL_JoystickNumHats(m_joystick),
				SDL_JoystickNumBalls(m_joystick));

		m_axisCenter = new float[m_joystickAxes];
		m_axisThrow = new float[m_joystickAxes];
		for ( int i = 0; i < m_joystickAxes; i++ )
		{
			m_axisCenter[i] = ReadAxis_Raw(i);
			m_axisThrow[i] = 1.0f;
		}
	}
}

vsController::~vsController()
{
	SDL_GameControllerClose( m_controller );
	vsDeleteArray( m_axisCenter );
	vsDeleteArray( m_axisThrow );
}

extern vsHeap *g_globalHeap;

void
vsInput::Deinit()
{
	Save();
	vsHeap::Push(g_globalHeap);
	m_axis.Clear();
	vsHeap::Pop(g_globalHeap);

	for ( int i = 0; i < MAX_JOYSTICKS; i++ )
		vsDelete( m_controller[i] );
}

void
vsInput::ClearAxes()
{
	vsHeap::Push(g_globalHeap);

	m_axis.Clear();
	m_loadedAxis.Clear();

	vsHeap::Pop(g_globalHeap);
}

void
vsInput::AddAxis( int cid, const vsString& name, const vsString& description )
{
	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially adding extra
	// axes to our array doesn't get charged to the currently active game, and
	// then treated as a memory leak)
	vsHeap::Push(g_globalHeap);

	if ( m_axis.ItemCount() <= cid )
	{
		m_axis.SetArraySize(cid+1);
	}

	if ( m_axis[cid].name != name )
	{
		// not already loaded in, here.  Let's check whether we have something with
		// this name loaded, and copy it into place, if so.
		for ( int i = 0; i < m_loadedAxis.ItemCount(); i++ )
		{
			if ( m_loadedAxis[i].name == name )
			{
				m_axis[cid] = m_loadedAxis[i];
				break;
			}
		}

		m_axis[cid].name = name;
	}
	m_axis[cid].description = description;

	vsHeap::Pop(g_globalHeap);
}

void
vsInput::DefaultBindKey( int cid, int scancode )
{
	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially adding extra
	// axes to our array doesn't get charged to the currently active game, and
	// then treated as a memory leak)
	vsHeap::Push(g_globalHeap);

	if ( !m_axis[cid].isLoaded )
	{
		DeviceControl dc;
		dc.type = CT_Keyboard;
		dc.id = scancode;

		m_axis[cid].positive.AddItem(dc);
	}

	vsHeap::Pop(g_globalHeap);
}

void
vsInput::DefaultBindControllerButton( int cid, int controllerButton )
{
	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially adding extra
	// axes to our array doesn't get charged to the currently active game, and
	// then treated as a memory leak)
	vsHeap::Push(g_globalHeap);

	if ( !m_axis[cid].isLoaded )
	{
		DeviceControl dc;
		dc.type = CT_Button;
		dc.id = controllerButton;

		m_axis[cid].positive.AddItem(dc);
	}

	vsHeap::Pop(g_globalHeap);
}

void
vsInput::DefaultBindControllerAxis( int cid, int controllerAxis, ControlDirection cd )
{
	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially adding extra
	// axes to our array doesn't get charged to the currently active game, and
	// then treated as a memory leak)
	vsHeap::Push(g_globalHeap);

	if ( !m_axis[cid].isLoaded )
	{
		DeviceControl dc;
		dc.type = CT_Axis;
		dc.id = controllerAxis;
		dc.dir = cd;

		m_axis[cid].positive.AddItem(dc);
	}

	vsHeap::Pop(g_globalHeap);
}

void
vsInput::DefaultBindMouseButton( int cid, int mouseButtonCode )
{
	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially adding extra
	// axes to our array doesn't get charged to the currently active game, and
	// then treated as a memory leak)
	vsHeap::Push(g_globalHeap);

	if ( !m_axis[cid].isLoaded )
	{
		DeviceControl dc;
		dc.type = CT_MouseButton;
		dc.id = mouseButtonCode;

		m_axis[cid].positive.AddItem(dc);
	}

	vsHeap::Pop(g_globalHeap);
}

void
vsInput::DefaultBindMouseWheel( int cid, ControlDirection cd )
{
	// Since this function is being called post-initialisation, we need to
	// switch back to our system heap.  (So that potentially adding extra
	// axes to our array doesn't get charged to the currently active game, and
	// then treated as a memory leak)
	vsHeap::Push(g_globalHeap);

	if ( !m_axis[cid].isLoaded )
	{
		DeviceControl dc;
		dc.type = CT_MouseWheel;
		dc.dir = cd;

		m_axis[cid].positive.AddItem(dc);
	}

	vsHeap::Pop(g_globalHeap);
}

void
vsInput::SetAxisAsSubtraction( int cid, int positiveAxis, int negativeAxis)
{
	m_axis[cid].isCalculated = true;
	m_axis[cid].positiveAxisId = positiveAxis;
	m_axis[cid].negativeAxisId = negativeAxis;
}

const vsString	c_controlTypeString[] =
{
	"CT_None",
	"CT_Axis",
	"CT_Button",
	"CT_Hat",
	"CT_MouseButton",
	"CT_MouseWheel",
	"CT_Keyboard"
};

static ControlType ControlTypeFromString( const vsString& s )
{
	for ( int i = 0; i < CT_MAX; i++ )
	{
		if ( s == c_controlTypeString[i] )
			return (ControlType)i;
	}
	return CT_None;
}

const vsString	c_controlDirectionString[] =
{
	"CD_Positive",
	"CD_Negative",
	"CD_Hat_Up",
	"CD_Hat_Right",
	"CD_Hat_Down",
	"CD_Hat_Left"
};

static ControlDirection ControlDirectionFromString( const vsString& s )
{
	for ( int i = 0; i < CT_MAX; i++ )
	{
		if ( s == c_controlDirectionString[i] )
			return (ControlDirection)i;
	}
	return CD_Positive;
}

void
vsInput::Load()
{
	vsHeap::Push(g_globalHeap);
	if ( vsFile::Exists("user/binds.txt") )
	{
		vsFile f("user/binds.txt", vsFile::MODE_Read);

		vsRecord a;
		while ( f.Record(&a) )
		{
			if ( a.GetLabel().AsString() != "Axis" )
				continue;

			vsInputAxis axis;
			for ( int j = 0; j < a.GetChildCount(); j++ )
			{
				vsRecord *child = a.GetChild(j);
				vsString label = child->GetLabel().AsString();
				if ( label == "name" )
					axis.name = child->GetToken(0).AsString();
				else if ( label == "DeviceControl" )
				{
					DeviceControl dc;
					dc.type = ControlTypeFromString( child->GetToken(0).AsString() );
					dc.dir = ControlDirectionFromString( child->GetToken(1).AsString() );
					dc.id = child->GetToken(2).AsInteger();

					axis.positive.AddItem(dc);
				}
			}

			axis.isLoaded = true;
			m_loadedAxis.AddItem(axis);
		}
	}

#ifdef VS_DEFAULT_VIRTUAL_CONTROLLER
	DEFAULT_BIND_KEY(CID_LUp, "LUp", SDL_SCANCODE_W);
	DEFAULT_BIND_KEY(CID_LUp, "LUp", SDL_SCANCODE_UP);
	DEFAULT_BIND_KEY(CID_LLeft, "LLeft", SDL_SCANCODE_A);
	DEFAULT_BIND_KEY(CID_LLeft, "LLeft", SDL_SCANCODE_LEFT);
	DEFAULT_BIND_KEY(CID_LRight, "LRight", SDL_SCANCODE_D);
	DEFAULT_BIND_KEY(CID_LRight, "LRight", SDL_SCANCODE_RIGHT);
	DEFAULT_BIND_KEY(CID_LDown, "LDown", SDL_SCANCODE_S);
	DEFAULT_BIND_KEY(CID_LDown, "LDown", SDL_SCANCODE_DOWN);
	SUBTRACTION_AXIS( CID_UpDownLAxis, "UpDownLAxis", CID_LDown, CID_LUp );
	SUBTRACTION_AXIS( CID_LeftRightLAxis, "LeftRightLAxis", CID_LRight, CID_LLeft );

	SUBTRACTION_AXIS( CID_UpDownRAxis, "UpDownRAxis", CID_RDown, CID_RUp );
	SUBTRACTION_AXIS( CID_LeftRightRAxis, "LeftRightRAxis", CID_RRight, CID_RLeft );

	DEFAULT_BIND_KEY(CID_Up, "Up", SDL_SCANCODE_W);
	DEFAULT_BIND_KEY(CID_Up, "Up", SDL_SCANCODE_UP);
	DEFAULT_BIND_KEY(CID_Left, "Left", SDL_SCANCODE_A);
	DEFAULT_BIND_KEY(CID_Left, "Left", SDL_SCANCODE_LEFT);
	DEFAULT_BIND_KEY(CID_Right, "Right", SDL_SCANCODE_D);
	DEFAULT_BIND_KEY(CID_Right, "Right", SDL_SCANCODE_RIGHT);
	DEFAULT_BIND_KEY(CID_Down, "Down", SDL_SCANCODE_S);
	DEFAULT_BIND_KEY(CID_Down, "Down", SDL_SCANCODE_DOWN);

	DEFAULT_BIND_KEY(CID_A, "A,", SDL_SCANCODE_SPACE);
	DEFAULT_BIND_KEY(CID_B, "B,", SDL_SCANCODE_LALT);
	DEFAULT_BIND_KEY(CID_Y, "Y,", SDL_SCANCODE_LSHIFT);

	DEFAULT_BIND_KEY(CID_Escape, "Escape", SDL_SCANCODE_ESCAPE);
	DEFAULT_BIND_KEY(CID_Exit, "Exit", SDL_SCANCODE_Q);
	DEFAULT_BIND_KEY(CID_Back, "Back", SDL_SCANCODE_BACKSPACE);

	DEFAULT_BIND_MOUSE_BUTTON( CID_MouseLeftButton, "Mouse Left Button", SDL_BUTTON_LEFT );
	DEFAULT_BIND_MOUSE_BUTTON( CID_MouseMiddleButton, "Mouse Middle Button", SDL_BUTTON_MIDDLE );
	DEFAULT_BIND_MOUSE_BUTTON( CID_MouseRightButton, "Mouse Right Button", SDL_BUTTON_RIGHT );

	DEFAULT_BIND_MOUSE_WHEEL( CID_MouseWheelUp, "Mouse Wheel Up", CD_Positive );
	DEFAULT_BIND_MOUSE_WHEEL( CID_MouseWheelDown, "Mouse Wheel Down", CD_Negative );

	SUBTRACTION_AXIS( CID_MouseWheel, "Mouse Wheel", CID_MouseWheelDown, CID_MouseWheelUp );
#endif // VS_DEFAULT_VIRTUAL_CONTROLLER

#if 0
#if !TARGET_OS_IPHONE
	// set some sensible defaults.  (XBox 360 gamepad)
	if ( m_controller )
	{
		m_controlMapping[CID_LLeft].Set( CT_Axis, SDL_CONTROLLER_AXIS_LEFTX, CD_Negative );
		m_controlMapping[CID_LRight].Set( CT_Axis, SDL_CONTROLLER_AXIS_LEFTX, CD_Positive );
		m_controlMapping[CID_LUp].Set( CT_Axis, SDL_CONTROLLER_AXIS_LEFTY, CD_Negative );
		m_controlMapping[CID_LDown].Set( CT_Axis, SDL_CONTROLLER_AXIS_LEFTY, CD_Positive );

		m_controlMapping[CID_RLeft].Set( CT_Axis, SDL_CONTROLLER_AXIS_RIGHTX, CD_Negative );
		m_controlMapping[CID_RRight].Set( CT_Axis, SDL_CONTROLLER_AXIS_RIGHTX, CD_Positive );
		m_controlMapping[CID_RUp].Set( CT_Axis, SDL_CONTROLLER_AXIS_RIGHTY, CD_Negative );
		m_controlMapping[CID_RDown].Set( CT_Axis, SDL_CONTROLLER_AXIS_RIGHTY, CD_Positive );

		m_controlMapping[CID_TriggerL].Set( CT_Axis, SDL_CONTROLLER_AXIS_TRIGGERLEFT, CD_Positive );
		m_controlMapping[CID_TriggerR].Set( CT_Axis, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, CD_Positive );

		m_controlMapping[CID_A].Set( CT_Button, SDL_CONTROLLER_BUTTON_A );
		m_controlMapping[CID_B].Set( CT_Button, SDL_CONTROLLER_BUTTON_B );
		m_controlMapping[CID_X].Set( CT_Button, SDL_CONTROLLER_BUTTON_X );
		m_controlMapping[CID_Y].Set( CT_Button, SDL_CONTROLLER_BUTTON_Y );
		// int aButton = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A);
		// int bButton = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_B);
		//
		// // int hatUp = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
		// // int hatDown = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		// // int hatLeft = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		// // int hatRight = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
	}
	else
	{
		if ( m_joystickAxes >= 2 )
		{
			m_controlMapping[CID_LLeft].Set( CT_Axis, 0, CD_Negative );
			m_controlMapping[CID_LRight].Set( CT_Axis, 0, CD_Positive );
			m_controlMapping[CID_LUp].Set( CT_Axis, 1, CD_Negative );
			m_controlMapping[CID_LDown].Set( CT_Axis, 1, CD_Positive );
		}
		if ( m_joystickAxes >= 4 )
		{
			m_controlMapping[CID_RLeft].Set( CT_Axis, 2, CD_Negative );
			m_controlMapping[CID_RRight].Set( CT_Axis, 2, CD_Positive );
			m_controlMapping[CID_RUp].Set( CT_Axis, 3, CD_Negative );
			m_controlMapping[CID_RDown].Set( CT_Axis, 3, CD_Positive );
		}
		if ( m_joystickAxes >= 6 )
		{
			m_controlMapping[CID_TriggerL].Set( CT_Axis, 4, CD_Positive );
			m_controlMapping[CID_TriggerR].Set( CT_Axis, 5, CD_Positive );
		}

		if ( m_joystickButtons >= 2 )
		{
			m_controlMapping[CID_A].Set( CT_Button, 0 );
			m_controlMapping[CID_B].Set( CT_Button, 1 );
		}

		if ( m_joystickHats > 0 )
		{
			// m_controlMapping[CID_Up].Set( CT_Hat, 0, CD_Hat_Up );
			// m_controlMapping[CID_Down].Set( CT_Hat, 0, CD_Hat_Down );
			// m_controlMapping[CID_Left].Set( CT_Hat, 0, CD_Hat_Left );
			// m_controlMapping[CID_Right].Set( CT_Hat, 0, CD_Hat_Right );
		}
	}

	if ( m_joystick && !m_controller )
	{
		const char* nameStr = SDL_JoystickName(0);
		vsString joystickName;
		if ( nameStr != nullptr )
			joystickName = nameStr;
		else
			joystickName = "GenericController";
		vsPreferences p(joystickName);

		for ( int i = 0; i < CID_MAX; i++ )
		{
			vsPreferenceObject *type, *id, *dir;

			type = p.GetPreference( vsFormatString("%sType", s_cidName[i].c_str()), m_controlMapping[i].type, 0, CT_MAX );
			id = p.GetPreference( vsFormatString("%sId", s_cidName[i].c_str()), m_controlMapping[i].id, 0, CID_MAX );
			dir = p.GetPreference( vsFormatString("%sDir", s_cidName[i].c_str()), m_controlMapping[i].dir, 0, CD_MAX );

			m_controlMapping[i].type = (ControlType)type->GetValue();
			m_controlMapping[i].id = id->GetValue();
			m_controlMapping[i].dir = (ControlDirection)dir->GetValue();
		}
		p.Save();
	}
#endif
#endif // 0

	// hardcode mapping of mouse buttons to mouse button CIDs.
	// m_controlMapping[CID_MouseLeftButton].Set( CT_MouseButton, SDL_BUTTON_LEFT );
	// m_controlMapping[CID_MouseMiddleButton].Set( CT_MouseButton, SDL_BUTTON_MIDDLE );
	// m_controlMapping[CID_MouseRightButton].Set( CT_MouseButton, SDL_BUTTON_RIGHT );
	//m_controlMapping[CID_MouseWheelUp].Set( CT_MouseButton, SDL_BUTTON_WHEELUP );
	//m_controlMapping[CID_MouseWheelDown].Set( CT_MouseButton, SDL_BUTTON_WHEELDOWN );

	// m_mappingsChanged = false;
	vsHeap::Pop();
}

void
vsInput::Save()
{
	vsFile f("user/binds.txt", vsFile::MODE_Write);

	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		vsInputAxis &axis = m_axis[i];

		if ( axis.isCalculated )
			continue; // don't bother saving out calculated fields.
		if ( axis.name.empty() )
			continue; // don't bother saving out fields which don't have names.

		vsRecord a;
		a.SetLabel( "Axis" );

		vsRecord *name = new vsRecord;
		name->SetLabel( "name" );
		name->SetTokenCount(1);
		name->GetToken(0).SetString( axis.name );
		a.AddChild( name );

		for ( int j = 0; j < axis.positive.ItemCount(); j++ )
		{
			DeviceControl dc = axis.positive[j];

			vsRecord *dcr = new vsRecord;
			dcr->SetLabel( "DeviceControl" );

			dcr->SetTokenCount(3);
			dcr->GetToken(0).SetString( c_controlTypeString[ dc.type ] );
			dcr->GetToken(1).SetString( c_controlDirectionString[ dc.dir ] );
			dcr->GetToken(2).SetInteger( dc.id );

			a.AddChild(dcr);
		}
		f.Record(&a);
	}

// #if !TARGET_OS_IPHONE
// 	if ( m_joystick && m_mappingsChanged )
// 	{
// 		const char* nameStr = SDL_JoystickName(0);
// 		vsString joystickName;
// 		if ( nameStr != nullptr )
// 			joystickName = nameStr;
// 		else
// 			joystickName = "GenericController";
// 		vsPreferences p(joystickName);
//
// 		for ( int i = 0; i < CID_MAX; i++ )
// 		{
// 			vsPreferenceObject *type = p.GetPreference( vsFormatString("%sType", s_cidName[i].c_str()), m_controlMapping[i].type, 0, CT_MAX );
// 			vsPreferenceObject *id = p.GetPreference( vsFormatString("%sId", s_cidName[i].c_str()), m_controlMapping[i].cid, 0, CID_MAX );
// 			vsPreferenceObject *dir = p.GetPreference( vsFormatString("%sDir", s_cidName[i].c_str()), m_controlMapping[i].dir, 0, CD_MAX );
//
// 			type->SetValue( m_controlMapping[i].type );
// 			id->SetValue( m_controlMapping[i].cid );
// 			dir->SetValue( m_controlMapping[i].dir );
// 		}
//
// 		p.Save();
// 	}
// #endif
}

void
vsInput::SetStringModeCursorHandler( vsInput::CursorHandler *handler )
{
	m_cursorHandler = handler;
}

void
vsInput::SetDropHandler( DropHandler *handler )
{
	m_dropHandler = handler;
}

void
vsInput::SetStringMode(bool mode, const vsBox2D& box, ValidationType vt, bool multiline)
{
	m_cursorHandler = nullptr;
	SetStringMode(mode, -1, box, vt, multiline);
}

void
vsInput::SetStringMode(bool mode, int maxLength, const vsBox2D& box, ValidationType vt, bool multiline)
{
	m_cursorHandler = nullptr;
	if ( mode != m_stringMode )
	{
		if ( mode )
		{
			SDL_StartTextInput();
			m_stringMode = true;
			m_stringModeIsMultiline = multiline;
			m_stringModePaused = false;
			m_stringModeString.clear();
			m_stringModeMaxLength = maxLength;
			m_stringValidationType = vt;
			SetStringModeSelectAll(false);

			SDL_Rect rect;
			rect.x = box.GetMin().x;
			rect.y = box.GetMin().y;
			rect.w = box.Width();
			rect.h = box.Height();

			SDL_SetTextInputRect(&rect);

			// bool hasScreenInput = SDL_HasScreenKeyboardSupport();
			// vsLog("Has screen input: %s", hasScreenInput ? "true" : "false");
		}
		else
		{
			SDL_StopTextInput();
			m_stringMode = false;
			m_stringModePaused = false;
			m_stringModeClearing = true;
			m_stringModeUndoStack.Clear();
		}
	}
}

void
vsInput::PauseStringMode(bool pause)
{
	m_stringModePaused = pause;
}

vsString
vsInput::GetStringModeSelection()
{
	vsString result;
	try
	{
		utf8::iterator<std::string::iterator> str( m_stringModeString.begin(), m_stringModeString.begin(), m_stringModeString.end() );
		int length = utf8::distance(m_stringModeString.begin(), m_stringModeString.end());
		for ( int i = 0; i < length; i++ )
		{
			if ( i >= m_stringModeCursorFirst.byteOffset && i < m_stringModeCursorLast.byteOffset )
				utf8::append( *str, back_inserter(result) );
			str++;
		}
	}
	catch(...)
	{
		vsLog("Failed to get string mode selection");
	}
	return result;
}

void
vsInput::SetStringModeCursor( const CursorPos& anchor, Opt options )
{
	SetStringModeCursor(anchor, anchor, options );
}

void
vsInput::SetStringModeCursor( const CursorPos& anchor_in, const CursorPos& floating_in, Opt options )
{
	CursorPos anchor(anchor_in);
	CursorPos floating(floating_in);
	try
	{
		m_undoMode = false; // if we're moving the cursor around, we're not in undo mode any more!
		if ( options == Opt_EndEdit )
		{
			if ( m_stringModeEditing )
			{
				StringModeSaveUndoState();
				m_stringModeEditing = false;
			}
			else if ( anchor != floating && m_stringModeCursorFirst == m_stringModeCursorLast )
			{
				// if we're makign a wide selection and previously we had an I-beam-style cursor,
				// save the I-beam-style position.
				StringModeSaveUndoState();
			}
		}

		// first, clamp these positions into legal positions between codepoints
		int inLength = m_stringModeString.size();
		anchor.byteOffset = vsClamp( anchor.byteOffset, 0, inLength );
		floating.byteOffset = vsClamp( floating.byteOffset, 0, inLength );

		m_stringModeCursorFirst = vsMin(anchor, floating);
		m_stringModeCursorLast = vsMax(anchor, floating);
		m_stringModeCursorAnchor = anchor;
		m_stringModeCursorFloating = floating;
	}
	catch(...)
	{
		vsLog("Failed to set string mode cursor position!");
	}
}

const vsInput::CursorPos&
vsInput::GetStringModeCursorFirst() const
{
	return m_stringModeCursorFirst;
}

const vsInput::CursorPos&
vsInput::GetStringModeCursorLast() const
{
	return m_stringModeCursorLast;
}

const vsInput::CursorPos&
vsInput::GetStringModeCursorAnchor() const
{
	return m_stringModeCursorAnchor;
}

const vsInput::CursorPos&
vsInput::GetStringModeCursorFloating() const
{
	return m_stringModeCursorFloating;
}

bool
vsInput::IsStringModeGlyphSelected(int glyphId)
{
	// blah.  I'm not convinced this will work if our font rendering
	// chooses to use a ligature.

	uni::breaks::grapheme::utf8 begin{m_stringModeString.cbegin(), m_stringModeString.cend()};
	uni::breaks::grapheme::utf8 end{m_stringModeString.cend(), m_stringModeString.cend()};

	bool selected = false;
	int glyph = 0;
	for ( auto it = begin; it != end; ++it )
	{
		if ( glyph == glyphId )
		{
			int byteOffset = (it-begin);
			if ( byteOffset >= m_stringModeCursorFirst.byteOffset &&
					byteOffset < m_stringModeCursorLast.byteOffset )
				return true;
			return false;
		}
		glyph++;
	}

	return selected;
}

void
vsInput::SetStringModeSelectAll( bool selectAll )
{
	try
	{
		int lastByteOffset = m_stringModeString.size();

		if ( selectAll )
		{
			SetStringModeCursor( CursorPos::Byte(0), CursorPos::Byte(lastByteOffset+1), Opt_EndEdit);
		}
		else
		{
			SetStringModeCursor( CursorPos::Byte(lastByteOffset+1), Opt_EndEdit );
		}
	}
	catch(...)
	{
		vsLog("SetStringModeSelectAll failed");
	}
}

bool
vsInput::GetStringModeSelectAll()
{
	try
	{
		int lastByteOffset = m_stringModeString.size();

		return ( m_stringModeCursorFirst.byteOffset == 0 &&
				m_stringModeCursorLast.byteOffset == lastByteOffset+1 );
	}
	catch(...)
	{
		vsLog("GetStringModeSelectAll failed");
	}
	return false;
}

vsVector2D
vsInput::Correct2DInputForOrientation( const vsVector2D &input )
{
	vsVector2D result = input;

	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Normal:
			break;
		case Orientation_Three:
			result.x = input.y;
			result.y = -input.x;
			break;
		case Orientation_Six:
			result.x = -input.x;
			result.y = -input.y;
			break;
		case Orientation_Nine:
			result.x = -input.y;
			result.y = input.x;
			break;
	}

	return result;
}


void
vsInput::Update(float timeStep)
{
	UNUSED(timeStep);
	PROFILE("vsInput::Update");

	m_timeSinceAnyInput += timeStep;

	// clear 'was pressed' and 'was released' flags
	//
	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		vsInputAxis& axis = m_axis[i];
		axis.wasPressed = false;
		axis.wasReleased = false;
	}

	m_mouseMotion = vsVector2D::Zero;
	m_wheelValue = 0.f;
	m_hadFocus = m_hasFocus;

#if TARGET_OS_IPHONE

	m_controlState[CID_MouseLeftButton] = ::GetTouch(0);
	m_controlState[CID_Touch0] = ::GetTouch(0);
	m_controlState[CID_Touch1] = ::GetTouch(1);

	m_mousePos = vsVector2D(GetTouchX(0),GetTouchY(0));

	m_mousePos.x /= (.5f * vsScreen::Instance()->GetTrueWidth());
	m_mousePos.y /= (.5f * vsScreen::Instance()->GetTrueHeight());
	m_mousePos -= vsVector2D(1.f,1.f);

	m_mousePos = Correct2DInputForOrientation( m_mousePos );
#else
	SDL_Event event;

	{
		//		int i, j;
		while( SDL_PollEvent( &event ) ){
			switch( event.type ){
				case SDL_APP_DIDENTERBACKGROUND:
					{
						vsSystem::Instance()->SetAppHasFocus(false);
					}
					break;
				case SDL_APP_DIDENTERFOREGROUND:
					{
						vsSystem::Instance()->SetAppHasFocus(true);
					}
					break;
				case SDL_CONTROLLERDEVICEADDED:
					{
						// WARNING:  'which' tells us the device_index of the
						// controller which has been added in the 'Added' event,
						// but tells us the instance_id in the 'Removed' and 'Remapped'
						// events.  Don't get confused about that!
						//
						// The device_index is the index we can use in the
						// "Open Game Controller" function.  The instance_id is..
						// well.. not.  Instance_id always increases as controllers
						// connect and reconnect, whereas device_index can reuse
						// indices if earlier devices disconnect first.
						vsLog("Controller connected: %d", event.cdevice.which);
						InitController(event.cdevice.which);
						break;
					}
				case SDL_CONTROLLERDEVICEREMOVED:
					{
						// WARNING:  'which' tells us the device_index of the
						// controller which has been added in the 'Added' event,
						// but tells us the instance_id in the 'Removed' and 'Remapped'
						// events.  Don't get confused about that!
						//
						// Best thing I can find to do with the instance_id is to
						// convert it into an SDL_GameController object, and use
						// that to figure out which vsController is associated
						// with it!  Then go destroy that vsController.

						vsLog("Controller instance removed: %d", event.cdevice.which);
						SDL_GameController *gc = SDL_GameControllerFromInstanceID( event.cdevice.which );
						if ( gc )
							DestroyController(gc);
						break;
					}
				case SDL_CONTROLLERDEVICEREMAPPED:
					{
						// WARNING:  'which' tells us the device_index of the
						// controller which has been added in the 'Added' event,
						// but tells us the instance_id in the 'Removed' and 'Remapped'
						// events.  Don't get confused about that!
						//
						// I can't find any documentation about what one should do
						// when this event comes in, so for the moment I'm just
						// going to assume that this is informational, and ignore
						// it.  (In practice, I probably should be querying the
						// controller again to verify that it still has the same
						// number of buttons as before, etc.  But until I can find
						// a situation where this can happen in practice, I'm
						// not going to spend much time guessing how I'm supposed
						// to handle this event type.)
						vsLog("Controller remap event received, ignored: %d", event.cdevice.which);
						break;
					}
				case SDL_CONTROLLERAXISMOTION:
					{
						// vsLog("Controller joystickId %d, axis %d: %d",
						// 		event.caxis.which,
						// 		event.caxis.axis,
						// 		event.caxis.value);
						break;
					}
				case SDL_CONTROLLERBUTTONDOWN:
					{
						// vsLog("Controller joystickId %d, button %d: %d",
						// 		event.cbutton.which,
						// 		event.cbutton.button,
						// 		event.cbutton.state);
						break;
					}
				case SDL_TEXTINPUT:
					{
						m_timeSinceAnyInput = 0.f;
						if ( !m_stringModePaused )
							HandleTextInput(event.text.text);
						break;
					}
					break;
				case SDL_TEXTEDITING:
					m_timeSinceAnyInput = 0.f;

					// vsLog("TextEditing: '%s', pos %d len %d",
					// 		event.edit.text,
					// 		event.edit.start,
					// 		event.edit.length );

					// This event is for partial, in-progress code points
					// which haven't yet settled on a final codepoint.  For now,
					// let's just ignore them.
					//
					//m_stringModeString = event.text.text;
					//m_stringModeCursorPosition = event.edit.start;
					//m_stringModeSelectionLength = event.edit.length;
					break;
				case SDL_FINGERDOWN:
					m_fingersDown++;
					break;
				case SDL_FINGERUP:
					m_fingersDown--;
					break;
				case SDL_FINGERMOTION:
					break;
				case SDL_MOUSEWHEEL:
					{
						m_timeSinceAnyInput = 0.f;
						// TODO:  FIX!
						float wheelAmt = (float)event.wheel.y;
						if ( m_fingersDownTimer > 0.f )
						{
							// probably trackpad scrolling.
							wheelAmt *= vsSystem::Instance()->GetPreferences()->GetTrackpadWheelScaling();
						}
						else
						{
							wheelAmt *= vsSystem::Instance()->GetPreferences()->GetMouseWheelScaling();
						}
						m_wheelValue = wheelAmt;
						break;
					}
				case SDL_MOUSEMOTION:
					{
						m_timeSinceAnyInput = 0.f;
						if ( event.motion.which == SDL_TOUCH_MOUSEID ) // touch event;  ignore!
							break;
						m_mousePos = vsVector2D((float)event.motion.x,(float)event.motion.y);

						m_mousePos.x /= (.5f * vsScreen::Instance()->GetTrueWidth());
						m_mousePos.y /= (.5f * vsScreen::Instance()->GetTrueHeight());
						m_mousePos -= vsVector2D(1.f,1.f);

						if ( m_suppressFirstMotion )
						{
							m_suppressFirstMotion = false;
						}
						else
						{
							vsVector2D motion = vsVector2D((float)event.motion.xrel,(float)event.motion.yrel);
							motion.x /= (.5f * vsScreen::Instance()->GetTrueWidth());
							motion.y /= (.5f * vsScreen::Instance()->GetTrueHeight());

							m_mouseMotion += motion;

							// TODO:  CORRECT FOR ORIENTATION ON IOS DEVICES
						}
						break;
					}
				case SDL_KEYDOWN:
					{
						m_timeSinceAnyInput = 0.f;
						if ( !m_stringModeClearing )
						{
							if ( m_stringMode )
							{
								if ( !m_stringModePaused )
									HandleStringModeKeyDown(event);
							}
							else
								HandleKeyDown(event);
						}
						break;
					}
				case SDL_KEYUP:
					{
						m_timeSinceAnyInput = 0.f;
						HandleKeyUp(event);
						break;
					}
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					{
						m_timeSinceAnyInput = 0.f;
						HandleMouseButtonEvent(event);
						break;
					}
				case SDL_DROPBEGIN:
					{
						vsLog("drop begins");
						if ( m_dropHandler )
						{
							m_dropHandler->BeginDrop();
						}
						break;
					}
				case SDL_DROPCOMPLETE:
					{
						vsLog("Filedrop complete");
						if ( m_dropHandler )
						{
							m_dropHandler->EndDrop();
						}
						break;
					}
				case SDL_DROPTEXT:
					{
						vsLog("Text dropped on window: %s", event.drop.file);
						if ( m_dropHandler )
						{
							m_dropHandler->Text(event.drop.file);
						}
						else
						{
							vsLog("No file drop handler installed");
						}
						break;
					}
				case SDL_DROPFILE:
					{
						vsLog("File dropped on window: %s", event.drop.file);

						if ( m_dropHandler )
						{
							m_dropHandler->File( event.drop.file );
						}
						else
						{
							vsLog("No file drop handler installed");
						}

						SDL_free(event.drop.file); // bah, SDL shouldn't require this.
						break;
					}
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						// we only have one window, so a close event means quit!
						case SDL_WINDOWEVENT_CLOSE:
							core::SetExit();
							break;
						case SDL_WINDOWEVENT_EXPOSED:
							// vsLog("Exposed");
							break;
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							{
								// in ppractice, we seem to be receiving this event
								// when alt-tabbing into a fullscreen window.  In
								// theory, this happens after the window size changes
								// for any reason.  If that reason was due to the OS,
								// then we'll get a SDL_WINDOWEVENT_SIZE_CHANGED
								// event right afterward.  In the alt-tabbing case,
								// though, we don't seem to receive the
								// SDL_WINDOWEVENT_RESIZED event.
								vsLog("SizeChanged EVENT:  %d, %d", event.window.data1, event.window.data2);
								vsSystem::Instance()->CheckVideoMode();
								break;
							}
						case SDL_WINDOWEVENT_RESIZED:
							{
								if ( m_suppressResizeEvent )
									m_suppressResizeEvent = false;
								else
								{
									vsLog("RESIZE EVENT:  %d, %d", event.window.data1, event.window.data2);
									// vsSystem::Instance()->CheckVideoMode();
									int windowWidth = event.window.data1;
									int windowHeight = event.window.data2;
									vsSystem::Instance()->NotifyResized(windowWidth, windowHeight);
								}
								break;
							}
						case SDL_WINDOWEVENT_SHOWN:
							// vsLog("Shown");
							vsSystem::Instance()->SetAppIsVisible( true );
							break;
						case SDL_WINDOWEVENT_HIDDEN:
							// vsLog("Hidden");
							vsSystem::Instance()->SetAppIsVisible( false );
							break;
						case SDL_WINDOWEVENT_MOVED:
							// check video mode, in case we've been moved to a
							// display with a different DPI.
							vsSystem::Instance()->CheckVideoMode();
							break;
						case SDL_WINDOWEVENT_FOCUS_LOST:
							vsSystem::Instance()->SetAppHasFocus( false );
							m_hasFocus = false;
							break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							vsSystem::Instance()->SetAppHasFocus( true );
							m_hasFocus = true;
							break;
						case SDL_WINDOWEVENT_ENTER:
							m_mouseIsInWindow = true;
							break;
						case SDL_WINDOWEVENT_LEAVE:
							m_mouseIsInWindow = false;
							break;
						case SDL_WINDOWEVENT_MINIMIZED:
							vsLog("Minimized");
							break;
						case SDL_WINDOWEVENT_MAXIMIZED:
							vsLog("Maximized");
							break;
						case SDL_WINDOWEVENT_RESTORED:
							vsLog("Restored");
							break;
							/*
						case SDL_WINDOWEVENT_TAKE_FOCUS:
							break;
						case SDL_WINDOWEVENT_HIT_TEST:
							break;
							*/
#if SDL_VERSION_ATLEAST( 2, 0, 5 ) // new window events added in SDL 2.0.5
						case SDL_WINDOWEVENT_TAKE_FOCUS:
							// On Linux, Window Manager is asking us whether
							// we want to take focus.  We don't;  the window
							// manager will give focus to us if it wants to;
							// we don't need to demand it.
							//
							// Also, in all my tests so far, we don't receive
							// this event until after the window manager has
							// already given us focus anyhow.
							break;
						case SDL_WINDOWEVENT_HIT_TEST:
							// window had a hit test that wasn't
							// SDL_HITTEST_NORMAL.  That is, we've swallowed
							// a mouse event because it didn't hit the window
							// content, but instead hit a window resize control
							// or something in the title bar.  Not sure why
							// we care about that, but this fix came from
							// the nice Unreal folks, so they presumably had an
							// issue around this somewhere.
							break;
#endif
#if SDL_VERSION_ATLEAST( 2, 0, 18 ) // new window events added in SDL 2.0.18
						case SDL_WINDOWEVENT_ICCPROF_CHANGED:
							// vsLog("Window ICC profile changed!  (do we care?  no we do not, right now.)");
							break;
						case SDL_WINDOWEVENT_DISPLAY_CHANGED:
							// vsLog("Window has changed which display it's centered on.  (do we care?  no we do not.)");
							break;
#endif
						default:
							vsLog("Unhandled window event:  %d", event.window.event);
							break;
					}
					break;
				case SDL_QUIT:
					core::SetExit();
					break;
			}
		}
	}

#if 0
	if ( m_controller )
	{
		SDL_JoystickUpdate();

		if ( m_preparingToPoll )
		{
			// wait for everything to be released.
			bool anythingDown = false;

			for ( int i = 0; i < m_joystickButtons; i++ )
			{
				if ( ReadButton(i) != 0.f )
					anythingDown = true;
			}
			for ( int i = 0; i < m_joystickAxes; i++ )
			{
				if ( ReadAxis(i) != 0.f )
					anythingDown = true;
			}

			if ( !anythingDown )
			{
				m_preparingToPoll = false;
				m_pollingForDeviceControl = true;
			}
		}
		else if ( m_pollingForDeviceControl )
		{
			bool found = false;

			for ( int i = 0; i < m_joystickHats; i++ )
			{
				for ( int j = CD_Hat_Up; j <= CD_Hat_Left; j++ )
				{
					ControlDirection cd = (ControlDirection)j;
					if ( ReadHat(i,cd) != 0.f )
					{
						found = true;
						m_pollResult.Set( CT_Hat, i, cd );
					}
				}
			}
			for ( int i = 0; i < m_joystickButtons; i++ )
			{
				if ( ReadButton(i) != 0.f )
				{
					found = true;
					m_pollResult.Set( CT_Button, i );
				}
			}
			for ( int i = 0; i < m_joystickAxes; i++ )
			{
				float axisValue = ReadAxis(i);
				if ( axisValue != 0.f )
				{
					found = true;
					m_pollResult.Set( CT_Axis, i, (axisValue > 0.f)? CD_Positive : CD_Negative );
				}
			}

			if ( found )
			{
				m_pollingForDeviceControl = false;

				Update(0.f);	// a couple updates to make sure we don't register a 'press' for the button we've pressed, if any.
				Update(0.f);
			}
		}
		else	// normal operation
		{
			for ( int i = 0; i < CID_MAX; i++ )
			{
				DeviceControl *dc = &m_controlMapping[i];

				if ( dc->type == CT_Axis )
				{
					ReadAxis( dc->id, dc->dir, i );
				}
				else if ( dc->type == CT_Button )
				{
					ReadButton( dc->id, i );
				}
				else if ( dc->type == CT_Hat )
				{
					ReadHat( dc->id, dc->dir, i );
				}
				else if ( dc->type == CT_MouseButton )
				{
					ReadMouseButton( dc->id, i );	// now read in the event loop, as mouse clicks are momentary.
				}
				else
					m_controlState[i] = 0.f;
			}
		}
	}
	else
#endif //0
	// {
	// 	for ( int i = 0; i < CID_MAX; i++ )
	// 	{
	// 		DeviceControl *dc = &m_controlMapping[i];
    //
	// 		if ( dc->type == CT_MouseButton )
	// 		{
	// 			m_controlState[i] = ReadMouseButton( dc->id );
	// 		}
	// 		else
	// 		{
	// 			m_controlState[i] = 0.f;
	// 		}
	// 	}
	// }

	// for ( int i = 0; i < CID_MAX; i++ )
	// {
	// 	if ( m_keyControlState[i] > m_controlState[i] )
	// 		m_controlState[i] = m_keyControlState[i];
	// }
    //
	// m_controlState[CID_LeftRightLAxis] = m_controlState[CID_LRight] - m_controlState[CID_LLeft];
	// m_controlState[CID_UpDownLAxis] = m_controlState[CID_LDown] - m_controlState[CID_LUp];
    //
	// m_controlState[CID_LeftRightRAxis] = m_controlState[CID_RRight] - m_controlState[CID_RLeft];
	// m_controlState[CID_UpDownRAxis] = m_controlState[CID_RDown] - m_controlState[CID_RUp];
    //
	// m_controlState[CID_MouseWheel] = m_controlState[CID_MouseWheelDown] - m_controlState[CID_MouseWheelUp];
    //
	// if ( m_fingersDown >= 2 )
	// {
	// 	m_fingersDownTimer = 1.f;
	// }
	// else
	// {
	// 	m_fingersDownTimer = vsMax(0.f,m_fingersDownTimer - timeStep);
	// }
	// if ( m_wheelSmoothing && m_fingersDownTimer == 0.f )
	// {
	// 	// on OSX, you can often get absurd instantaneous mouse movements;  reporting
	// 	// up to 20 or so wheel 'click's in a single frame.  This is probably in part
	// 	// because of SDL's attempt to convert OSX's clickless scrolling into the
	// 	// more usual integral 'clicks' used on other OSes.  Let's clamp it to +-2.
	// 	float current = vsClamp(-2.f, m_controlState[CID_MouseWheel], 2.f);
    //
	// 	const float c_stiffness = 23.f;
	// 	const float c_damping = 2.f * vsSqrt(c_stiffness);
	// 	float delta = (current - m_wheelSpeed);
	// 	m_wheelSpeed += delta * c_stiffness * timeStep;
	// 	m_wheelSpeed -= delta * c_damping * timeStep;
    //
	// 	m_controlState[CID_MouseWheelUp] = m_wheelSpeed > 0.f ? m_wheelSpeed : 0.f;
	// 	m_controlState[CID_MouseWheelDown] = m_wheelSpeed < 0.f ? -m_wheelSpeed : 0.f;
	// 	m_controlState[CID_MouseWheel] = m_wheelSpeed;
	// }
	// else
	// {
	// 	m_wheelSpeed = 0.f;
	// }
#endif
	m_suppressResizeEvent = false;

	if ( m_stringModeClearing )
	{
		bool anythingDown = AnythingIsDown();
		if ( !anythingDown )
			m_stringModeClearing = false;
	}
	if ( m_preparingToPoll )
	{
		// wait for everything to be released.
		bool anythingDown = AnythingIsDown();

		if ( !anythingDown )
		{
			m_preparingToPoll = false;
			m_pollingForDeviceControl = true;
		}
	}
	else if ( m_pollingForDeviceControl )
	{
		bool found = false;

		int keyCount;
		const Uint8* keys = SDL_GetKeyboardState(&keyCount);

		// TODO:  It'd probably be *heaps* faster to check this four or eight
		// bytes at a time, instead of key-by-key.  But then you have to cope
		// with the end of the array possibly not filling all those bytes up.
		// And honestly, we're virtually never going through this code at all,
		// so I'm being silly even considering trying to optimise this test for
		// any keys being held down.  I'm sorry.
		for ( int i = 0; i < keyCount; i++ )
		{
			if ( keys[i] != 0 )
			{
				m_pollResult.type = CT_Keyboard;
				m_pollResult.id = i;
				m_pollResult.dir = CD_Positive;
				found = true;
				break;
			}
		}

		// Now let's also check for mouse wheel
		if ( !found )
		{
			if ( vsFabs(m_wheelValue) >= 1.f )
			{
				m_pollResult.type = CT_MouseWheel;
				m_pollResult.id = 0;
				m_pollResult.dir = ( m_wheelValue > 0.f ) ? CD_Positive : CD_Negative;
				found = true;
			}

		}

		for ( int i = 0; i < m_axis.ItemCount(); i++ )
		{
			m_axis[i].lastValue = m_axis[i].currentValue = 0.f;
		}

		if ( found )
		{
			m_pollingForDeviceControl = false;

			Update(0.f);	// a couple updates to make sure we don't register a 'press' for the button we've pressed, if any.
			Update(0.f);
		}
	}
	else	// normal operation
	{

		for ( int i = 0; i < m_axis.ItemCount(); i++ )
		{
			if ( !m_axis[i].isCalculated )
				m_axis[i].Update(m_hasFocus, m_hadFocus);
		}

		for ( int i = 0; i < m_axis.ItemCount(); i++ )
		{
			if ( m_axis[i].isCalculated )
				m_axis[i].Update(m_hasFocus, m_hadFocus);
		}
	}

	// set 'pressed' and 'released' flags generically
	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		vsInputAxis& axis = m_axis[i];
		if ( IsDown(i) && !WasDown(i) )
			axis.wasPressed = true;
		if ( WasDown(i) && !IsDown(i) )
			axis.wasReleased = true;
	}
}

float
vsInput::ReadMouseButton( int buttonID )
{
#if !TARGET_OS_IPHONE
	int buttons = SDL_GetMouseState(nullptr,nullptr);
	bool buttonDown = !!(buttons & SDL_BUTTON(buttonID));

#if defined(__APPLE_CC__)
	// Apple-style Command+LeftMouseButton == RightMouseButton.
	if ( !buttonDown && buttonID == SDL_BUTTON_RIGHT )
	{
		// test alt+left_button, for trackpad users.
		// SDL1 did this automatically, apparently.

		SDL_Keymod keymod = SDL_GetModState();
		if ( keymod & KMOD_LGUI )
			buttonDown = !!(buttons & SDL_BUTTON(SDL_BUTTON_LEFT));
	}
	if ( buttonID == SDL_BUTTON_LEFT )
	{
		// turn off left button if alt is down;  we're treating
		// that combo as the right mouse button.

		SDL_Keymod keymod = SDL_GetModState();
		if ( keymod & KMOD_LGUI )
			buttonDown = false;
	}
#endif // __APPLE_CC__

	return buttonDown?1.0f:0.0f;
#else
	return 0.0f;
#endif
}

vsController *
vsInput::GetController()
{
	for ( int i = 0; i < MAX_JOYSTICKS; i++ )
		if ( m_controller[i] )
			return m_controller[i];
	return nullptr;
}

float
vsController::ReadAxis_Raw( int axisID )
{
#if !TARGET_OS_IPHONE && defined(VS_GAMEPADS)
	float axisValue;
	if ( m_controller )
		axisValue = SDL_GameControllerGetAxis(m_controller, (SDL_GameControllerAxis)axisID) / 32767.0f;
	else
		axisValue = SDL_JoystickGetAxis(m_joystick, axisID) / 32767.0f;
#else
	float axisValue = 0.0f;
#endif
	return axisValue;
}

float
vsController::ReadAxis( int axisID )
{
	const float c_deadZone = 0.2f;
	const float c_oneMinusDeadZone = 1.0f - c_deadZone;

	float axisValue = ReadAxis_Raw( axisID );
	axisValue -= m_axisCenter[axisID];

	if ( axisValue > m_axisThrow[axisID] )
		m_axisThrow[axisID] = axisValue;

	axisValue /= m_axisThrow[axisID];

	if ( vsFabs(axisValue) < c_deadZone )
		axisValue = 0.f;
	else
	{
		if ( axisValue > 0.f )
			axisValue -= c_deadZone;
		else
			axisValue += c_deadZone;
		axisValue *= (1.0f / c_oneMinusDeadZone);
	}

	return axisValue;
}

float
vsController::ReadAxis( int axisID, ControlDirection dir )
{
	float axisValue = ReadAxis( axisID );

	if ( dir == CD_Negative )
		axisValue *= -1.0f;

	if ( axisValue < 0.f )
		axisValue = 0.f;

	return axisValue;
}

float
vsController::ReadHat(int hatID, ControlDirection dir)
{
#if !TARGET_OS_IPHONE
	float result = 0.f;
	uint8_t sdlDir = SDL_JoystickGetHat(m_joystick, hatID);

	if ( sdlDir != SDL_HAT_CENTERED )
	{
		switch( dir )
		{
			case CD_Hat_Left:
				if ( sdlDir & SDL_HAT_LEFT )
					result = 1.0f;
				break;
			case CD_Hat_Up:
				if ( sdlDir & SDL_HAT_UP )
					result = 1.0f;
				break;
			case CD_Hat_Right:
				if ( sdlDir & SDL_HAT_RIGHT )
					result = 1.0f;
				break;
			case CD_Hat_Down:
				if ( sdlDir & SDL_HAT_DOWN )
					result = 1.0f;
				break;
			default:
				vsAssert(0, "Invalid hat direction requested!");
				break;
		}
	}
#else
	float result = 0.0f;
#endif
	return result;
}

float
vsController::ReadButton( int buttonID )
{
	float result = 0.f;
#if !TARGET_OS_IPHONE && defined(VS_GAMEPADS)
	bool buttonDown = false;
	if ( m_controller )
		buttonDown = !!SDL_GameControllerGetButton(m_controller, (SDL_GameControllerButton)buttonID);
	else
		buttonDown = !!SDL_JoystickGetButton(m_joystick, buttonID);

	if ( buttonDown )
		result = 1.0f;
#endif
	return result;
}

bool
vsInput::IsDown( int id )
{
	return (m_axis[id].currentValue > 0.f);
}

bool
vsInput::WasDown( int id )
{
	return (m_axis[id].lastValue > 0.f);
}

bool
vsInput::WasPressed( int id )
{
	return (m_axis[id].wasPressed);
}

void
vsInput::ConsumePress( int id )
{
	m_axis[id].wasPressed = false;
	m_axis[id].wasReleased = false;
}

bool
vsInput::WasReleased( int id )
{
	return (m_axis[id].wasReleased);
}

bool
vsInput::MouseIsOnScreen()
{
#if !TARGET_OS_IPHONE
	return m_mouseIsInWindow;
	//uint8_t state = SDL_GetAppState();

	//return ( state & SDL_APPMOUSEFOCUS );
#else
	return false;
#endif
}

vsVector2D
vsInput::GetWindowMousePosition()
{
	vsVector2D retval = m_mousePos;
	retval += vsVector2D::One;
	retval.x *= (.5f * vsScreen::Instance()->GetTrueWidth());
	retval.y *= (.5f * vsScreen::Instance()->GetTrueHeight());

	return retval;
}

vsVector2D
vsInput::GetMousePosition(int scene)
{
	// mousePos is going from (-1,-1 .. 1,1) as we go from top left to bottom
	// right corner of the window

	vsScene *s = vsScreen::Instance()->GetScene(scene);
	if ( s->Is3D() )
	{
		return m_mousePos;
	}

	vsCamera2D *c = s->GetCamera();

	vsTransform2D t = c->GetCameraTransform();
	vsVector2D scale = t.GetScale();
	scale.y *= 0.5f;
	scale.x = scale.y * vsScreen::Instance()->GetAspectRatio();
	t.SetScale(scale);
	vsVector2D mousePos = t.ApplyTo( m_mousePos );

	return mousePos;
}

void
vsInput::WarpMouseTo( int scene, const vsVector2D& position_in )
{
	vsScene *s = vsScreen::Instance()->GetScene(scene);
	vsVector2D screenPos;
	if ( s->Is3D() )
	{
		// convert from normalised device coordinates to something useful
		screenPos = position_in;
		screenPos.y *= -1.0f; // SDL considers its y coordinates opposite from OpenGL
		screenPos += vsVector2D(1.f,1.f);
		screenPos.x *= (.5f * vsScreen::Instance()->GetTrueWidth());
		screenPos.y *= (.5f * vsScreen::Instance()->GetTrueHeight());
	}
	else
	{
		vsCamera2D *c = s->GetCamera();

		vsTransform2D t = c->GetCameraTransform();
		vsVector2D scale = t.GetScale();
		scale.y *= 0.5f;
		scale.x = scale.y * vsScreen::Instance()->GetAspectRatio();
		t.SetScale(scale);
		screenPos = t.ApplyInverseTo( m_mousePos );
	}
	SDL_WarpMouseInWindow( g_sdlWindow, screenPos.x, screenPos.y );

	m_mousePos = screenPos;

	m_mousePos.x /= (.5f * vsScreen::Instance()->GetTrueWidth());
	m_mousePos.y /= (.5f * vsScreen::Instance()->GetTrueHeight());
	m_mousePos -= vsVector2D(1.f,1.f);

	m_mousePos = Correct2DInputForOrientation( m_mousePos );

}

vsVector2D
vsInput::GetMouseMotion(int scene)
{
	vsScene *s = vsScreen::Instance()->GetScene(scene);
	if ( s->Is3D() )
	{
		return m_mouseMotion;
	}

	vsCamera2D *c = s->GetCamera();

	vsTransform2D t = c->GetCameraTransform();
	vsVector2D scale = t.GetScale();
	scale.y *= 0.5f;
	scale.x = scale.y * vsScreen::Instance()->GetAspectRatio();
	vsVector2D mousePos = vsVector2D( m_mouseMotion.x * scale.x, m_mouseMotion.y * scale.y );

	return mousePos;
}

bool
vsInput::HasTouch(int touchID)
{
#if TARGET_OS_IPHONE
	return ::GetTouch(touchID);
#else
	return false;
#endif // TARGET_OS_IPHONE
}

int
vsInput::GetMaxTouchCount()
{
#if TARGET_OS_IPHONE
	return 10;
#else
	return 0;
#endif // TARGET_OS_IPHONE
}

vsVector2D
vsInput::GetTouchPosition(int touchID, int scene)
{
#if TARGET_OS_IPHONE

	vsScene *s = vsScreen::Instance()->GetScene(scene);
	vsVector2D touchPos( GetTouchX(touchID), GetTouchY(touchID) );

	touchPos.x /= (.5f * vsScreen::Instance()->GetTrueWidth());
	touchPos.y /= (.5f * vsScreen::Instance()->GetTrueHeight());
	touchPos -= vsVector2D(1.f,1.f);

	touchPos = Correct2DInputForOrientation( touchPos );

	if ( s->Is3D() )
	{
		return touchPos;
	}

	vsCamera2D *c = s->GetCamera();

	vsTransform2D t = c->GetCameraTransform();
	t.m_scale.y *= 0.5f;
	t.m_scale.x = t.m_scale.y * vsScreen::Instance()->GetAspectRatio();
	touchPos = t.ApplyTo( touchPos );

	return touchPos;

#else
	return vsVector2D::Zero;
#endif // TARGET_OS_IPHONE
}


void
vsInput::CaptureMouse( bool capture )
{
#if !TARGET_OS_IPHONE
	if ( m_captureMouse != capture )
	{
		m_captureMouse = capture;
		if ( capture )
		{
			SDL_GetMouseState(&m_capturedMouseX,&m_capturedMouseY);

			SDL_SetRelativeMouseMode(SDL_TRUE);
			m_mousePos = vsVector2D::Zero;
			m_mouseMotion = vsVector2D::Zero;
			m_suppressFirstMotion = true;
		}
		else
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
// #ifndef _WIN32
			// Bug in SDL2 on OSX:  Relative mouse mode moves the cursor to the
			// middle of the window, even though the function documentation says
			// that it's not supposed to do that.  Workaround:  On OSX, explicitly
			// warp the cursor back to its correct position, when we turn off
			// mouse capture.
			//
			// Similar bug on Linux:  Relative mouse mode doesn't leave the cursor
			// in the same position where it began, the way that the documentation
			// says that it's supposed to.  Same workaround:  explicitly warp
			// the mouse back to the correct position when mouse capture ends.
			//
			// ...and now the Linux behaviour is happening on Windows as well.
			// So I guess that's just expected behaviour, now.  :)  Enabling
			// mouse warping for Windows, too!
			//
			SDL_WarpMouseInWindow( g_sdlWindow, m_capturedMouseX, m_capturedMouseY );
// #endif

			m_mousePos = vsVector2D((float)m_capturedMouseX,(float)m_capturedMouseY);

			m_mousePos.x /= (.5f * vsScreen::Instance()->GetTrueWidth());
			m_mousePos.y /= (.5f * vsScreen::Instance()->GetTrueHeight());
			m_mousePos -= vsVector2D(1.f,1.f);

			m_mousePos = Correct2DInputForOrientation( m_mousePos );
			m_suppressFirstMotion = true;
		}
	}
#endif
}

void
vsInput::HandleTextInput( const vsString& _input )
{
	vsString input(_input);
	m_undoMode = false;
	m_backspaceMode = false;

	// always remove all \r;  we don't use those!
	input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());

	if ( !m_stringModeIsMultiline )
	{
		// if we're not doing multiline editing, replace all '\n' with ' '
		std::replace( input.begin(), input.end(), '\n', ' ' );
	}

	if ( !m_stringModeEditing )
	{
		StringModeSaveUndoState();
		m_stringModeEditing = true;
	}

	if ( m_cursorHandler )
	{
		TextInputResult tir = m_cursorHandler->TextInput(input);
		m_stringModeString = tir.newString;
		SetStringModeCursor( tir.newCursorPos, Opt_None );

		ValidateString();
		if ( m_cursorHandler )
			m_cursorHandler->NotifyStringChanged( m_stringModeString );
	}
	else
	{
		vsString oldString = m_stringModeString;
		m_stringModeString = vsEmptyString;

		try
		{
			// Okay, we're going to insert this new input into the utf8 string.

			if ( m_stringModeCursorFirst.byteOffset != m_stringModeCursorLast.byteOffset )
				oldString.erase( m_stringModeCursorFirst.byteOffset, m_stringModeCursorLast.byteOffset-m_stringModeCursorFirst.byteOffset );

			oldString.insert( m_stringModeCursorFirst.byteOffset, input );
			m_stringModeString = oldString;

			SetStringModeCursor( CursorPos::Byte(m_stringModeCursorFirst.byteOffset + input.size()), Opt_None );

			ValidateString();
		}
		catch (...)
		{
			vsLog("Failed to handle UTF8 text input!");
			vsLog("  String: %s", oldString);
			vsLog("  New input: %s", input);
		}
	}
}

void
vsInput::ValidateString()
{
	vsString oldString = m_stringModeString;
	m_stringModeString = vsEmptyString;

	try
	{
		utf8::iterator<std::string::iterator> it( oldString.begin(), oldString.begin(), oldString.end() );

		int length = utf8::distance(oldString.begin(), oldString.end());

		bool hasDecimalSeparator = false;
		int codepointsSoFar = 0;

		extern vsString s_decimalSeparator;

		for ( int i = 0; i < length; i++ )
		{
			bool valid = true;

			if ( m_stringValidationType == Validation_PositiveInteger )
			{
				vsString validString = "0123456789";
				valid = false;
				utf8::iterator<std::string::iterator> vit( validString.begin(), validString.begin(), validString.end() );
				for ( int l = 0; l < utf8::distance(validString.begin(), validString.end()); l++ )
				{
					if ( *it == *(vit++) )
						valid = true;
				}
			}
			else if ( m_stringValidationType == Validation_PositiveIntegerPercent )
			{
				vsString validString = "0123456789";
				valid = false;
				utf8::iterator<std::string::iterator> vit( validString.begin(), validString.begin(), validString.end() );
				for ( int l = 0; l < utf8::distance(validString.begin(), validString.end()); l++ )
				{
					if ( *it == *(vit++) )
						valid = true;
				}
			}
			else if ( m_stringValidationType == Validation_Integer )
			{
				vsString validString = "0123456789";
				valid = false;
				utf8::iterator<std::string::iterator> vit( validString.begin(), validString.begin(), validString.end() );
				for ( int l = 0; l < utf8::distance(validString.begin(), validString.end()); l++ )
				{
					if ( *it == *(vit++) )
						valid = true;
				}
				if ( *it == '-' && i == 0 )
					valid = true;
			}
			else if ( m_stringValidationType == Validation_Numeric )
			{
				vsString validString = "0123456789";
				// we support only [0-9].
				//
				// We also support up to one '.', and we may have a '-' on the front.

				if ( *it == '-' && i == 0 )
					valid = true;
				else if ( *it == (uint8_t)(s_decimalSeparator[0]) )
				{
					if ( hasDecimalSeparator )
						valid = false;
					else
						hasDecimalSeparator = true;
				}
				else
				{
					valid = false;
					utf8::iterator<std::string::iterator> vit( validString.begin(), validString.begin(), validString.end() );
					for ( int l = 0; l < utf8::distance(validString.begin(), validString.end()); l++ )
					{
						if ( *it == *(vit++) )
							valid = true;
					}
				}
			}
			else if ( m_stringValidationType == Validation_PositiveNumeric )
			{
				vsString validString = "0123456789";
				// we support only [0-9].
				//
				// We also support up to one '.', and we may have a '-' on the front.

				if ( *it == (uint8_t)(s_decimalSeparator[0]) )
				{
					if ( hasDecimalSeparator )
						valid = false;
					else
						hasDecimalSeparator = true;
				}
				else
				{
					valid = false;
					utf8::iterator<std::string::iterator> vit( validString.begin(), validString.begin(), validString.end() );
					for ( int l = 0; l < utf8::distance(validString.begin(), validString.end()); l++ )
					{
						if ( *it == *(vit++) )
							valid = true;
					}
				}
			}
			else if ( m_stringValidationType == Validation_Filename )
			{
				vsString invalidString = "|!@#$%^&*()_{}][/\\.,';\":>?<";
				utf8::iterator<std::string::iterator> vit( invalidString.begin(), invalidString.begin(), invalidString.end() );
				for ( int l = 0; l < utf8::distance(invalidString.begin(), invalidString.end()); l++ )
				{
					if ( *it == *(vit++) )
						valid = false;
				}
			}

			if ( valid && (m_stringModeMaxLength < 0 || codepointsSoFar < m_stringModeMaxLength) )
			{
				codepointsSoFar++;
				utf8::append( *it, back_inserter(m_stringModeString) );
			}
			else
			{
				// This codepoint wasn't valid!  Therefore, we're removing it, and we
				// need to adjust the cursor positioning.
				utf8::iterator<std::string::iterator> n(it);
				n++;

				int bytesSoFar = it.base()-oldString.begin();
				int invalidBytes = n.base()-it.base();

				if ( m_stringModeCursorFirst.byteOffset > bytesSoFar )
					m_stringModeCursorFirst.byteOffset-=invalidBytes;
				if ( m_stringModeCursorLast.byteOffset > bytesSoFar )
					m_stringModeCursorLast.byteOffset-=invalidBytes;
			}

			it++;
		}

		int validBytes = m_stringModeString.size();
		m_stringModeCursorFirst.byteOffset = vsMin( m_stringModeCursorFirst.byteOffset, validBytes );
		m_stringModeCursorLast.byteOffset = vsMin( m_stringModeCursorLast.byteOffset, validBytes );
	}
	catch( ... )
	{
		vsLog("Failed to validate utf8 for string: %s", oldString );
	}
}

void
vsInput::SetStringModeString( const vsString &s )
{
	m_stringModeEditing = true; // 'true', since we just reset the string!  Now when we set the cursor position, we'll automatically save off an undo state.
	m_stringModeUndoStack.Clear();
	m_stringModeString = s;
	SetStringModeCursor( CursorPos::Byte(s.size()), Opt_None );
	// vsLog("Clearing undo stack");
}

void
vsInput::StringModeSaveUndoState()
{
	StringModeState *state = new StringModeState;
	state->string = m_stringModeString;
	state->anchor = m_stringModeCursorAnchor;
	state->floating = m_stringModeCursorFloating;
	m_stringModeUndoStack.AddItem(state);
	// vsLog("Saved undo state");
}

void
vsInput::StringModeCancel()
{
	if ( !m_stringModeUndoStack.IsEmpty() )
	{
		vsArrayStoreIterator<StringModeState> first = m_stringModeUndoStack.Back();
		m_stringModeString = first->string;

		m_stringModeUndoStack.Clear();
	}
	SetStringMode(false, vsBox2D());
}

bool
vsInput::StringModeUndo()
{
	if ( !m_stringModeUndoStack.IsEmpty() )
	{
		// if we're in undo mode, we're undoing again, so we need
		// to actually remove our current undo state.
		// if ( m_undoMode && m_stringModeUndoState.ItemCount() > 1 )
		// 	m_stringModeUndoStack.PopBack();

		// vsLog("Restore undo state");
		vsArrayStoreIterator<StringModeState> last = m_stringModeUndoStack.Back();
		// StringModeState &state = m_stringModeUndoStack[0];
		// SetStringModeString( last->string );
		m_stringModeString = last->string;
		m_stringModeEditing = false;
		SetStringModeCursor( last->anchor, last->floating, Opt_None );
		m_stringModeUndoStack.PopBack();
		if ( m_cursorHandler )
			m_cursorHandler->NotifyStringChanged( m_stringModeString );


		m_undoMode = true;
		return true;
	}
	vsLog("Failed to undo");
	return false;
}

void
vsInput::HandleStringModeKeyDown( const SDL_Event& event )
{
	switch( event.key.keysym.sym )
	{
		case SDLK_BACKSPACE:
			if ( m_stringMode && m_stringModeString.length() != 0 )
			{
				if ( m_stringModeCursorFirst.byteOffset != m_stringModeCursorLast.byteOffset )
				{
					// delete the stuff that's selected.  Which is equivalent
					// to inserting nothing "".
					//
					// [Note] The above 'if' test is explicitly ONLY checking
					// byte offsets, not lines.  If we've somehow got a selection
					// on a single byte offset across two lines (as theoretically
					// could happen over a linewrapped line), we DON'T want to
					// "replace" that zero-byte span with an empty string.
					HandleTextInput("");
				}
				else
				{
					if ( m_stringModeCursorFirst.byteOffset == 0 ) // no-op, deleting from 0.
						return;
					if ( !m_backspaceMode )
						StringModeSaveUndoState();

					try
					{
						// delete one character
						vsString oldString = m_stringModeString;
						m_stringModeString = vsEmptyString;
						// Okay.  copy all the codepoints up to the cursor MINUS ONE.
						// then copy the rest.
						utf8::iterator<std::string::iterator> in( oldString.begin(), oldString.begin(), oldString.end() );
						utf8::iterator<std::string::iterator> next = in;

						int inLength = utf8::distance(oldString.begin(), oldString.end());
						int trimmedBytes = 0;
						for ( int i = 0; i < inLength; i++ )
						{
							next++;

							int byteOffset = next.base() - oldString.begin();
							if ( byteOffset != m_stringModeCursorFirst.byteOffset )
								utf8::append( *in, std::back_inserter(m_stringModeString) );
							else
								trimmedBytes += next.base()-in.base();

							in = next;
						}

						m_backspaceMode = true;
						m_undoMode = false;

						SetStringModeCursor( CursorPos::LineByte(m_stringModeCursorFirst.line, m_stringModeCursorFirst.byteOffset-trimmedBytes), Opt_EndEdit );

						if ( m_cursorHandler )
							m_cursorHandler->NotifyStringChanged( m_stringModeString );
					}
					catch(...)
					{
						vsLog("Failed to handle backspace!");
					}
				}
			}
			break;
		case SDLK_DELETE:
			if ( m_stringMode && m_stringModeString.length() != 0 )
			{
				if ( m_stringModeCursorFirst.byteOffset != m_stringModeCursorLast.byteOffset )
				{
					// delete the stuff that's selected.  Which is equivalent
					// to inserting nothing "".
					HandleTextInput("");
				}
				else
				{
					// int codepointCount = utf8::distance(m_stringModeString.begin(), m_stringModeString.end());
					// Check if we're deleting forward from last codepoint
					if ( m_stringModeCursorFirst.byteOffset == (int)m_stringModeString.length() )
						return;
					if ( !m_backspaceMode )
						StringModeSaveUndoState();

					try
					{
						// delete one character
						vsString oldString = m_stringModeString;
						m_stringModeString = vsEmptyString;
						// Okay.  copy all the codepoints up to the cursor.
						// then skip one and copy the rest.
						utf8::iterator<std::string::iterator> in( oldString.begin(), oldString.begin(), oldString.end() );

						int inLength = utf8::distance(oldString.begin(), oldString.end());
						for ( int i = 0; i < inLength; i++ )
						{
							int byteOffset = in.base() - oldString.begin();
							if ( byteOffset != m_stringModeCursorFirst.byteOffset )
								utf8::append( *in, std::back_inserter(m_stringModeString) );

							in++;
						}

						m_backspaceMode = true;
						m_undoMode = false;
						// end the edit but I don't think the selection can actually
						// move due to a forward-deletion??
						SetStringModeCursor( m_stringModeCursorFirst, Opt_EndEdit );
						if ( m_cursorHandler )
							m_cursorHandler->NotifyStringChanged( m_stringModeString );
					}
					catch(...)
					{
						vsLog("Failed to handle backspace!");
					}
				}
			}
			break;
		case SDLK_UP:
			{
				CursorPos newPosition = CursorPos::LineByte(0,0);
				{
					if ( event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT) )
					{
						if ( m_cursorHandler )
							newPosition = m_cursorHandler->Up( m_stringModeCursorFloating );
						// shift is held down;  move the floating codepoint, but not the anchor codepoint!
						SetStringModeCursor( m_stringModeCursorAnchor, newPosition, Opt_EndEdit );
					}
					else
					{
						if ( m_cursorHandler )
							newPosition = m_cursorHandler->Up( m_stringModeCursorFloating );
						// shift isn't down;  move the anchor codepoint and collapse any selection
						SetStringModeCursor( newPosition, Opt_EndEdit );
					}
				}
			}
			break;
		case SDLK_DOWN:
			{
				int len = m_stringModeString.size();
				CursorPos newPosition = CursorPos::Byte(len);
				if ( event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT) )
				{
					if ( m_cursorHandler )
						newPosition = m_cursorHandler->Down( m_stringModeCursorFloating );
					// shift is held down;  move the floating codepoint, but not the anchor codepoint!
					SetStringModeCursor( m_stringModeCursorAnchor, newPosition, Opt_EndEdit );
				}
				else
				{
					if ( m_cursorHandler )
						newPosition = m_cursorHandler->Down( m_stringModeCursorFloating );
					// shift isn't down;  move the anchor codepoint and collapse any selection
					SetStringModeCursor( newPosition, Opt_EndEdit );
				}
			}
			break;
		case SDLK_HOME:
			{
				CursorPos newPosition = CursorPos::Byte(0);

				if ( event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT) )
				{
					if ( m_cursorHandler )
						newPosition = m_cursorHandler->Home( m_stringModeCursorFloating );
					SetStringModeCursor( m_stringModeCursorAnchor, newPosition, Opt_EndEdit );
				}
				else
				{
					if ( m_cursorHandler )
						newPosition = m_cursorHandler->Home( m_stringModeCursorFirst );
					SetStringModeCursor( newPosition, Opt_EndEdit );
				}
				break;
			}
		case SDLK_END:
			{
				CursorPos newPosition = CursorPos::Byte(m_stringModeString.size());
				if ( event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT) )
				{
					if ( m_cursorHandler )
						newPosition = m_cursorHandler->End( m_stringModeCursorFloating );
					SetStringModeCursor( m_stringModeCursorAnchor, newPosition, Opt_EndEdit );
				}
				else
				{
					if ( m_cursorHandler )
						newPosition = m_cursorHandler->End( m_stringModeCursorLast );
					SetStringModeCursor( newPosition, Opt_EndEdit );
				}
				break;
			}
		case SDLK_LEFT:
			{
				CursorPos nextFloatingPosition = m_stringModeCursorFloating;

				if ( m_cursorHandler )
				{
					nextFloatingPosition = m_cursorHandler->Left( nextFloatingPosition );
				}
				else
				{
					// [TODO]: uni-algo doesn't have reverse iterators yet, so we have
					// to do a full grapheme search FORWARD through the string.  This
					// is something they're interested in fixing, so this will simplify
					// a lot once that happens.
					uni::breaks::grapheme::utf8 begin{m_stringModeString.cbegin(), m_stringModeString.cend()};
					uni::breaks::grapheme::utf8 end{m_stringModeString.cend(), m_stringModeString.cend()};

					for ( auto it = begin; it != end; ++it )
					{
						int byteOffset = (it-begin);
						if ( byteOffset >= m_stringModeCursorFloating.byteOffset )
							break;
						nextFloatingPosition.byteOffset = byteOffset;
					}
				}

				// now we need to move back to the previous grapheme break

				if ( event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT) )
				{
					// shift is held down;  move the floating codepoint, but not the anchor codepoint!
					SetStringModeCursor( m_stringModeCursorAnchor, nextFloatingPosition, Opt_EndEdit );
				}
				else
				{
					// shift isn't down
					// if it's a single insertion point
					if ( m_stringModeCursorAnchor == m_stringModeCursorFloating )
					{
						//move the anchor codepoint and collapse any selection
						SetStringModeCursor( nextFloatingPosition, Opt_EndEdit );
					}
					else
					{
						//collapse selection to the leftmost codepoint in the selection
						SetStringModeCursor( m_stringModeCursorFirst, Opt_EndEdit );
					}
				}
			}
			break;
		case SDLK_RIGHT:
			{
				uni::breaks::grapheme::utf8 begin{m_stringModeString.cbegin(), m_stringModeString.cend()};
				uni::breaks::grapheme::utf8 end{m_stringModeString.cend(), m_stringModeString.cend()};

				if ( event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT) )
				{
					CursorPos nextFloatingPosition = m_stringModeCursorFloating;
					if ( m_cursorHandler )
					{
						nextFloatingPosition = m_cursorHandler->Right( nextFloatingPosition );
					}
					else
					{
						// [TODO]: uni-algo doesn't have reverse iterators yet, so we have
						// to do a full grapheme search FORWARD through the string.  This
						// is something they're interested in fixing, so this will simplify
						// a lot once that happens.

						for ( auto it = begin; it != end; ++it )
						{
							int byteOffset = (it-begin);
							if ( byteOffset >= m_stringModeCursorFloating.byteOffset )
								break;
							nextFloatingPosition.byteOffset = byteOffset;
						}
						// NOTE TO FUTURE TREVOR:  The following 'if' statement
						// looks weird but it's handling the case where we're moving
						// to the right *past* the end of the string.  We need to
						// keep it!  Best, past-Trevor, 15/7/2022
						if ( nextFloatingPosition == m_stringModeCursorFloating )
							nextFloatingPosition.byteOffset = m_stringModeString.size();
					}


					// shift is held down;  move the floating codepoint, but not the anchor codepoint!
					SetStringModeCursor( m_stringModeCursorAnchor, nextFloatingPosition, Opt_EndEdit );
				}
				else
				{
					if ( m_stringModeCursorAnchor.byteOffset == m_stringModeCursorFloating.byteOffset )
					{
						// shift isn't down;  move the anchor codepoint and collapse any selection
						CursorPos nextAnchorPosition = m_stringModeCursorAnchor;
						if ( m_cursorHandler )
						{
							nextAnchorPosition = m_cursorHandler->Right( nextAnchorPosition );
						}
						else
						{
							for ( auto it = begin; it != end; ++it )
							{
								int byteOffset = (it-begin);
								if ( byteOffset > m_stringModeCursorAnchor.byteOffset )
								{
									nextAnchorPosition.byteOffset = byteOffset;
									break;
								}
							}
							// as above, the following 'if' handles the case
							// where we've moved to the right to beyond the end
							// of the string, which can't be caught by our
							// iterator above
							if ( nextAnchorPosition == m_stringModeCursorAnchor )
								nextAnchorPosition.byteOffset = m_stringModeString.size();
						}
						SetStringModeCursor( nextAnchorPosition, Opt_EndEdit );
					}
					else
					{
						//collapse selection to the end of the selection
						SetStringModeCursor( m_stringModeCursorLast, Opt_EndEdit );
					}
				}
			}
			break;
		case SDLK_a:
			{
				// select all
				if ( event.key.keysym.mod & c_shortcutModifierKeys )
				{
					SetStringModeSelectAll(true);
				}
				break;
			}
		case SDLK_v:
			{
				// paste
				if ( event.key.keysym.mod & c_shortcutModifierKeys )
				{
					vsString clipboardText = SDL_GetClipboardText();
					if ( !clipboardText.empty() )
					{
						StringModeSaveUndoState();
						HandleTextInput( clipboardText );
					}
				}
				break;
			}
		case SDLK_c:
			{
				// copy
				if ( event.key.keysym.mod & c_shortcutModifierKeys )
				{
					// extract the currently selected text
					vsString sel = GetStringModeSelection();
					if ( !sel.empty() )
						SDL_SetClipboardText( sel.c_str() );
				}
				break;
			}
		case SDLK_x:
			{
				// cut
				if ( event.key.keysym.mod & c_shortcutModifierKeys )
				{
					// extract the currently selected text
					StringModeSaveUndoState();
					vsString sel = GetStringModeSelection();
					if ( !sel.empty() )
						SDL_SetClipboardText( sel.c_str() );
					HandleTextInput("");
				}
				break;
			}
		case SDLK_z:
			{
				// undo
				if ( event.key.keysym.mod & c_shortcutModifierKeys )
					StringModeUndo();
				break;
			}
		case SDLK_SPACE:
			StringModeSaveUndoState();
			break;
		case SDLK_RETURN:
			if ( m_stringMode )
			{
				if ( m_stringModeIsMultiline )
				{
					// our regular text input doesn't support adding newlines to strings
					// for some reason (seems like they don't come through in SDL's TextInput
					// events?), so let's do it here explicitly.
					HandleTextInput("\n");
				}
				else
				{
					StringModeSaveUndoState();
					SetStringMode(false, vsBox2D());
				}
			}
			break;
		case SDLK_ESCAPE:
			if ( m_stringMode )
			{
				StringModeCancel();
			}
			break;
		default:
			break;
	}
}

void
vsInput::HandleKeyDown( const SDL_Event& event )
{
	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		// consider setting the axis as 'pressed' if previous value was not
		// pressed but we got a 'pressed' event.
		vsInputAxis& axis = m_axis[i];
		if ( axis.currentValue == 0 )
		{
			for ( int j = 0; j < axis.positive.ItemCount(); j++ )
			{
				if ( axis.positive[j].type == CT_Keyboard &&
						axis.positive[j].id == event.key.keysym.scancode )
					axis.wasPressed = true;
			}
		}
	}

	switch( event.key.keysym.scancode )
	{
		case SDL_SCANCODE_Q:
			if ( vsSystem::Instance()->IsExitGameKeyEnabled() )
			{
				core::SetExitToMenu();
			}
			// m_keyControlState[CID_Exit] = 1.0f;
			break;
		case SDL_SCANCODE_ESCAPE:
			if ( vsSystem::Instance()->IsExitApplicationKeyEnabled() )
			{
				core::SetExit();
			}
			// m_keyControlState[CID_Escape] = 1.0f;
			break;
		case SDL_SCANCODE_RETURN:
			{
				// if ( m_keyControlState[CID_Start] == 0.0f )
				{
					SDL_Keymod keymod = SDL_GetModState();
					if ( keymod & KMOD_LALT )
					{
						// alt-enter means toggle fullscreen!
						vsSystem::Instance()->ToggleFullscreen();
					}
				}
				// m_keyControlState[CID_Start] = 1.0f;
				break;
			}
#ifdef _DEBUG
		case SDL_SCANCODE_P:
			// reload materials, (DEBUG ONLY)
			{
				vsMaterialManager *mm = static_cast<vsMaterialManager*>(vsMaterialManager::Instance());
				if ( mm )
					mm->ReloadAll();
			}
			break;
#endif // _DEBUG
		default:
			break;
	}
}

void
vsInput::HandleKeyUp( const SDL_Event& event )
{
	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		vsInputAxis& axis = m_axis[i];
		for ( int j = 0; j < axis.positive.ItemCount(); j++ )
		{
			if ( axis.positive[j].type == CT_Keyboard &&
					axis.positive[j].id == event.key.keysym.scancode )
				axis.wasReleased = true;
		}
	}

	switch( event.key.keysym.scancode ){
		default:
			break;
	}
}

void
vsInput::HandleMouseButtonEvent( const SDL_Event& event )
{
	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		vsInputAxis& axis = m_axis[i];
		for ( int j = 0; j < axis.positive.ItemCount(); j++ )
		{
			if ( axis.positive[j].type == CT_MouseButton &&
					axis.positive[j].id == event.button.button )
			{
				if ( event.type == SDL_MOUSEBUTTONDOWN )
					axis.wasPressed = true;
				else
					axis.wasReleased = true;
			}
		}
	}
}

void
vsInput::InitController(int i)
{
#if defined(VS_GAMEPADS)
	SDL_GameController *gc = SDL_GameControllerOpen(i);
	if ( gc && i < MAX_JOYSTICKS )
	{
		vsDelete( m_controller[i] );
		m_controller[i] = new vsController(gc, i);
	}
	else
	{
		vsLog("...ignoring it.");
	}
#endif // VS_GAMEPADS
}

void
vsInput::DestroyController(SDL_GameController *gc)
{
#if defined(VS_GAMEPADS)
	for ( int i = 0; i < MAX_JOYSTICKS; i++ )
	if ( m_controller[i] && m_controller[i]->Matches(gc) )
	{
		vsDelete( m_controller[i] );
	}
#endif // VS_GAMEPADS
}

float
DeviceControl::Evaluate(bool hasFocus)
{
	// If we don't have focus, don't obey keyboard or mouse controls.
	// DO still obey gamepad/joystick controls.
	if ( !hasFocus &&
			(type == CT_Keyboard ||
			type == CT_MouseButton ||
			type == CT_MouseWheel )
	   )
	{
		return 0.f;
	}

	float value = 0.f;
	switch( type )
	{
		case CT_Axis:
			{
				vsController *c = vsInput::Instance()->GetController();
				if ( c )
					value = c->ReadAxis(id, dir);
				break;
			}
		case CT_Button:
			{
				vsController *c = vsInput::Instance()->GetController();
				if ( c )
					value = c->ReadButton(id);
				break;
			}
			break;
		case CT_Hat:
			break;
		case CT_MouseWheel:
			{
				value = vsInput::Instance()->m_wheelValue;
				if ( dir == CD_Negative )
					value *= -1.0f;

				value = vsMax( value, 0.f );
				break;
			}
		case CT_MouseButton:
			{
				value = vsInput::Instance()->ReadMouseButton(id);
				break;
			}
		case CT_Keyboard:
			{
				if ( vsInput::Instance()->IsKeyboardSuppressed() )
					value = 0;
				else
				{
					int keyCount;
					const Uint8* keys = SDL_GetKeyboardState(&keyCount);
					if ( id < keyCount )
					{
						// SDL_Keymod actual_keymod = SDL_GetModState();
						// const int keymodsWeCareAbout =
						// 	KMOD_ALT | KMOD_CTRL | KMOD_GUI;
						// bool doMasking = false;
                        //
						// if ( id == SDL_SCANCODE_LALT || id == SDL_SCANCODE_LCTRL ||
						// 		id == SDL_SCANCODE_LGUI )
						// 	doMasking = false;
                        //
						// if ( !doMasking || ((actual_keymod & keymodsWeCareAbout) == keymod) )
							value = keys[id] ? 1.0f : 0.0f;
						// else
						// 	value = 0.0f;
					}
				}
				break;
			}
		case CT_None:
		default:
			break;
	}
	return value;
}

vsInputAxis::vsInputAxis():
	name(""),
	description(""),
	lastValue(0.f),
	currentValue(0.f),
	wasPressed(false),
	wasReleased(false),
	isLoaded(false),
	isCalculated(false)
{
}

void
vsInputAxis::Update( bool hasFocus, bool hadFocus )
{
	lastValue = currentValue;
	currentValue = 0.0f;

	if ( isCalculated )
	{
		currentValue = vsInput::Instance()->GetState( positiveAxisId ) -
			vsInput::Instance()->GetState( negativeAxisId );
	}
	else
	{
		float value;
		for ( int i = 0; i < positive.ItemCount(); i++ )
		{
			value = positive[i].Evaluate( hasFocus );
			currentValue += value;

			// value = negative[i].Evaluate();
			// currentValue -= value;
		}

		// we go straight to "IsDown", with no "Pressed" in between, if
		// we've just gained focus, since the press happened whilst somebody
		// else had focus.
		if ( hasFocus && !hadFocus )
			lastValue = currentValue;
	}
}

const struct vsInputAxis*
vsInput::GetAxis(const vsString& name)
{
	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		if ( m_axis[i].name == name )
			return &m_axis[i];
	}
	vsLog("vsLog: Unable to find requested axis '%s'", name);
	return nullptr;
}

int
vsInput::GetAxisId(const vsString& name)
{
	for ( int i = 0; i < m_axis.ItemCount(); i++ )
	{
		if ( m_axis[i].name == name )
			return i;
	}
	vsLog("vsLog: Unable to find requested axis '%s'", name);
	return -1;
}

vsString
vsInput::GetBindDescription( const DeviceControl& dc )
{
	switch ( dc.type )
	{
		case CT_None:
			return "[-]";
			break;
		case CT_Axis:
			return vsFormatString("Axis %d", dc.id);
			break;
		case CT_Button:
			return vsFormatString("Button %d", dc.id);
			break;
		case CT_Hat:
			return vsFormatString("Hat %d", dc.id);
			break;
		case CT_MouseButton:
			switch( dc.id )
			{
				case SDL_BUTTON_LEFT:
					return "Left Mouse Button";
					break;
				case SDL_BUTTON_MIDDLE:
					return "Middle Mouse Button";
					break;
				case SDL_BUTTON_RIGHT:
					return "Right Mouse Button";
					break;
			}
			break;
		case CT_MouseWheel:
			if ( dc.dir == CD_Positive )
				return "Wheel Up";
			else
				return "Wheel Down";
			break;
		case CT_Keyboard:
			// Okay, here's the thing.  Our bindings are done with "SCANCODES",
			// which means that if you change your keyboard layout, the bindings
			// remain in the same POSITION on the newly mapped keys.  That's
			// great.  It means that we can just say "use WASD" in our code, and
			// it'll use the keys in those position, no matter what keyboard is
			// in use.
			//
			// HOWEVER, when we're asking for a description for the binding, we
			// probably don't want to actually use the scancode name.  Instead,
			// we want to convert through the keyboard layout and output the actual
			// key name!  (Right now, as I type this, the only thing anywhere in
			// all my games that uses this function are the control binding
			// interface and tooltips in MMORPG Tycoon 2, which definitely do both
			// want the key name, not the scancode name.  So let's return
			// that, instead.)
			//
			// PREVIOUSLY:
			// return SDL_GetScancodeName( (SDL_Scancode)dc.id );

			{
				SDL_Keycode keycode = SDL_GetKeyFromScancode( (SDL_Scancode)dc.id );
				return SDL_GetKeyName( keycode );
			}

			break;
		default:
			break;
	}
	return "UNKNOWN";
}

void
vsInput::Rebind( int cid, const DeviceControl& dc )
{
	// Okay, this is a little awkward;  need to think more about how I want
	// this to work.
	//
	// Right now, this "Rebind" function changes only the FIRST binding point
	// for an axis.  If there's no control bound, then we add one.  If there's
	// more than one, we REPLACE the first one.  So if you have multiple bindings,
	// the first one can be rebound, the others are static.  Eew.

	if ( m_axis[cid].positive.IsEmpty() )
	{
		m_axis[cid].positive.AddItem(dc);
	}
	else
	{
		m_axis[cid].positive[0] = dc;
	}

}

bool
vsInput::AnythingIsDown()
{
	bool anythingDown = false;

	// For right now, let's just check for keyboard.
	int keyCount;
	const Uint8* keys = SDL_GetKeyboardState(&keyCount);

	// TODO:  It'd probably be *heaps* faster to check this four or eight
	// bytes at a time, instead of key-by-key.  But then you have to cope
	// with the end of the array possibly not filling all those bytes up.
	// And honestly, we're virtually never going through this code at all,
	// so I'm being silly even considering trying to optimise this test for
	// any keys being held down.  I'm sorry.
	for ( int i = 0; i < keyCount; i++ )
	{
		if ( keys[i] != 0 )
		{
			anythingDown = true;
			break;
		}
	}
	return anythingDown;
}

