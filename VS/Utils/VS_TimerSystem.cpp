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
#include "VS_Scene.h"
#include "VS_Screen.h"
#include "VS_System.h"

// set to 1 to explicitly insert delays between frames,
// if the user has disabled vsync, in order to try to
// render just at 60fps.
#define ENFORCE_FPS_MAXIMUM (1)
#define FPS_MAXIMUM (150)

#if !TARGET_OS_IPHONE
#include <SDL2/SDL.h>
#endif

vsTimerSystem *	vsTimerSystem::s_instance = NULL;

vsTimerSystemSprite::vsTimerSystemSprite():
	m_vertices( new vsRenderBuffer(vsRenderBuffer::Type_Stream) ),
	m_indices( new vsRenderBuffer(vsRenderBuffer::Type_Static) )
{
	// we're going to draw eight lines, in this format:
	//                     (5)           (6)
	//  (1)--(2)--(3)--(4)--|             |
	//  ---------------(7)--(8)------------
	//
	// 1: CPU time taken
	// 2: "Gather" draw time taken
	// 3: "Draw" draw time taken
	// 4: GPU time taken
	// 5: 60fps marker.  (if 1+2+3 reach to here, we're running at 60fps)
	// 6: 30fps marker.  (if 1+2+3 reach to here, we're running at 30fps)
	// 7: FIFO usage
	// 8: FIFO non-usage
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
	const float offsetPerMilli = 10.f;
	const int c_vertexCount = 16;
	vsRenderBuffer::PC verts[c_vertexCount];

	vsTimerSystem *ts = vsTimerSystem::Instance();

	verts[0].position.Set(0.f,0.f,0.f);
	verts[1].position.Set(offsetPerMilli * ts->GetCPUTime(), 0.f, 0.f);
	verts[0].color = c_blue;
	verts[1].color = c_blue;

	verts[2].position = verts[1].position;
	verts[3].position = verts[2].position + vsVector2D(offsetPerMilli * ts->GetGatherTime(), 0.f);
	verts[2].color = c_green;
	verts[3].color = c_green;

	verts[4].position = verts[3].position;
	verts[5].position = verts[4].position + vsVector2D(offsetPerMilli * ts->GetDrawTime(), 0.f);
	verts[4].color = c_yellow;
	verts[5].color = c_yellow;

	verts[6].position = verts[5].position;
	verts[7].position = verts[6].position + vsVector2D(offsetPerMilli * ts->GetGPUTime(), 0.f);
	verts[6].color = c_red;
	verts[7].color = c_red;

	verts[8].position.Set( offsetPerMilli * 16.666f, -10.f, 0.f );
	verts[9].position.Set( offsetPerMilli * 16.666f, 10.f, 0.f );
	verts[10].position.Set( offsetPerMilli * 33.333f, -10.f, 0.f );
	verts[11].position.Set( offsetPerMilli * 33.333f, 10.f, 0.f );
	for (int i = 8; i < 12; i++ )
		verts[i].color = c_blue;

	float fifoY = 5.f;
	int fifoSize = vsScreen::Instance()->GetFifoSize();
	int fifoUsed = vsScreen::Instance()->GetFifoUsage();
	float endPoint = offsetPerMilli * 33.333f;
	float midPoint = vsInterpolate( fifoUsed / (float)fifoSize, 0.f, endPoint );

	verts[12].position.Set( 0.f, fifoY, 0.f );
	verts[12].color = c_red;
	verts[13].position.Set( midPoint, fifoY, 0.f );
	verts[13].color = c_red;
	verts[14].position.Set( midPoint, fifoY, 0.f );
	verts[14].color = c_green;
	verts[15].position.Set( endPoint, fifoY, 0.f );
	verts[15].color = c_green;

	m_vertices->SetArray(verts, c_vertexCount);
}

vsTimerSystem::vsTimerSystem():
	m_startCpu(0),
	m_startGather(0),
	m_startDraw(0),
	m_startGpu(0),
	m_missedFrames(0),
	m_gpuTime(0),
	m_gatherTime(0),
	m_drawTime(0),
	m_cpuTime(0)
{
#if defined(DEBUG_TIMING_BAR)
	m_sprite = NULL;
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
	m_initTime = GetMicroseconds();
	//	m_startCpu = SDL_GetTicks();
	//	m_startGpu = SDL_GetTicks();
	m_startCpu = GetMicroseconds();
	m_startGather = GetMicroseconds();
	m_startDraw = GetMicroseconds();
	m_startGpu = GetMicroseconds();
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
vsTimerSystem::GetMicroseconds()
{
	uint64_t counter = SDL_GetPerformanceCounter();
	counter = (counter * 1000000) / SDL_GetPerformanceFrequency();
	return counter;
}

uint64_t
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

#if defined(DEBUG_TIMING_BAR)
	vsVector2D bl = vsScreen::Instance()->GetDebugScene()->GetBottomLeftCorner();
	m_sprite->SetPosition( bl + vsVector2D(10.0f, -120.f) );
#endif // DEBUG_TIMING_BAR

	//	uint64_t now = SDL_GetTicks();
	uint64_t now = GetMicroseconds();

	uint64_t desiredTicksPerRound = 1000000 / FPS_MAXIMUM;
	uint64_t minTicksPerRound = desiredTicksPerRound - 1000; // close enough

	uint64_t roundTime = now - m_startCpu;

	// slow down our frame rate by a lot, if our window isn't visible.
	if ( !vsSystem::Instance()->AppIsVisible() )
	{
		minTicksPerRound = 160000;
		desiredTicksPerRound = 160000;
	}

	if ( roundTime > 100000 )	// probably hit a breakpoint or something
		roundTime = m_startCpu = now - desiredTicksPerRound;

	if ( roundTime < minTicksPerRound )
	{
#if ENFORCE_FPS_MAXIMUM
		int delayTicks = (desiredTicksPerRound-roundTime)/1000;
		// vsLog("Delaying %d ticks.\n", delayTicks);
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
vsTimerSystem::EndGatherTime()
{
	// Note that we don't set our 'cpuTime' until now (after our Draw() has
	// been called), because we need last frame's cpuTime during our Draw().
	uint64_t now = GetMicroseconds();
	m_gatherTime = (now - m_startGather);
	m_cpuTime = (m_startGather - m_startCpu);
	m_startDraw = now;
}

void
vsTimerSystem::EndDrawTime()
{
	uint64_t now = GetMicroseconds();
	m_drawTime = (now - m_startDraw);
	m_startGpu = now;
}

void
vsTimerSystem::EndGPUTime()
{
	uint64_t now = GetMicroseconds();
	m_gpuTime = (now - m_startGpu);
}

void
vsTimerSystem::PostUpdate( float timeStep )
{
	UNUSED(timeStep);
	//	uint64_t now = SDL_GetTicks();
	uint64_t now = GetMicroseconds();

	m_startGather = now;
}

void
vsTimerSystem::ShowTimingBars(bool show)
{
#if defined(DEBUG_TIMING_BAR)
	m_sprite->SetVisible(show);
#endif
}

