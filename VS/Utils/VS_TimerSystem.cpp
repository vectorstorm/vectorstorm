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
#include "VS_RenderBuffer.h"
#include "VS_RenderQueue.h"
#include "VS_Renderer.h"
#include "VS_Scene.h"
#include "VS_Screen.h"
#include "VS_System.h"
#include "VS_Profile.h"

// set to 1 to explicitly insert delays between frames,
// if the user has disabled vsync, in order to try to
// render just at 60fps.
#define ENFORCE_FPS_MAXIMUM (0)

#if !TARGET_OS_IPHONE
#include <SDL3/SDL.h>
#endif

vsTimerSystem *	vsTimerSystem::s_instance = nullptr;

vsTimerSystemSprite::vsTimerSystemSprite():
	m_vertices( new vsRenderBuffer(vsRenderBuffer::Type_Stream) ),
	m_indices( new vsRenderBuffer(vsRenderBuffer::Type_Static) )
{
	// we're going to draw eight lines, in this format:
	//                     (4)           (5)
	//  (1)--(2)--(3)-------|             |
	//  ---------------(6)--(7)------------
	//
	// 1: CPU time taken
	// 2: "Draw" draw time taken
	// 3: "Present" time taken
	// 4: 60fps marker.  (if 1+2+3 reach to here, we're running at 60fps)
	// 5: 30fps marker.  (if 1+2+3 reach to here, we're running at 30fps)
	//
	// Our ten indices remain the same, so we put them in a static buffer.  Our
	// vertices will change every frame, so we'll put them in a streaming
	// buffer, and update their values in our 'Update()' call each frame.
	//
	const int c_indexCount = 16;
	uint16_t indices[c_indexCount] =
	{
		0, 1,
		2, 3,
		4, 5,
		6, 7,
		8, 9,
		10, 11,
		12, 13,
		14, 15
	};
	m_indices->SetArray( indices, c_indexCount );
	m_vertices->ResizeArray( sizeof(vsRenderBuffer::PC) * c_indexCount );

	vsFragment *frag = new vsFragment;
	frag->SetMaterial("White");
	vsDisplayList *list = new vsDisplayList(64);
	list->BindBuffer(m_vertices);
	list->LineListBuffer(m_indices);
	list->ClearArrays();
	frag->SetDisplayList(list);
	AddFragment(frag);
}

vsTimerSystemSprite::~vsTimerSystemSprite()
{
	vsDelete( m_vertices );
	vsDelete( m_indices );
}

void
vsTimerSystemSprite::Update( float timeStep )
{
	const float c_offsetPerMilli = 10.f;
	const int c_vertexCount = 16;
	vsRenderBuffer::PC verts[c_vertexCount];

	vsTimerSystem *ts = vsTimerSystem::Instance();

	verts[0].position.Set(0.f,0.f,0.f);
	verts[1].position.Set(c_offsetPerMilli * ts->GetCPUMilliseconds(), 0.f, 0.f);
	verts[0].color = c_blue;
	verts[1].color = c_blue;

	verts[2].position = verts[1].position;
	verts[3].position = verts[2].position + vsVector2D(c_offsetPerMilli * ts->GetDrawMilliseconds(), 0.f);
	verts[2].color = c_green;
	verts[3].color = c_green;

	verts[4].position = verts[3].position;
	verts[5].position = verts[4].position + vsVector2D(c_offsetPerMilli * ts->GetPresentMilliseconds(), 0.f);
	verts[4].color = c_yellow;
	verts[5].color = c_yellow;

	verts[6].position = verts[5].position;
	verts[7].position = verts[6].position + vsVector2D(c_offsetPerMilli * ts->GetSleepMilliseconds(), 0.f);
	verts[6].color = c_red;
	verts[7].color = c_red;

	verts[8].position.Set( c_offsetPerMilli * 16.666f, -10.f, 0.f );
	verts[9].position.Set( c_offsetPerMilli * 16.666f, 10.f, 0.f );
	verts[10].position.Set( c_offsetPerMilli * 33.333f, -10.f, 0.f );
	verts[11].position.Set( c_offsetPerMilli * 33.333f, 10.f, 0.f );
	for (int i = 8; i < 12; i++ )
		verts[i].color = c_blue;

	m_vertices->SetArray(verts, c_vertexCount);
}

