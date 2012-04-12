/*
 *  UT_Timer.cpp
 *  MMORPG
 *
 *  Created by Trevor Powell on 4/10/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Timer.h"

#include "Memory/VS_Serialiser.h"

vsTimer::vsTimer():
	m_time(0.f),
	m_maxTime(0.f),
	m_timeScale(1.f)
{
}

void
vsTimer::Serialise( vsSerialiser *s )
{
	s->Float( m_time );
	s->Float( m_maxTime );
	s->Float( m_timeScale );
}

void
vsTimer::Update( float timeStep )
{
	m_time += timeStep * m_timeScale;
}

