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

#include "VS_Preferences.h"

#if TARGET_OS_IPHONE
#include "Wedge.h"
#else
#include <SDL/SDL.h>
#endif

vsInput::vsInput()
{
	m_captureMouse = false;
}

vsInput::~vsInput()
{
}

void
vsInput::Init()
{
	m_preparingToPoll = false;
	m_pollingForDeviceControl = false;
	m_stringMode = false;

	m_axisCenter = NULL;
	m_axisThrow = NULL;
	m_joystickAxes = -1;
	m_joystickButtons = -1;
	m_joystickHats = -1;
	for ( int i = 0; i < CID_MAX; i++ )
	{
		m_lastControlState[i] = 0.f;
		m_controlState[i] = 0.f;
	}

#if !TARGET_OS_IPHONE
	m_joystick = NULL;
	// try to get a joystick

	int joystickCount = SDL_NumJoysticks();

	if ( joystickCount )
	{
		printf("Found %d joysticks.\n", joystickCount);
		m_joystick = SDL_JoystickOpen(0);
		m_joystickAxes = SDL_JoystickNumAxes(m_joystick);
		m_joystickButtons = SDL_JoystickNumButtons(m_joystick);
		m_joystickHats = SDL_JoystickNumHats(m_joystick);
		printf("Using %s, %d axes, %d buttons, %d hats, %d balls\n", SDL_JoystickName(0), SDL_JoystickNumAxes(m_joystick),
			   SDL_JoystickNumButtons(m_joystick), SDL_JoystickNumHats(m_joystick), SDL_JoystickNumBalls(m_joystick));


		m_axisCenter = new float[m_joystickAxes];
		m_axisThrow = new float[m_joystickAxes];
		for ( int i = 0; i < m_joystickAxes; i++ )
		{
			m_axisCenter[i] = ReadAxis_Raw(i);
			m_axisThrow[i] = 1.0f;
		}

		//SDL_JoystickEventState(SDL_ENABLE);
	}
	else
		printf("No joystick found.  Using keyboard input.\n");


	Load();

	Update(0.0f);	// run a zero-time update, so we can correctly identify key presses vs. holds on the first frame.
#endif
}

void
vsInput::Deinit()
{
	Save();

	if ( m_axisCenter )
		vsDeleteArray(m_axisCenter);
	if ( m_axisThrow )
		vsDeleteArray(m_axisThrow);
}

static vsString s_cidName[CID_MAX] =
{
	"CID_UpDownLAxis",
	"CID_LeftRightLAxis",
	"CID_LUp",
	"CID_LDown",
	"CID_LLeft",
	"CID_LRight",

	"CID_UpDownRAxis",
	"CID_LeftRightRAxis",
	"CID_RUp",
	"CID_RDown",
	"CID_RLeft",
	"CID_RRight",

	"CID_Up",
	"CID_Down",
	"CID_Left",
	"CID_Right",

	"CID_A",
	"CID_B",
	"CID_TriggerR",
	"CID_TriggerL",

	"CID_ZoomIn",
	"CID_ZoomOut",

	"CID_MouseLeftButton",
	"CID_MouseMiddleButton",
	"CID_MouseRightButton",

	"CID_MouseWheel",
	"CID_MouseWheelUp",
	"CID_MouseWheelDown",

	"CID_Touch0",
	"CID_Touch1"
};

