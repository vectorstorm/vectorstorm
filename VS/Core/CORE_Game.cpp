/*
 *  CORE_Game.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "CORE_Game.h"
#include "CORE_GameMode.h"
#include "CORE_GameRegistry.h"

#include "Input/VS_Input.h"
#include "Utils/VS_TimerSystem.h"
#include "Physics/VS_CollisionSystem.h"
#include "Sound/VS_SoundSystem.h"

#include "VS/Graphics/VS_Scene.h"
#include "VS/Graphics/VS_Screen.h"
#include "VS/Graphics/VS_Sprite.h"
#include "VS/Graphics/VS_DynamicBatchManager.h"
#include "VS/Utils/VS_System.h"

//REGISTER_GAME("Empty", coreGame)

coreGameSystem	*coreGame::s_system[GameSystem_MAX];

//coreGame *coreGame::s_instance = nullptr;

coreGame::coreGame():
	m_system(nullptr),
	m_currentMode(nullptr),
	m_sceneCount(1)
{
}

coreGame::~coreGame()
{
}

void
coreGame::Init()
{
	const vsString &name = coreGameRegistry::GetGameName(this);
	vsLog(" -- Initialising game \"%s\"", name.c_str());
	vsSystem::Instance()->EnableGameDirectory( name, true );
	vsSystem::Instance()->SetWindowCaption( name );
	vsSystem::Instance()->InitGameData();
	vsScreen::Instance()->CreateScenes(m_sceneCount);	// just one layer for now

	ConfigureGameSystems();
	InitGameSystems();

	m_firstFrame = true;

	m_framesRendered = 0;
}

void
coreGame::Deinit()
{
	const vsString &name = coreGameRegistry::GetGameName(this);
	vsLog(" -- Deinitialising game \"%s\"", name.c_str());
	DeinitGameSystems();

	vsScreen::Instance()->DestroyScenes();
	vsSystem::Instance()->DeinitGameData();
	vsSystem::Instance()->DisableGameDirectory( name );
	vsLog(" -- Exitting game \"%s\"", name.c_str());
}

void
coreGame::SetGameMode( coreGameMode *newMode )
{
	if ( m_currentMode )
		m_currentMode->Deinit();
	m_currentMode = newMode;
	if ( m_currentMode )
		m_currentMode->Init();
}

void
coreGame::StartTimer()
{
	vsTimerSystem *timer = GetTimer();

	timer->Init();	// reinit our timer, so as not to have missed frames during our load sequence tallied.
	m_startTicks = timer->GetCurrentMillis();
}

void
coreGame::StopTimer()
{
	vsTimerSystem *timer = GetTimer();

	float fps = m_framesRendered / ((timer->GetCurrentMillis()-m_startTicks)/1000.0f);

	vsLog(" ## Frames rendered: %ld", m_framesRendered );
	vsLog(" ## Average FPS: %f", fps);

	int missedFrames = timer->GetMissedFrameCount();		// how many times did we miss rendering at 60?

	vsLog(" ## Frames missed:  %d", missedFrames);
}

void
coreGame::CreateGameSystems()
{
	s_system[ GameSystem_Timer ] = new vsTimerSystem;
	s_system[ GameSystem_Input ] = new vsInput;
#ifdef USE_BOX2D_PHYSICS
	s_system[ GameSystem_Collision ] = new vsCollisionSystem;
#endif
#ifdef USE_SDL_SOUND
	s_system[ GameSystem_Sound ] = new vsSoundSystem;
#endif
}

void
coreGame::DestroyGameSystems()
{
	vsDelete( s_system[ GameSystem_Timer] );
	vsDelete( s_system[ GameSystem_Input] );
#ifdef USE_BOX2D_PHYSICS
	vsDelete( s_system[ GameSystem_Collision] );
#endif
#ifdef USE_SDL_SOUND
	vsDelete( s_system[ GameSystem_Sound] );
#endif
}


// By default, we have all game systems, in the standard order.  Other games
// can reconfigure this to get a different set of systems, or a different update
// order if they really want to.  (I can't imagine why they'd want to, but I'm
// sure somebody'll come up with a good reason)
void
coreGame::ConfigureGameSystems()
{
	m_systemCount = GameSystem_MAX;

	m_system = (coreGameSystem**)malloc( sizeof( coreGameSystem* ) * m_systemCount );
	for ( int i = 0; i < m_systemCount; i++ )
		m_system[i] = s_system[i];
}

void
coreGame::AddSystem( coreGameSystem *system )
{
	coreGameSystem** newSystems = (coreGameSystem**)malloc( sizeof( coreGameSystem* ) * (m_systemCount+1) );
	for ( int i = 0; i < m_systemCount; i++ )
		newSystems[i] = m_system[i];
	newSystems[m_systemCount] = system;

	free(m_system);
	m_system = newSystems;
	m_systemCount++;
}

void
coreGame::InitGameSystems()
{
	for ( int i = 0; i < m_systemCount; i++ )
	{
		m_system[i]->Init();
		m_system[i]->Activate();
	}
}

void
coreGame::DeinitGameSystems()
{
	for ( int i = 0; i < m_systemCount; i++ )
	{
		m_system[i]->Deinit();
//		delete m_system[i];
//		m_system[i] = nullptr;
	}
	free(m_system);
	m_system = nullptr;
}

void
coreGame::Go()
{
	m_framesRendered++;

	for ( int i = 0; i < m_systemCount; i++ )
		if ( m_system[i]->IsActive() )
			m_system[i]->Update( m_timeStep );

	if ( vsScreen::Instance()->Resized() )
	{
		HandleResize();
	}

	Update( m_timeStep );

	if ( m_currentMode )
		m_currentMode->Update( m_timeStep );

	vsScreen::Instance()->Update( m_timeStep );

	for ( int i = 0; i < m_systemCount; i++ )
		if ( m_system[i]->IsActive() )
			m_system[i]->PostUpdate( m_timeStep );

	DrawFrame();
	vsDynamicBatchManager::Instance()->FrameRendered();
}

void
coreGame::DrawFrame()
{
	vsScreen::Instance()->Draw();
}

vsInput *
coreGame::GetInput()
{
	return (vsInput *)s_system[GameSystem_Input];
}

#ifdef USE_BOX2D_PHYSICS
vsCollisionSystem *
coreGame::GetCollision()
{
	return (vsCollisionSystem *)s_system[GameSystem_Collision];
}
#endif

#ifdef USE_SDL_SOUND
vsSoundSystem *
coreGame::GetSound()
{
	return (vsSoundSystem *)s_system[GameSystem_Sound];
}
#endif

vsTimerSystem *
coreGame::GetTimer()
{
	return (vsTimerSystem *)s_system[GameSystem_Timer];
}

