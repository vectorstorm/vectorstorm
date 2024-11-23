/*
 * SYS_FrameRateCap.h
 * VectorStorm
 *
 * Created by Trevor Powell on 18/03/07.
 * Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include "Core/CORE_GameSystem.h"
#include "Utils/VS_Singleton.h"

#include "VS/Graphics/VS_Sprite.h"
#include "VS/Graphics/VS_Screen.h"

class vsMaterial;
class vsRenderBuffer;

#if defined(VS_TIMING_BARS) && defined(DEBUG_SCENE)
#define DEBUG_TIMING_BAR
#endif

class vsTimerSystemSprite : public vsSprite
{
	vsRenderBuffer *m_vertices;
	vsRenderBuffer *m_indices;
public:
	vsTimerSystemSprite();
	virtual ~vsTimerSystemSprite();
	virtual void Update( float timeStep );
	// virtual void DynamicDraw( vsRenderQueue *queue );
};

class vsTimerSystem : public coreGameSystem
{
	static vsTimerSystem *s_instance;

	uint64_t m_launchTimeRaw;
	uint64_t m_initTimeRaw;

	// we divide our frame into three buckets:
	// "Update" is our scene update.
	// "Draw" is the time we spend processing the list of draw commands.
	// "Present" is the time we spend blocked while presenting our draw.
	//
	uint64_t m_startFrame;
	uint64_t m_startDraw;
	uint64_t m_startPresent;

	uint64_t m_cpuMicroseconds;
	uint64_t m_drawMicroseconds;
	uint64_t m_presentMicroseconds;
	uint64_t m_sleepMicroseconds;

	uint64_t m_drawAccumulator;
	uint64_t m_presentAccumulator;

	unsigned int m_missedFrames;

#if defined(DEBUG_TIMING_BAR)
	vsTimerSystemSprite * m_sprite;
#endif // DEBUG_TIMING_BAR

	bool m_firstFrame;
	uint64_t GetRawTime();
	uint64_t ConvertRawTimeToMicroseconds(uint64_t raw);
	uint64_t ConvertRawTimeToMilliseconds(uint64_t raw);

public:

	vsTimerSystem();
	virtual ~vsTimerSystem();

	virtual void Init();
	virtual void Deinit();

	void BeginDraw();
	void EndDraw();
	void BeginPresent();
	void EndPresent();

	// this maximum FPS rate is automatically set by the renderer, and tells us
	// how many FPS we're allowed to render at.
	void SetMaxFPS( int fps );

	uint64_t GetMicroseconds();
	uint64_t GetMicrosecondsSinceInit();   // time since current game finished initialising
	uint64_t GetMicrosecondsSinceLaunch(); // time since program launch
	float GetSecondsSinceLaunch();

	virtual void Update( float timeStep );

	uint64_t GetCurrentMillis() { return m_startFrame / 1000; }
	unsigned int GetMissedFrameCount() { return m_missedFrames / 1000; }

	// all of the following are returning MILLISECONDS.
	// [TODO] Clean up this API to make that more obvious
	uint64_t GetCPUMilliseconds() { return m_cpuMicroseconds / 1000; }
	uint64_t GetDrawMilliseconds() { return m_drawMicroseconds / 1000; }
	uint64_t GetPresentMilliseconds() { return m_presentMicroseconds / 1000; }
	uint64_t GetSleepMilliseconds() { return m_sleepMicroseconds / 1000; }

	void ShowTimingBars(bool show);

	static vsTimerSystem * Instance() { return s_instance; }
};


#endif // SYS_FRAMERATECAP_H