void
vsInput::Load()
{
#if !TARGET_OS_IPHONE
	// set some sensible defaults.  (XBox 360 gamepad)
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

	if ( m_joystickButtons >= 2 )
	{
		m_controlMapping[CID_A].Set( CT_Button, 0 );
		m_controlMapping[CID_B].Set( CT_Button, 1 );
	}

	if ( m_joystickHats > 0 )
	{
		m_controlMapping[CID_Up].Set( CT_Hat, 0, CD_Hat_Up );
		m_controlMapping[CID_Down].Set( CT_Hat, 0, CD_Hat_Down );
		m_controlMapping[CID_Left].Set( CT_Hat, 0, CD_Hat_Left );
		m_controlMapping[CID_Right].Set( CT_Hat, 0, CD_Hat_Right );
	}

	m_controlMapping[CID_MouseLeftButton].Set( CT_MouseButton, SDL_BUTTON_LEFT );
	m_controlMapping[CID_MouseMiddleButton].Set( CT_MouseButton, SDL_BUTTON_MIDDLE );
	m_controlMapping[CID_MouseRightButton].Set( CT_MouseButton, SDL_BUTTON_RIGHT );
	m_controlMapping[CID_MouseWheelUp].Set( CT_MouseButton, SDL_BUTTON_WHEELUP );
	m_controlMapping[CID_MouseWheelDown].Set( CT_MouseButton, SDL_BUTTON_WHEELDOWN );

	if ( m_joystick )
	{
		vsString joystickName = SDL_JoystickName(0);
		if ( joystickName.length() == 0 )
			joystickName = "Generic";
		vsPreferences p(joystickName);

		for ( int i = 0; i < CID_MAX; i++ )
		{
			vsPreferenceObject *type, *id, *dir;

			type = p.GetPreference( vsFormatString("%sType", s_cidName[i].c_str()), m_controlMapping[i].type, 0, CT_MAX );
			id = p.GetPreference( vsFormatString("%sId", s_cidName[i].c_str()), m_controlMapping[i].cid, 0, CID_MAX );
			dir = p.GetPreference( vsFormatString("%sDir", s_cidName[i].c_str()), m_controlMapping[i].dir, 0, CD_MAX );

			m_controlMapping[i].type = (ControlType)type->GetValue();
			m_controlMapping[i].cid = id->GetValue();
			m_controlMapping[i].dir = (ControlDirection)dir->GetValue();
		}
	}
#endif
	m_mappingsChanged = false;
}

void
vsInput::Save()
{
#if !TARGET_OS_IPHONE
	if ( m_joystick && m_mappingsChanged )
	{
		vsString joystickName = SDL_JoystickName(0);
		if ( joystickName.length() == 0 )
			joystickName = "Generic";
		vsPreferences p(joystickName);

		for ( int i = 0; i < CID_MAX; i++ )
		{
			vsPreferenceObject *type = p.GetPreference( vsFormatString("%sType", s_cidName[i].c_str()), m_controlMapping[i].type, 0, CT_MAX );
			vsPreferenceObject *id = p.GetPreference( vsFormatString("%sId", s_cidName[i].c_str()), m_controlMapping[i].cid, 0, CID_MAX );
			vsPreferenceObject *dir = p.GetPreference( vsFormatString("%sDir", s_cidName[i].c_str()), m_controlMapping[i].dir, 0, CD_MAX );

			type->SetValue( m_controlMapping[i].type );
			id->SetValue( m_controlMapping[i].cid );
			dir->SetValue( m_controlMapping[i].dir );
		}

		p.Save();
	}
#endif
}