vsTimerSystem::vsTimerSystem():
	m_startFrame(0),
	m_startDraw(0),
	m_startPresent(0),
	m_cpuMicroseconds(0),
	m_drawMicroseconds(0),
	m_presentMicroseconds(0),
	m_drawAccumulator(0),
	m_presentAccumulator(0),
	m_missedFrames(0)
{
	m_launchTimeRaw = m_initTimeRaw = GetRawTime();
#if defined(DEBUG_TIMING_BAR)
	m_sprite = nullptr;
#endif // DEBUG_TIMING_BAR

	s_instance = this;
}

vsTimerSystem::~vsTimerSystem()
{
	s_instance = nullptr;
}

void
vsTimerSystem::Init()
{
	m_initTimeRaw = GetRawTime();
	//	m_startCpu = SDL_GetTicks();
	//	m_startGpu = SDL_GetTicks();
	m_startFrame = GetMicroseconds();
	m_startDraw = GetMicroseconds();
	m_startPresent = GetMicroseconds();
	m_missedFrames = 0;
	m_firstFrame = true;

#if defined(DEBUG_TIMING_BAR)
	if ( !m_sprite )	// we get 'initted' multiple times;  make sure we don't re-allocate this!
	{
		m_sprite = new vsTimerSystemSprite;
		vsScreen::Instance()->GetDebugScene()->RegisterEntityOnTop( m_sprite );
	}
#endif // DEBUG_TIMING_BAR
}

void
vsTimerSystem::Deinit()
{
#if defined(DEBUG_TIMING_BAR)
	vsDelete(m_sprite);
#endif // DEBUG_TIMING_BAR
}

uint64_t
vsTimerSystem::GetRawTime()
{
	return SDL_GetPerformanceCounter();
}

uint64_t
vsTimerSystem::ConvertRawTimeToMicroseconds(uint64_t raw)
{
	return (raw * 1000000) / SDL_GetPerformanceFrequency();
}

uint64_t
vsTimerSystem::ConvertRawTimeToMilliseconds(uint64_t raw)
{
	return (raw * 1000) / SDL_GetPerformanceFrequency();
}

uint64_t
vsTimerSystem::GetMicroseconds()
{
	uint64_t counter = SDL_GetPerformanceCounter();
	counter = (counter * 1000000) / SDL_GetPerformanceFrequency();
	return counter;
}

uint64_t
vsTimerSystem::GetMicrosecondsSinceInit()
{
	return ConvertRawTimeToMicroseconds(GetRawTime() - m_initTimeRaw);
}

uint64_t
vsTimerSystem::GetMicrosecondsSinceLaunch()
{
	return ConvertRawTimeToMicroseconds(GetRawTime() - m_launchTimeRaw);
}

float
vsTimerSystem::GetSecondsSinceLaunch()
{
	uint64_t deltaRaw = GetRawTime() - m_launchTimeRaw;

	float t = (float)deltaRaw / (float)SDL_GetPerformanceFrequency();
	return t;
}

#define MAX_TIME_PER_FRAME (2.0f/60.0f)		// 60fps.
#define MIN_TIME_PER_FRAME (1.0f/60.0f)

