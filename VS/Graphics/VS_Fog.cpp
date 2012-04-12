/*
 *  VS_Fog.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 14/09/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Fog.h"

vsFog::vsFog():
	m_fogColor( vsColor::Black ),
	m_density(0.f),
	m_start(0.f),
	m_end(0.f),
	m_linear(true)
{
}

void
vsFog::SetLinear( const vsColor &color, float start, float end )
{
	m_linear = true;
	m_fogColor = color;
	m_start = start;
	m_end = end;
}

void
vsFog::SetExponential( const vsColor &color, float density )
{
	m_linear = false;
	m_fogColor = color;
	m_density = density;
}