void
vsInput::SetStringMode(bool mode)
{
#if !TARGET_OS_IPHONE
	if ( mode != m_stringMode )
	{
		if ( mode )
		{
			m_stringMode = true;
			m_stringModeString.clear();

			//Enable Unicode
			SDL_EnableUNICODE( SDL_ENABLE );
		}
		else
		{
			m_stringMode = false;

			//Disable Unicode
			SDL_EnableUNICODE( SDL_DISABLE );

		}
	}
#endif
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

	m_keyControlState[CID_MouseWheelUp] = 0.f;
	m_keyControlState[CID_MouseWheelDown] = 0.f;
	m_keyControlState[CID_MouseWheel] = 0.f;

	for ( int i = 0; i < CID_MAX; i++ )
		m_lastControlState[i] = m_controlState[i];

	m_mouseMotion = vsVector2D::Zero;

#if TARGET_OS_IPHONE

	m_controlState[CID_MouseLeftButton] = ::GetTouch(0);
	m_controlState[CID_Touch0] = ::GetTouch(0);
	m_controlState[CID_Touch1] = ::GetTouch(1);

	//extern vsVector2D touch;


	m_mousePos = vsVector2D(GetTouchX(0),GetTouchY(0));

	m_mousePos.x /= (.5f * vsSystem::GetScreen()->GetTrueWidth());
	m_mousePos.y /= (.5f * vsSystem::GetScreen()->GetTrueHeight());
	m_mousePos -= vsVector2D(1.f,1.f);

	m_mousePos = Correct2DInputForOrientation( m_mousePos );
#else
	SDL_Event event;

	if ( m_stringMode )
	{
		while( SDL_PollEvent( &event ) ){
			//vsString temp = m_stringModeString;

			switch( event.type ){
				case SDL_ACTIVEEVENT:
					if ( event.active.type == SDL_APPINPUTFOCUS )
					{
						vsSystem::Instance()->SetAppHasFocus( event.active.gain );
					}
					break;
				case SDL_KEYDOWN:
					//Keep a copy of the current version of the string
					//If the string less than maximum size
					if( m_stringModeString.length() <= 1024 )
					{
						//If the key is a space
						/*if( event.key.keysym.unicode == (Uint16)' ' ||
							(( event.key.keysym.unicode >= (Uint16)'0' ) && ( event.key.keysym.unicode <= (Uint16)'9' )) ||
							( ( event.key.keysym.unicode >= (Uint16)'A' ) && ( event.key.keysym.unicode <= (Uint16)'Z' ) ) ||
							( ( event.key.keysym.unicode >= (Uint16)'a' ) && ( event.key.keysym.unicode <= (Uint16)'z' ) ) ||
						    ( ( event.key.keysym.unicode >= (Uint16)'') */
						if( event.key.keysym.unicode >= (Uint16)' ' && event.key.keysym.unicode <= (Uint16)'~' )
						{
							//Append the character
							m_stringModeString += (char)event.key.keysym.unicode;
						}
					}
					if( ( event.key.keysym.sym == SDLK_BACKSPACE ) && ( m_stringModeString.length() != 0 ) )
					{
						//Remove a character from the end
						m_stringModeString.erase( m_stringModeString.length() - 1 );
					}
					else if ( event.key.keysym.sym == SDLK_RETURN )
					{
						SetStringMode(false);
					}
					break;
				case SDL_QUIT:
					core::SetExit();
					break;
				default:
					break;
			}
		}
	}
	else	// !m_stringMode
	{
		//		int i, j;
		while( SDL_PollEvent( &event ) ){
			switch( event.type ){
				case SDL_ACTIVEEVENT:
				{
					if ( event.active.state & SDL_APPINPUTFOCUS )
					{
						vsSystem::Instance()->SetAppHasFocus( event.active.gain );
					}
					break;
				}
				case SDL_MOUSEBUTTONDOWN:
				{
					switch( event.button.button ){
						case SDL_BUTTON_WHEELUP:
							m_keyControlState[CID_MouseWheelUp] += 1.f;
							break;
						case SDL_BUTTON_WHEELDOWN:
							m_keyControlState[CID_MouseWheelDown] += 1.f;
							break;
					}
					break;
				}
				case SDL_MOUSEMOTION:
				{
					m_mousePos = vsVector2D((float)event.motion.x,(float)event.motion.y);

					m_mousePos.x /= (.5f * vsSystem::GetScreen()->GetTrueWidth());
					m_mousePos.y /= (.5f * vsSystem::GetScreen()->GetTrueHeight());
					m_mousePos -= vsVector2D(1.f,1.f);

					if ( m_suppressFirstMotion )
					{
						m_suppressFirstMotion = false;
					}
					else
					{
						m_mouseMotion = vsVector2D((float)event.motion.xrel,(float)event.motion.yrel);

						m_mouseMotion.x /= (.5f * vsSystem::GetScreen()->GetTrueWidth());
						m_mouseMotion.y /= (.5f * vsSystem::GetScreen()->GetTrueHeight());
						// TODO:  CORRECT FOR ORIENTATION ON IOS DEVICES
					}
					break;
				}
				case SDL_KEYDOWN:
				{
					switch( event.key.keysym.sym ){
						//case SDLK_q:
						//	core::SetExitToMenu();
						//	break;
						case SDLK_ESCAPE:
							core::SetExit();
							break;
						case SDLK_w:
							m_keyControlState[CID_Up] = 1.0f;
							m_keyControlState[CID_LUp] = 1.0f;
							break;
						case SDLK_s:
							m_keyControlState[CID_Down] = 1.0f;
							m_keyControlState[CID_LDown] = 1.0f;
							break;
						case SDLK_a:
							m_keyControlState[CID_Left] = 1.0f;
							m_keyControlState[CID_LLeft] = 1.0f;
							break;
						case SDLK_d:
							m_keyControlState[CID_Right] = 1.0f;
							m_keyControlState[CID_LRight] = 1.0f;
							break;
						case SDLK_SPACE:
							m_keyControlState[CID_A] = 1.0f;
							break;
						case SDLK_LALT:
							m_keyControlState[CID_B] = 1.0f;
							break;
//						case 'a':
//						case 'A':
//							m_keyControlState[CID_ZoomIn] = 1.f;
//							break;
//						case 'z':
//						case 'Z':
//							m_keyControlState[CID_ZoomOut] = 1.f;
//							break;
						default:
							break;
					}
					break;
				}
				case SDL_KEYUP:
				{
					switch( event.key.keysym.sym ){
						case SDLK_w:
							m_keyControlState[CID_Up] = 0.0f;
							m_keyControlState[CID_LUp] = 0.0f;
							break;
						case SDLK_s:
							m_keyControlState[CID_Down] = 0.0f;
							m_keyControlState[CID_LDown] = 0.0f;
							break;
						case SDLK_a:
							m_keyControlState[CID_Left] = 0.0f;
							m_keyControlState[CID_LLeft] = 0.0f;
							break;
						case SDLK_d:
							m_keyControlState[CID_Right] = 0.0f;
							m_keyControlState[CID_LRight] = 0.0f;
							break;
						case SDLK_SPACE:
							m_keyControlState[CID_A] = 0.0f;
							break;
						case SDLK_LALT:
							m_keyControlState[CID_B] = 0.0f;
							break;
//						case 'a':
//						case 'A':
//							m_keyControlState[CID_ZoomIn] = 0.f;
//							break;
//						case 'z':
//						case 'Z':
//							m_keyControlState[CID_ZoomOut] = 0.f;
//							break;
						default:
							break;
					}
					break;
				}
				case SDL_QUIT:
					core::SetExit();
					break;
				default:
					break;
			}
		}
	}

	if ( m_joystick )
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
					ReadAxis( dc->cid, dc->dir, i );
				}
				else if ( dc->type == CT_Button )
				{
					ReadButton( dc->cid, i );
				}
				else if ( dc->type == CT_Hat )
				{
					ReadHat( dc->cid, dc->dir, i );
				}
				else if ( dc->type == CT_MouseButton )
				{
					ReadMouseButton( dc->cid, i );	// now read in the event loop, as mouse clicks are momentary.
				}
				else
					m_controlState[i] = 0.f;
			}
		}
	}
	else
	{
		for ( int i = 0; i < CID_MAX; i++ )
		{
			DeviceControl *dc = &m_controlMapping[i];

			if ( dc->type == CT_MouseButton )
			{
				ReadMouseButton( dc->cid, i );
			}
			else
			{
				m_controlState[i] = 0.f;
			}
		}
	}

	for ( int i = 0; i < CID_MAX; i++ )
	{
		if ( m_keyControlState[i] > m_controlState[i] )
			m_controlState[i] = m_keyControlState[i];
	}

	m_controlState[CID_LeftRightLAxis] = m_controlState[CID_LRight] - m_controlState[CID_LLeft];
	m_controlState[CID_UpDownLAxis] = m_controlState[CID_LDown] - m_controlState[CID_LUp];

	m_controlState[CID_LeftRightRAxis] = m_controlState[CID_RRight] - m_controlState[CID_RLeft];
	m_controlState[CID_UpDownRAxis] = m_controlState[CID_RDown] - m_controlState[CID_RUp];

	m_controlState[CID_MouseWheel] = m_controlState[CID_MouseWheelDown] - m_controlState[CID_MouseWheelUp];


	// mouse warping
	{
		/*int x,y;

		SDL_GetMouseState(&x,&y);

		// this is based on the window size.  (0,0 .. width,height)
		m_mousePos = vsVector2D((float)x,(float)y);

		m_mousePos.x /= (.5f * vsSystem::GetScreen()->GetTrueWidth());
		m_mousePos.y /= (.5f * vsSystem::GetScreen()->GetTrueHeight());
		m_mousePos -= vsVector2D(1.f,1.f);

		m_mousePos = Correct2DInputForOrientation( m_mousePos );*/

		// now m_mousePos is [-1..1]

		if ( m_captureMouse )
		{
			//SDL_WarpMouse( vsSystem::GetScreen()->GetTrueWidth() >> 1,
			//			  vsSystem::GetScreen()->GetTrueHeight() >> 1 );
		}
	}

