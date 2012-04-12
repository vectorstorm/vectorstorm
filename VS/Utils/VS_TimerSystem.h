/*
 *  SYS_FrameRateCap.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include "Core/CORE_GameSystem.h"
#include "Utils/VS_Singleton.h"

#include "VS/Graphics/VS_Entity.h"
#include "VS/Graphics/VS_Screen.h"

class vsMaterial;

#if defined(DEBUG_SCENE)
#define DEBUG_TIMING_BAR
#endif

class vsTimerSystemEntity : public vsEntity
{
	vsMaterial *		m_material;
public:
	vsTimerSystemEntity();
	virtual ~vsTimerSystemEntity();
	virtual void Draw( vsRenderQueue *queue );
};

class vsTimerSystem : public coreGameSystem
{
	static vsTimerSystem *	s_instance;

	unsigned long		m_startCpu;
	unsigned long		m_startRender;
	unsigned long		m_startGpu;
	unsigned int		m_missedFrames;

	unsigned long		m_gpuTime;
	unsigned long		m_renderTime;
	unsigned long		m_cpuTime;

#if defined(DEBUG_TIMING_BAR)
	vsTimerSystemEntity *	m_entity;
#endif // DEBUG_TIMING_BAR

	bool				m_firstFrame;

public:

						vsTimerSystem();
	virtual				~vsTimerSystem();

	virtual void		Init();
	virtual void		Deinit();

	unsigned long		GetMicroseconds();

	virtual void		Update( float timeStep );
	virtual void		PostUpdate(float timeStep);
	virtual void		EndRenderTime();	// we've finished processing our display lists
	virtual void		EndGPUTime();		// OpenGL has returned control to our app

	unsigned long		GetCurrentMillis() { return m_startCpu / 1000; }
	unsigned int		GetMissedFrameCount() { return m_missedFrames / 1000; }

	unsigned long		GetGPUTime() { return m_gpuTime / 1000; }
	unsigned long		GetRenderTime() { return m_renderTime / 1000; }
	unsigned long		GetCPUTime() { return m_cpuTime / 1000; }

	static vsTimerSystem *	Instance() { return s_instance; }
};

#endif // SYS_FRAMERATECAP_H
