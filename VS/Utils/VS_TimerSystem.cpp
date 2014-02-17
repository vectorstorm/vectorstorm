/*
 *  SYS_FrameRateCap.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_TimerSystem.h"
#include "CORE_Game.h"
#include "Core.h"

#include "VS_DisplayList.h"
#include "VS_RenderQueue.h"
#include "VS_Scene.h"
#include "VS_Screen.h"
#include "VS_System.h"

#if !TARGET_OS_IPHONE
#include <SDL2/SDL.h>
#endif

#define TICK_INTERVAL (1)	// 16 ticks per frame;  slightly over 60fps

vsTimerSystem *	vsTimerSystem::s_instance = NULL;

#if defined(_WIN32)

#include <windows.h>

static LARGE_INTEGER g_liFrequency;
static LARGE_INTEGER g_liCurrent;

#else

#include <sys/time.h>

#endif

vsTimerSystemEntity::vsTimerSystemEntity()
{
	m_material = new vsMaterial( "White" );
}

vsTimerSystemEntity::~vsTimerSystemEntity()
{
	vsDelete(m_material);
}

void
vsTimerSystemEntity::Draw( vsRenderQueue *queue )
{
	vsDisplayList *list = queue->GetGenericList();

	// TODO:  Fix, so we're not casting to floats any more;  should be able to do this entirely with
	// integers.  (Used to use float timing, but now it's done with integers;  need to update the
	// vsTimerSystemEntity to render using the new integer method)
	float offsetPerMilli = 0.01f;
	vsVector2D startPoint(0.1f, 0.9f);
	vsVector2D cpuPos(offsetPerMilli, 0.0f);
	cpuPos *= (float)vsTimerSystem::Instance()->GetCPUTime();
	cpuPos += startPoint;
	vsVector2D renderPos(offsetPerMilli, 0.0f);
	renderPos *= (float)vsTimerSystem::Instance()->GetRenderTime();
	renderPos += cpuPos;
	vsVector2D gpuPos(offsetPerMilli, 0.0f);
	gpuPos *= (float)vsTimerSystem::Instance()->GetGPUTime();
	gpuPos += renderPos;

	list->SetMaterial( m_material );
	list->SetColor( c_blue );
	list->MoveTo( startPoint );
	list->LineTo( cpuPos );

	list->SetColor( c_green );
	list->MoveTo( cpuPos );
	list->LineTo( renderPos );

	list->SetColor( c_red );
	list->MoveTo( renderPos );
	list->LineTo( gpuPos );

	vsVector2D sixtyUp( offsetPerMilli * 16.666f, -0.02f );
	vsVector2D thirtyUp( offsetPerMilli * 33.333f, -0.02f );

	vsVector2D sixtyDown( sixtyUp.x, 0.02f );
	vsVector2D thirtyDown( thirtyUp.x, 0.02f );

	list->SetColor( c_blue );
	list->MoveTo( sixtyUp + startPoint );
	list->LineTo( sixtyDown + startPoint );

	list->MoveTo( thirtyUp + startPoint );
	list->LineTo( thirtyDown + startPoint );
}

vsTimerSystem::vsTimerSystem()
{
#if defined(DEBUG_TIMING_BAR)
	m_entity = NULL;
#endif // DEBUG_TIMING_BAR

	s_instance = this;
}

vsTimerSystem::~vsTimerSystem()
{
	s_instance = NULL;
}

void
vsTimerSystem::Init()
{
#if defined(_WIN32)
	if (!QueryPerformanceFrequency(&g_liFrequency))
		printf("QPF() failed with error %d\n", GetLastError());
#endif
	m_initTime = GetMicroseconds();
	//	m_startCpu = SDL_GetTicks();
	//	m_startGpu = SDL_GetTicks();
	m_startCpu = GetMicroseconds();
	m_startRender = GetMicroseconds();
	m_startGpu = GetMicroseconds();
	m_missedFrames = 0;
	m_firstFrame = true;

#if defined(DEBUG_TIMING_BAR)
	if ( !m_entity )	// we get 'initted' multiple times;  make sure we don't re-allocate this!
	{
		m_entity = new vsTimerSystemEntity;
		vsScreen::Instance()->GetDebugScene()->RegisterEntityOnTop( m_entity );
	}
#endif // DEBUG_TIMING_BAR
}

void
vsTimerSystem::Deinit()
{
#if defined(DEBUG_TIMING_BAR)
	vsDelete(m_entity);
#endif // DEBUG_TIMING_BAR
}

unsigned long
vsTimerSystem::GetMicroseconds()
{
#if defined(_WIN32)
	if ( !QueryPerformanceCounter(&g_liCurrent) )
		printf("QPC() failed with error %d\n", GetLastError());

	return (unsigned long)((g_liCurrent.QuadPart * 1000000) / g_liFrequency.QuadPart);
#else	// !_WIN32
	struct timeval time;
	gettimeofday(&time, NULL);

	return (time.tv_sec * 1000000) + (time.tv_usec);
#endif	// !_WIN32
}

unsigned long
vsTimerSystem::GetMicrosecondsSinceInit()
{
	return GetMicroseconds() - m_initTime;
}

#define MAX_TIME_PER_FRAME (2.0f/60.0f)		// 60fps.
#define MIN_TIME_PER_FRAME (1.0f/60.0f)

void
vsTimerSystem::Update( float timeStep )
{
	UNUSED(timeStep);

	//	unsigned long now = SDL_GetTicks();
	unsigned long now = GetMicroseconds();

	unsigned long minTicksPerRound = 15000;
	unsigned long desiredTicksPerRound = 16000;

	unsigned long roundTime = now - m_startCpu;

	if ( !vsSystem::Instance()->AppHasFocus() )
	{
		minTicksPerRound = 160000;
		desiredTicksPerRound = 160000;
	}

	if ( roundTime > 100000 )	// probably hit a breakpoint or something
		roundTime = m_startCpu = now - desiredTicksPerRound;

	if ( roundTime < minTicksPerRound )
	{
#if !TARGET_OS_IPHONE
		int delayTicks = (desiredTicksPerRound-roundTime)/1000;
		//		vsLog("Delaying %d ticks.\n", delayTicks);
		SDL_Delay(delayTicks);
		now = GetMicroseconds();
		roundTime = now - m_startCpu;
#endif
	}

	if ( roundTime < 1000 )
	{
		// less than a millisecond between frames?  Looks like we have a driver that's refusing to wait for a vsync!
	}

	float actualTimeStep = (roundTime) / 1000000.0f;
	if ( m_firstFrame )	// first frame.
	{
		actualTimeStep = MIN_TIME_PER_FRAME;
		m_firstFrame = false;
	}
	if ( actualTimeStep > MAX_TIME_PER_FRAME )
	{
		actualTimeStep = MAX_TIME_PER_FRAME;
		m_missedFrames++;
	}

	core::GetGame()->SetTimeStep( actualTimeStep );

	m_startCpu = now;
}

void
vsTimerSystem::EndRenderTime()
{
	//	unsigned long now = SDL_GetTicks();
	unsigned long now = GetMicroseconds();
	m_renderTime = (now - m_startRender);
	m_cpuTime = (m_startRender - m_startCpu);
	m_startGpu = now;
}

void
vsTimerSystem::EndGPUTime()
{
	//	unsigned long now = SDL_GetTicks();
	unsigned long now = GetMicroseconds();
	m_gpuTime = (now - m_startGpu);
}

void
vsTimerSystem::PostUpdate( float timeStep )
{
	UNUSED(timeStep);
	//	unsigned long now = SDL_GetTicks();
	unsigned long now = GetMicroseconds();

	m_startRender = now;
}