#endif
}

float
vsInput::ReadMouseButton( int buttonID )
{
#if !TARGET_OS_IPHONE
	int buttons = SDL_GetMouseState(NULL,NULL);
	bool buttonDown = !!(buttons & SDL_BUTTON(buttonID));

	return buttonDown?1.0f:0.0f;
#else
	return 0.0f;
#endif
}

void
vsInput::ReadMouseButton( int buttonID, int cid )
{
	m_controlState[cid] = ReadMouseButton(buttonID);
}

float
vsInput::ReadAxis_Raw( int axisID )
{
#if !TARGET_OS_IPHONE
	float axisValue = SDL_JoystickGetAxis(m_joystick, axisID) / 32767.0f;
#else
	float axisValue = 0.0f;
#endif
	return axisValue;
}

float
vsInput::ReadAxis( int axisID )
{
	float axisValue = ReadAxis_Raw( axisID );
	axisValue -= m_axisCenter[axisID];

	if ( axisValue > m_axisThrow[axisID] )
		m_axisThrow[axisID] = axisValue;

	axisValue /= m_axisThrow[axisID];

	if ( vsFabs(axisValue) < 0.2f )
		axisValue = 0.f;
	else
	{
		if ( axisValue > 0.f )
			axisValue -= 0.2f;
		else
			axisValue += 0.2f;
		axisValue *= (1.0f / 0.8f);
	}

	return axisValue;
}

