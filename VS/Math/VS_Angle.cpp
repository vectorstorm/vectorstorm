/*
 *  VS_Angle.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Angle.h"
#include "VS_Vector.h"

//#include <math.h>

vsAngle vsAngle::Zero(0.0f);

vsAngle::vsAngle(float angle_in):
	m_angle(0.f),
	m_cosValue(0.f),
	m_sinValue(0.f),
	m_trigCalculated(false)
{
	Set(angle_in);
}

vsAngle::vsAngle(const vsAngle& o):
	m_angle(o.m_angle),
	m_cosValue(0.f),
	m_sinValue(0.f),
	m_trigCalculated(false)
{
}

vsAngle
vsAngle::FromForwardVector( const vsVector2D &forward )
{
	float angle = vsATan2(forward.x, -forward.y);

	return vsAngle(angle);
}


void
vsAngle::Set(float angle_in)
{
	// our angle ranges from [-pi .. pi), so make sure we're in that range!

	if ( angle_in >= PI || angle_in < -PI )
	{
		int wrapsAround = int(angle_in / TWOPI);
		if ( angle_in >= PI )
			wrapsAround++;
		else
			wrapsAround--;
		angle_in -= wrapsAround * TWOPI;
	}

	if ( m_angle != angle_in )
	{
		m_angle = angle_in;
		m_trigCalculated = false;
	}
}

void
vsAngle::Rotate(float angle_in)
{
	Set( m_angle + angle_in );
}

void
vsAngle::CalculateTrig() const
{
	m_cosValue = vsCos(m_angle);
	m_sinValue = vsSin(m_angle);
	m_trigCalculated = true;
}

float
vsAngle::Cos() const
{
	if ( !m_trigCalculated )
		CalculateTrig();

	return m_cosValue;
}

float
vsAngle::Sin() const
{
	if ( !m_trigCalculated )
		CalculateTrig();

	return m_sinValue;
}

vsVector2D
vsAngle::ApplyRotationTo( const vsVector2D &in ) const
{
	vsVector2D result;

	float c = Cos();
	float s = Sin();

	result.x = (c * in.x) - (s * in.y);		// check this math
	result.y = (c * in.y) + (s * in.x);

	return result;
}

vsAngle operator-(const vsAngle &ang) { return vsAngle(-ang.Get()); }

vsAngle vsInterpolate( float alpha, const vsAngle &a, const vsAngle &b )
{
	// find how far we're going from 'a' to 'b'

	vsAngle delta = b-a;

	return vsAngle(a.Get() + (delta.Get()*alpha));
}