void
vsTimerSystem::Update( float timeStep )
{
	PROFILE("vsTimerSystem::Update");
	UNUSED(timeStep);

	m_sleepMicroseconds = 0;
#if ENFORCE_FPS_MAXIMUM
	{
		uint64_t now = GetMicroseconds();

		int maxFPS = vsRenderer::Instance()->GetRefreshRate();

		uint64_t desiredTicksPerRound = 1000000 / maxFPS;
		uint64_t minTicksPerRound = desiredTicksPerRound / 2;// close enough

		uint64_t roundTime = now - m_startFrame;

		// slow down our frame rate by a lot, if our window isn't visible.
		// if ( !vsSystem::Instance()->AppIsVisible() )
		// {
		// 	minTicksPerRound = 160000;
		// 	desiredTicksPerRound = 160000;
		// }

		if ( roundTime > 100000 )	// probably hit a breakpoint or something
			roundTime = m_startFrame = now - desiredTicksPerRound;

		if ( roundTime < minTicksPerRound )
		{
			PROFILE("vsTimerSystem::EnforceFPSMaximum");
			int delayTicks = (desiredTicksPerRound-roundTime)/1000;
			// vsLog("Delaying %d ticks.", delayTicks);
			SDL_Delay(delayTicks);

			uint64_t afterDelay = GetMicroseconds();
			m_sleepMicroseconds = afterDelay - now;
			now = afterDelay;
			roundTime = now - m_startFrame;
		}
	}
#endif


	uint64_t now = GetMicroseconds();

	uint64_t roundTime = now - m_startFrame;
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

	m_drawMicroseconds = m_drawAccumulator;
	m_presentMicroseconds = m_presentAccumulator;
	m_drawAccumulator = 0;
	m_presentAccumulator = 0;

	m_cpuMicroseconds = (now - m_startFrame) - (m_drawMicroseconds + m_presentMicroseconds + m_sleepMicroseconds);

	m_startFrame = now;

// 	if ( vsScreen::Instance()->Resized() )
// 	{
// #if defined(DEBUG_TIMING_BAR)
// 		PROFILE("PositionTimingBar");
// 		vsVector2D bl = vsScreen::Instance()->GetDebugScene()->GetBottomLeftCorner();
// 		m_sprite->SetPosition( bl + vsVector2D(10.0f, -120.f) );
// #endif // DEBUG_TIMING_BAR
// 	}

}

void
vsTimerSystem::BeginDraw()
{
	m_startDraw = GetMicroseconds();
}

void
vsTimerSystem::EndDraw()
{
	m_drawAccumulator += GetMicroseconds() - m_startDraw;
}

void
vsTimerSystem::BeginPresent()
{
	m_startPresent = GetMicroseconds();
}

void
vsTimerSystem::EndPresent()
{
	m_presentAccumulator += GetMicroseconds() - m_startPresent;
}


// void
// vsTimerSystem::EndGPUTime()
// {
// 	uint64_t now = GetMicroseconds();
// 	m_gpuTime = (now - m_startGpu);
//
// #if ENFORCE_FPS_MAXIMUM
// 	{
// 		int maxFPS = vsRenderer::Instance()->GetRefreshRate();
//
// 		uint64_t desiredTicksPerRound = 1000000 / maxFPS;
// 		uint64_t minTicksPerRound = desiredTicksPerRound / 2;// close enough
//
// 		uint64_t roundTime = now - m_startCpu;
//
// 		// slow down our frame rate by a lot, if our window isn't visible.
// 		// if ( !vsSystem::Instance()->AppIsVisible() )
// 		// {
// 		// 	minTicksPerRound = 160000;
// 		// 	desiredTicksPerRound = 160000;
// 		// }
//
// 		if ( roundTime > 100000 )	// probably hit a breakpoint or something
// 			roundTime = m_startCpu = now - desiredTicksPerRound;
//
// 		if ( roundTime < minTicksPerRound )
// 		{
// 			PROFILE("vsTimerSystem::EnforceFPSMaximum");
// 			int delayTicks = (desiredTicksPerRound-roundTime)/1000;
// 			// vsLog("Delaying %d ticks.", delayTicks);
// 			SDL_Delay(delayTicks);
// 			now = GetMicroseconds();
// 			roundTime = now - m_startCpu;
// 		}
// 	}
// #endif
// }

void
vsTimerSystem::ShowTimingBars(bool show)
{
#if defined(DEBUG_TIMING_BAR)
	m_sprite->SetVisible(show);
#endif
}