void
vsInput::ReadAxis( int axisID, ControlDirection dir, int cid )
{
	float axisValue = ReadAxis( axisID );

	if ( dir == CD_Negative )
		axisValue *= -1.0f;

	if ( axisValue < 0.f )
		axisValue = 0.f;

	m_controlState[cid] = axisValue;
}

float
vsInput::ReadHat(int hatID, ControlDirection dir)
{
#if !TARGET_OS_IPHONE
	float result = 0.f;
	Uint8 sdlDir = SDL_JoystickGetHat(m_joystick, hatID);

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

void
vsInput::ReadHat(int hatID, ControlDirection dir, int cid)
{
	float hatValue = ReadHat( hatID, dir );

	m_controlState[cid] = hatValue;
}

float
vsInput::ReadButton( int buttonID )
{
	float result = 0.f;
#if !TARGET_OS_IPHONE
	bool buttonDown = !!SDL_JoystickGetButton(m_joystick, buttonID);

	if ( buttonDown )
		result = 1.0f;
#endif
	return result;
}

void
vsInput::ReadButton( int buttonID, int cid )
{
	m_controlState[cid] = ReadButton(buttonID);
}

bool
vsInput::IsDown( ControlID id )
{
	return (m_controlState[id] > 0.f);
}

bool
vsInput::WasDown( ControlID id )
{
	return (m_lastControlState[id] > 0.f);
}

bool
vsInput::WasPressed( ControlID id )
{
	return (IsDown(id) && !WasDown(id));
}

bool
vsInput::WasReleased( ControlID id )
{
	return (!IsDown(id) && WasDown(id));
}

bool
vsInput::MouseIsOnScreen()
{
#if !TARGET_OS_IPHONE
	Uint8 state = SDL_GetAppState();

	return ( state & SDL_APPMOUSEFOCUS );
#else
	return false;
#endif
}

vsVector2D
vsInput::GetMousePosition(int scene)
{
	/*int x,y;

	SDL_GetMouseState(&x,&y);

	// this is based on the window size.  (0,0 .. width,height)
	vsVector2D mousePos = vsVector2D((float)x,(float)y);

	mousePos.x /= (.5f * vsSystem::GetScreen()->GetTrueWidth());
	mousePos.y /= (.5f * vsSystem::GetScreen()->GetTrueHeight());
	mousePos -= vsVector2D(1.f,1.f);
	*/
	// now mousePos is going from (-1,-1 .. 1,1) as we go from top left to bottom right corner of the window

	vsScene *s = vsSystem::GetScreen()->GetScene(scene);
	if ( s->Is3D() )
	{
		return m_mousePos;
	}

	vsCamera2D *c = s->GetCamera();

	vsTransform2D t = c->GetCameraTransform();
	t.m_scale.y *= 0.5f;
	t.m_scale.x = t.m_scale.y * vsSystem::GetScreen()->GetAspectRatio();
	vsVector2D mousePos = t.ApplyTo( m_mousePos );

	return mousePos;
}

vsVector2D
vsInput::GetMouseMotion(int scene)
{
	vsScene *s = vsSystem::GetScreen()->GetScene(scene);
	if ( s->Is3D() )
	{
		return m_mouseMotion;
	}

	vsCamera2D *c = s->GetCamera();

	vsTransform2D t = c->GetCameraTransform();
	t.m_scale.y *= 0.5f;
	t.m_scale.x = t.m_scale.y * vsSystem::GetScreen()->GetAspectRatio();
	vsVector2D mousePos = t.ApplyTo( m_mouseMotion );

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

	vsScene *s = vsSystem::GetScreen()->GetScene(scene);
	vsVector2D touchPos( GetTouchX(touchID), GetTouchY(touchID) );

	touchPos.x /= (.5f * vsSystem::GetScreen()->GetTrueWidth());
	touchPos.y /= (.5f * vsSystem::GetScreen()->GetTrueHeight());
	touchPos -= vsVector2D(1.f,1.f);

	touchPos = Correct2DInputForOrientation( touchPos );

	if ( s->Is3D() )
	{
		return touchPos;
	}

	vsCamera2D *c = s->GetCamera();

	vsTransform2D t = c->GetCameraTransform();
	t.m_scale.y *= 0.5f;
	t.m_scale.x = t.m_scale.y * vsSystem::GetScreen()->GetAspectRatio();
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

			SDL_WM_GrabInput( SDL_GRAB_ON );
			//SDL_WarpMouse( (uint16)(.5f * vsSystem::GetScreen()->GetTrueWidth()),
			//	(uint16)(.5f * vsSystem::GetScreen()->GetTrueHeight()) );
			m_mousePos = vsVector2D::Zero;
			m_mouseMotion = vsVector2D::Zero;
			m_suppressFirstMotion = true;
		}
		else
		{
			SDL_WM_GrabInput( SDL_GRAB_OFF );
			SDL_WarpMouse( m_capturedMouseX,m_capturedMouseY );

			m_mousePos = vsVector2D((float)m_capturedMouseX,(float)m_capturedMouseY);

			m_mousePos.x /= (.5f * vsSystem::GetScreen()->GetTrueWidth());
			m_mousePos.y /= (.5f * vsSystem::GetScreen()->GetTrueHeight());
			m_mousePos -= vsVector2D(1.f,1.f);

			m_mousePos = Correct2DInputForOrientation( m_mousePos );
			m_suppressFirstMotion = true;
		}
	}
#endif
}
