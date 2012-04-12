/*
 *  UT_Timer.h
 *  MMORPG
 *
 *  Created by Trevor Powell on 4/10/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef UT_TIMER_H
#define UT_TIMER_H

class vsSerialiser;

class vsTimer
{
	float		m_time;
	float		m_maxTime;

	float		m_timeScale;

public:

	vsTimer();

	void		SetMaximum( float max ) { m_maxTime = max; }
	void		SetCurrent( float time ) { m_time = time; }
	void		SetTimeScale( float scale ) { m_timeScale = scale; }

	void		Reset() { m_time = 0.f; }

	bool		Expired() { return (m_time >= m_maxTime); }

	float		GetCurrent() { return m_time; }

	void		Serialise( vsSerialiser *s );

	void		Update( float timeStep );
};


#endif // UT_TIMER_H

