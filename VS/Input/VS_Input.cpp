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

#include "VS_Preferences.h"
#include "Utils/utfcpp/utf8.h"
#include <iterator>


#include "VS_Heap.h"

#if TARGET_OS_IPHONE
#include "Wedge.h"
#else
#include <SDL2/SDL.h>
#endif

extern SDL_Window *g_sdlWindow;


vsInput::vsInput():
	m_stringMode(false),
	m_hasFocus(true),
	m_hadFocus(true),
	m_stringValidationType(Validation_None),
	m_mouseIsInWindow(false),
	m_backspaceMode(false)
{
	m_captureMouse = false;
	m_suppressFirstMotion = false;
	m_suppressResizeEvent = false;
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
	m_preparingToPoll = false;
	m_pollingForDeviceControl = false;
	m_stringMode = false;
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
		m_controller[i] = NULL;

	// SDL_GameControllerAddMapping("030000005e040000a102000000010000,Xbox 360 Wireless Receiver,a:b1,b:b2,y:b3,x:b0,leftx:a0,lefty:a1");
	// int joystickCount = SDL_NumJoysticks();
    //
	// if ( joystickCount )
	// {
	// 	vsLog("Found %d joysticks.", joystickCount);
    //
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
	if ( vsFile::Exists("binds.txt") )
	{
		vsFile f("binds.txt", vsFile::MODE_Read);

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
		if ( nameStr != NULL )
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
	vsFile f("binds.txt", vsFile::MODE_Write);

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
// 		if ( nameStr != NULL )
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
vsInput::SetStringMode(bool mode, ValidationType vt)
{
	SetStringMode(mode, -1, vt);
}

void
vsInput::SetStringMode(bool mode, int maxLength, ValidationType vt)
{
	if ( mode != m_stringMode )
	{
		if ( mode )
		{
			SDL_StartTextInput();
			m_stringMode = true;
			m_stringModePaused = false;
			m_stringModeString.clear();
			m_stringModeMaxLength = maxLength;
			m_stringValidationType = vt;
			SetStringModeSelectAll(false);
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
			if ( i >= m_stringModeCursorFirstGlyph && i < m_stringModeCursorLastGlyph )
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
vsInput::SetStringModeCursor( int anchorGlyph, bool endEdit )
{
	SetStringModeCursor(anchorGlyph, anchorGlyph, endEdit );
}

void
vsInput::SetStringModeCursor( int anchorGlyph, int floatingGlyph, bool endEdit )
{
	try
	{
		m_undoMode = false; // if we're moving the cursor around, we're not in undo mode any more!
		if ( endEdit && m_stringModeEditing )
		{
			StringModeSaveUndoState();
			m_stringModeEditing = false;
		}
		else if ( endEdit && anchorGlyph != floatingGlyph && m_stringModeCursorFirstGlyph == m_stringModeCursorLastGlyph )
		{
			// if we're makign a wide selection and previously we had an I-beam-style cursor,
			// save the I-beam-style position.
			StringModeSaveUndoState();
		}

		// first, clamp these positions into legal positions between glyphs
		int inLength = utf8::distance(m_stringModeString.begin(), m_stringModeString.end());
		anchorGlyph = vsClamp( anchorGlyph, 0, inLength );
		floatingGlyph = vsClamp( floatingGlyph, 0, inLength );

		m_stringModeCursorFirstGlyph = vsMin(anchorGlyph, floatingGlyph);
		m_stringModeCursorLastGlyph = vsMax(anchorGlyph, floatingGlyph);
		m_stringModeCursorAnchorGlyph = anchorGlyph;
		m_stringModeCursorFloatingGlyph = floatingGlyph;
	}
	catch(...)
	{
		vsLog("Failed to set string mode cursor position!");
	}
}

int
vsInput::GetStringModeCursorFirstGlyph()
{
	return m_stringModeCursorFirstGlyph;
}

int
vsInput::GetStringModeCursorLastGlyph()
{
	return m_stringModeCursorLastGlyph;
}

int
vsInput::GetStringModeCursorAnchorGlyph()
{
	return m_stringModeCursorAnchorGlyph;
}

int
vsInput::GetStringModeCursorFloatingGlyph()
{
	return m_stringModeCursorFloatingGlyph;
}

void
vsInput::SetStringModeSelectAll( bool selectAll )
{
	try
	{
		int lastGlyph = utf8::distance(
				m_stringModeString.c_str(),
				m_stringModeString.c_str() + m_stringModeString.size()
				);

		if ( selectAll )
		{
			SetStringModeCursor( 0, lastGlyph+1, true);
		}
		else
		{
			SetStringModeCursor( lastGlyph+1, true );
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
		int lastGlyph = utf8::distance(
				m_stringModeString.c_str(),
				m_stringModeString.c_str() + m_stringModeString.size()
				);

		return ( m_stringModeCursorFirstGlyph == 0 &&
				m_stringModeCursorLastGlyph == lastGlyph+1 );
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
				case SDL_TEXTINPUT:
					{
						if ( !m_stringModePaused )
							HandleTextInput(event.text.text);
						break;
					}
					break;
				case SDL_TEXTEDITING:
					// This event is for partial, in-progress code points
					// which haven't yet settled on a final glyph.  For now,
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
							m_mouseMotion = vsVector2D((float)event.motion.xrel,(float)event.motion.yrel);

							m_mouseMotion.x /= (.5f * vsScreen::Instance()->GetTrueWidth());
							m_mouseMotion.y /= (.5f * vsScreen::Instance()->GetTrueHeight());
							// TODO:  CORRECT FOR ORIENTATION ON IOS DEVICES
						}
						break;
					}
				case SDL_KEYDOWN:
					{
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
						HandleKeyUp(event);
						break;
					}
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					{
						HandleMouseButtonEvent(event);
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
								vsSystem::Instance()->CheckVideoMode();
								break;
							}
						case SDL_WINDOWEVENT_RESIZED:
							{
								if ( m_suppressResizeEvent )
									m_suppressResizeEvent = false;
								else
								{
									// vsLog("RESIZE EVENT:  %d, %d", event.window.data1, event.window.data2);
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
							// vsLog("Minimized");
							break;
						case SDL_WINDOWEVENT_MAXIMIZED:
							// vsLog("Maximized");
							break;
						case SDL_WINDOWEVENT_RESTORED:
							// vsLog("Restored");
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
	int buttons = SDL_GetMouseState(NULL,NULL);
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
	return NULL;
}

float
vsController::ReadAxis_Raw( int axisID )
{
#if !TARGET_OS_IPHONE
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
#if !TARGET_OS_IPHONE
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
	m_undoMode = false;
	m_backspaceMode = false;

	if ( !m_stringModeEditing )
	{
		StringModeSaveUndoState();
		m_stringModeEditing = true;
	}

	vsString oldString = m_stringModeString;
	m_stringModeString = vsEmptyString;

	try
	{
		// Okay.  First, let's copy all the glyphs up to the cursor
		// to the new string
		utf8::iterator<std::string::iterator> old( oldString.begin(), oldString.begin(), oldString.end() );
		int oldLength = utf8::distance(oldString.begin(), oldString.end());
		for ( int i = 0; i < m_stringModeCursorFirstGlyph; i++ )
			utf8::append( *(old++), back_inserter(m_stringModeString) );

		// skip any glyphs which were inside a cursor selection;  they'll be
		// replaced by the new input.
		for ( int i = m_stringModeCursorFirstGlyph; i < m_stringModeCursorLastGlyph; i++ )
			old++;

		// Now, append our new input onto the string we're building up.
		// Important point:  use utf8::append() to build up the string!  Otherwise,
		// we're just stuffing raw code points onto the end of the string, instead of
		// UTF8-encoded text!
		//
		// Note that we copy the input out into a separate string to do this, as I
		// haven't found a way to make utfcpp play nicely with const_iterators.
		vsString inputString(_input);
		utf8::iterator<std::string::iterator> input( inputString.begin(), inputString.begin(), inputString.end() );
		int inputLength = utf8::distance(inputString.begin(), inputString.end());
		for ( int i = 0; i < inputLength; i++ )
			utf8::append( *(input++), back_inserter(m_stringModeString) );

		// now add the end of our original input string;  anything which was past
		// the end of the cursor selection
		for ( int i = m_stringModeCursorLastGlyph; i < oldLength; i++ )
			utf8::append( *(old++), back_inserter(m_stringModeString));

		SetStringModeCursor( m_stringModeCursorFirstGlyph + inputLength, false );

		ValidateString();
	}
	catch (...)
	{
		vsLog("Failed to handle UTF8 text input!");
		vsLog("  String: %s", oldString);
		vsLog("  New input: %s", _input);
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

		bool hasDot = false;
		int glyphsSoFar = 0;

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
				else if ( *it == '.' )
				{
					if ( hasDot )
						valid = false;
					else
						hasDot = true;
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

			if ( valid && (m_stringModeMaxLength < 0 || glyphsSoFar < m_stringModeMaxLength) )
			{
				glyphsSoFar++;
				utf8::append( *it, back_inserter(m_stringModeString) );
			}
			else
			{
				// This glyph wasn't valid!  Therefore, we're removing it, and we
				// need to adjust the cursor positioning.

				if ( m_stringModeCursorFirstGlyph > glyphsSoFar )
					m_stringModeCursorFirstGlyph--;
				if ( m_stringModeCursorLastGlyph > glyphsSoFar )
					m_stringModeCursorLastGlyph--;
			}

			it++;
		}

		m_stringModeCursorFirstGlyph = vsMin( m_stringModeCursorFirstGlyph, glyphsSoFar );
		m_stringModeCursorLastGlyph = vsMin( m_stringModeCursorLastGlyph, glyphsSoFar );
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
	SetStringModeCursor( s.size(), false );
	// vsLog("Clearing undo stack");
}

void
vsInput::StringModeSaveUndoState()
{
	StringModeState *state = new StringModeState;
	state->string = m_stringModeString;
	state->anchorGlyph = m_stringModeCursorAnchorGlyph;
	state->floatingGlyph = m_stringModeCursorFloatingGlyph;
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
	SetStringMode(false);
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
		SetStringModeCursor( last->anchorGlyph, last->floatingGlyph, false );
		m_stringModeUndoStack.PopBack();


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
				if ( m_stringModeCursorFirstGlyph == m_stringModeCursorLastGlyph )
				{
					if ( m_stringModeCursorFirstGlyph == 0 ) // no-op, deleting from 0.
						return;
					if ( !m_backspaceMode )
						StringModeSaveUndoState();

					try
					{
						// delete one character
						vsString oldString = m_stringModeString;
						m_stringModeString = vsEmptyString;
						// Okay.  copy all the glyphs up to the cursor MINUS ONE.
						// then copy the rest.
						utf8::iterator<std::string::iterator> in( oldString.begin(), oldString.begin(), oldString.end() );
						int inLength = utf8::distance(oldString.begin(), oldString.end());
						for ( int i = 0; i < inLength; i++ )
						{
							if ( i != m_stringModeCursorFirstGlyph-1 )
								utf8::append( *in, std::back_inserter(m_stringModeString) );
							in++;
						}

						m_backspaceMode = true;
						m_undoMode = false;
						SetStringModeCursor( m_stringModeCursorFirstGlyph-1, true );
					}
					catch(...)
					{
						vsLog("Failed to handle backspace!");
					}
				}
				else
				{
					// delete the stuff that's selected.  WHich is equivalent to inserting nothing "".
					HandleTextInput("");
				}
			}
			break;
		case SDLK_LEFT:
			{
				if ( event.key.keysym.mod & KMOD_LSHIFT ||
						event.key.keysym.mod & KMOD_RSHIFT )
				{
					// shift is held down;  move the floating glyph, but not the anchor glyph!
					SetStringModeCursor( m_stringModeCursorAnchorGlyph, m_stringModeCursorFloatingGlyph-1, true );
				}
				else
				{
					// shift isn't down;  move the anchor glyph and collapse any selection
					SetStringModeCursor( m_stringModeCursorAnchorGlyph-1, true );
				}
			}
			break;
		case SDLK_RIGHT:
			{
				if ( event.key.keysym.mod & KMOD_LSHIFT ||
						event.key.keysym.mod & KMOD_RSHIFT )
				{
					// shift is held down;  move the floating glyph, but not the anchor glyph!
					SetStringModeCursor( m_stringModeCursorAnchorGlyph, m_stringModeCursorFloatingGlyph+1, true );
				}
				else
				{
					// shift isn't down;  move the anchor glyph and collapse any selection
					SetStringModeCursor( m_stringModeCursorAnchorGlyph+1, true );
				}
			}
			break;
		case SDLK_v:
			{
#if defined( __APPLE_CC__ )
				if ( event.key.keysym.mod & KMOD_LGUI ||
						event.key.keysym.mod & KMOD_RGUI )
#else
					if ( event.key.keysym.mod & KMOD_LCTRL ||
							event.key.keysym.mod & KMOD_RCTRL )
#endif
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
#if defined( __APPLE_CC__ )
				if ( event.key.keysym.mod & KMOD_LGUI ||
						event.key.keysym.mod & KMOD_RGUI )
#else
					if ( event.key.keysym.mod & KMOD_LCTRL ||
							event.key.keysym.mod & KMOD_RCTRL )
#endif
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
#if defined( __APPLE_CC__ )
				if ( event.key.keysym.mod & KMOD_LGUI ||
						event.key.keysym.mod & KMOD_RGUI )
#else
					if ( event.key.keysym.mod & KMOD_LCTRL ||
							event.key.keysym.mod & KMOD_RCTRL )
#endif
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
#if defined( __APPLE_CC__ )
				const int undoModifierKeys = KMOD_LGUI | KMOD_RGUI;
#else
				const int undoModifierKeys = KMOD_LCTRL | KMOD_RCTRL;
#endif // defined( __APPLE_CC__ )

				if ( event.key.keysym.mod & undoModifierKeys )
					StringModeUndo();
				break;
			}
		case SDLK_SPACE:
			StringModeSaveUndoState();
			break;
		case SDLK_RETURN:
			if ( m_stringMode )
			{
				StringModeSaveUndoState();
				SetStringMode(false);
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
		vsInputAxis& axis = m_axis[i];
		for ( int j = 0; j < axis.positive.ItemCount(); j++ )
		{
			if ( axis.positive[j].type == CT_Keyboard &&
					axis.positive[j].id == event.key.keysym.scancode )
				axis.wasPressed = true;
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
}

void
vsInput::DestroyController(SDL_GameController *gc)
{
	for ( int i = 0; i < MAX_JOYSTICKS; i++ )
	if ( m_controller[i] && m_controller[i]->Matches(gc) )
	{
		vsDelete( m_controller[i] );
	}
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
	return NULL;
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
			return SDL_GetScancodeName( (SDL_Scancode)dc.id );
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
