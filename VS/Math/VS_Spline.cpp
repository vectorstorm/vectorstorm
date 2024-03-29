/*
 *  VS_Spline.cpp
 *  MMORPG
 *
 *  Created by Trevor Powell on 12/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Spline.h"

vsSpline1D::vsSpline1D()
{
	Set(0,0,0,0);
}

vsSpline1D::vsSpline1D( float start, float startVelocity, float end, float endVelocity )
{
	Set( start, startVelocity, end, endVelocity );
}

void
vsSpline1D::Set( float start, float startVelocity, float end, float endVelocity )
{
	m_start = start;
	m_startVelocity = startVelocity;
	m_end = end;
	m_endVelocity = endVelocity;
}

float
vsSpline1D::PositionAtTime( float t ) const
{
	float tSquared = t * t;
	float tCubed = tSquared * t;

	float a = 2.f * tCubed - 3.f * tSquared + 1.f;	// 2t^3 - 3t^2 + 1
	float b = tCubed - 2.f * tSquared + t;			// t^3 - 2t^2 + t
	float c = -2.f * tCubed + 3.f * tSquared;		// -2t^3 + 3t^2
	float d = tCubed - tSquared;					// t^3 - t^2

	float result =
		(a * m_start) +
		(b * m_startVelocity) +
		(c * m_end) +
		(d * m_endVelocity);

	return result;
}

float
vsSpline1D::VelocityAtTime( float t ) const
{
	float tSquared = t * t;

	float a = 6.f * tSquared - 6.f * t;				// 6t^2 - 6t
	float b = 3.f * tSquared - 4.f * t + 1;			// 3t^2 - 4t + 1
	float c = -6.f * tSquared + 6.f * t;			// -6t^2 + 6t
	float d = 3.f * tSquared - 2.f * t;				// 3t^2 - 2t

	float result =
		(a * m_start) +
		(b * m_startVelocity) +
		(c * m_end) +
		(d * m_endVelocity);

	return result;
}

vsSpline1D
vsSpline1D::Slice( float t1, float t2 ) const
{
	vsAssertF( t1 >= 0.f && t1 <= 1.f, "t1 value of %f is outside the range [0..1]!", t1 );
	vsAssertF( t2 >= 0.f && t2 <= 1.f, "t2 value of %f is outside the range [0..1]!", t1 );

	float newDuration = (t2 - t1);
	float velocityConversion = newDuration;

	vsSpline1D segment(
			PositionAtTime(t1), VelocityAtTime(t1) * velocityConversion,
			PositionAtTime(t2), VelocityAtTime(t2) * velocityConversion
			);
	return segment;
}

vsSpline2D::vsSpline2D()
{
	Set( vsVector2D::Zero, vsVector2D::Zero, vsVector2D::Zero, vsVector2D::Zero );
}

vsSpline2D::vsSpline2D( const vsVector2D &start, const vsVector2D &startVelocity, const vsVector2D &end, const vsVector2D &endVelocity )
{
	Set( start, startVelocity, end, endVelocity );
}

void
vsSpline2D::Set( const vsVector2D &start, const vsVector2D &startVelocity, const vsVector2D &end, const vsVector2D &endVelocity )
{
	m_start = start;
	m_startVelocity = startVelocity;
	m_end = end;
	m_endVelocity = endVelocity;
}

vsVector2D
vsSpline2D::PositionAtTime( float t ) const
{
	float tSquared = t * t;
	float tCubed = tSquared * t;

	float a = 2.f * tCubed - 3.f * tSquared + 1.f;	// 2t^3 - 3t^2 + 1
	float b = tCubed - 2.f * tSquared + t;			// t^3 - 2t^2 + t
	float c = -2.f * tCubed + 3.f * tSquared;		// -2t^3 + 3t^2
	float d = tCubed - tSquared;					// t^3 - t^2

	vsVector2D result =
		(a * m_start) +
		(b * m_startVelocity) +
		(c * m_end) +
		(d * m_endVelocity);

	return result;
}

vsVector2D
vsSpline2D::VelocityAtTime( float t ) const
{
	float tSquared = t * t;

	float a = 6.f * tSquared - 6.f * t;				// 6t^2 - 6t
	float b = 3.f * tSquared - 4.f * t + 1;			// 3t^2 - 4t + 1
	float c = -6.f * tSquared + 6.f * t;			// -6t^2 + 6t
	float d = 3.f * tSquared - 2.f * t;				// 3t^2 - 2t

	vsVector2D result =
		(a * m_start) +
		(b * m_startVelocity) +
		(c * m_end) +
		(d * m_endVelocity);

	return result;
}

vsSpline2D
vsSpline2D::Slice( float t1, float t2 ) const
{
	vsAssertF( t1 >= 0.f && t1 <= 1.f, "t1 value of %f is outside the range [0..1]!", t1 );
	vsAssertF( t2 >= 0.f && t2 <= 1.f, "t2 value of %f is outside the range [0..1]!", t1 );

	float newDuration = (t2 - t1);
	float velocityConversion = newDuration;

	vsSpline2D segment(
			PositionAtTime(t1), VelocityAtTime(t1) * velocityConversion,
			PositionAtTime(t2), VelocityAtTime(t2) * velocityConversion
			);
	return segment;
}

vsSpline3D::vsSpline3D()
{
	Set( vsVector3D::Zero, vsVector3D::Zero, vsVector3D::Zero, vsVector3D::Zero );
}

vsSpline3D::vsSpline3D( const vsVector3D &start, const vsVector3D &startVelocity, const vsVector3D &end, const vsVector3D &endVelocity )
{
	Set( start, startVelocity, end, endVelocity );
}

void
vsSpline3D::Set( const vsVector3D &start, const vsVector3D &startVelocity, const vsVector3D &end, const vsVector3D &endVelocity )
{
	m_start = start;
	m_startVelocity = startVelocity;
	m_end = end;
	m_endVelocity = endVelocity;
}

vsVector3D
vsSpline3D::PositionAtTime( float t ) const
{
	float tSquared = t * t;
	float tCubed = tSquared * t;

	float a = 2.f * tCubed - 3.f * tSquared + 1.f;	// 2t^3 - 3t^2 + 1
	float b = tCubed - 2.f * tSquared + t;			// t^3 - 2t^2 + t
	float c = -2.f * tCubed + 3.f * tSquared;		// -2t^3 + 3t^2
	float d = tCubed - tSquared;					// t^3 - t^2

	vsVector3D result = (a * m_start) +
	(b * m_startVelocity) +
	(c * m_end) +
	(d * m_endVelocity);

	return result;
}

vsVector3D
vsSpline3D::VelocityAtTime( float t ) const
{
	float tSquared = t * t;

	float a = 6.f * tSquared - 6.f * t;				// 6t^2 - 6t
	float b = 3.f * tSquared - 4.f * t + 1;			// 3t^2 - 4t + 1
	float c = -6.f * tSquared + 6.f * t;			// -6t^2 + 6t
	float d = 3.f * tSquared - 2.f * t;				// 3t^2 - 2t

	vsVector3D result = (a * m_start) +
	(b * m_startVelocity) +
	(c * m_end) +
	(d * m_endVelocity);

	return result;
}

float
vsSpline3D::ClosestTimeTo( const vsVector3D& position ) const
{
	//lastBestT is a composite value;
	//the whole part is the "before" knot, and the fraction
	//is the 't' for that segment.

	float lastBestT = 0.5f;
	float lastMove = 1.f;
	float scale = 0.75f;
	vsVector3D lastBestPoint;
	vsVector3D lastBestTangent;
	// int iterations = 0;

	while ( lastMove > 0.0001f )
	{
		lastBestPoint = PositionAtTime(lastBestT);
		lastBestTangent = VelocityAtTime(lastBestT);

		vsVector3D delta = position - lastBestPoint;
		float move = lastBestTangent.Dot(delta) / lastBestTangent.SqLength();

		move = vsClamp(move, -lastMove*scale, lastMove*scale);
		lastBestT = vsClamp( lastBestT+move, 0.f, 1.f );
		lastMove = vsFabs(move);
		// iterations++;
	}
	return lastBestT;
}
vsVector3D
vsSpline3D::ClosestPointTo( const vsVector3D& position ) const
{
	return PositionAtTime( ClosestTimeTo(position) );
}

float
vsSpline3D::Length() const
{
	// let's take 100 samples, and take the linear distance.
	vsVector3D cursor = m_start;
	float distance = 0.f;
	const int c_count = 100;
	for ( int i = 1; i < c_count; i++ )
	{
		float t = i / (float)c_count;
		vsVector3D next = PositionAtTime(t);
		distance += (cursor - next).Length();
		cursor = next;
	}
	return distance;
}

float
vsSpline3D::TimeAtLength(float target) const
{
	// let's take 100 samples, and take the linear distance.
	float distance = 0.f;
	float timeCursor = 0.f;
	vsVector3D cursor = m_start;
	while ( timeCursor < 1.f )
	{
		float nextTime = timeCursor + 0.01f;
		vsVector3D next = PositionAtTime(nextTime);
		float thisDistance = (cursor - next).Length();

		if ( distance + thisDistance > target )
		{
			// we've gone too far!  Interpolate.
			float fraction = vsProgressFraction( target, distance, distance + thisDistance );
			timeCursor = vsInterpolate( fraction, timeCursor, nextTime );
			break;
		}
		distance += thisDistance;
		cursor = next;
		timeCursor = nextTime;
	}
	return timeCursor;
}

vsSpline3D
vsSpline3D::Slice( float t1, float t2 ) const
{
	vsAssertF( t1 >= 0.f && t1 <= 1.f, "t1 value of %f is outside the range [0..1]!", t1 );
	vsAssertF( t2 >= 0.f && t2 <= 1.f, "t2 value of %f is outside the range [0..1]!", t1 );

	float newDuration = (t2 - t1);
	float velocityConversion = newDuration;

	vsSpline3D segment(
			PositionAtTime(t1), VelocityAtTime(t1) * velocityConversion,
			PositionAtTime(t2), VelocityAtTime(t2) * velocityConversion
			);
	return segment;
}

vsSplineColor::vsSplineColor()
{
	Set( c_white, c_white, c_white );
}

vsSplineColor::vsSplineColor( const vsColor &start, const vsColor &middle, const vsColor &end )
{
	Set( start, middle, end );
}

void
vsSplineColor::Set( const vsColor &start, const vsColor &middle, const vsColor &end )
{
	m_start = start;
	m_startVelocity = middle - start;
	m_end = end;
	m_endVelocity = end - middle;
}

vsColor
vsSplineColor::ColorAtTime( float t ) const
{
	float tSquared = t * t;
	float tCubed = tSquared * t;

	float a = 2.f * tCubed - 3.f * tSquared + 1.f;	// 2t^3 - 3t^2 + 1
	float b = tCubed - 2.f * tSquared + t;			// t^3 - 2t^2 + t
	float c = -2.f * tCubed + 3.f * tSquared;		// -2t^3 + 3t^2
	float d = tCubed - tSquared;					// t^3 - t^2

	vsColor result = (a * m_start) +
	(b * m_startVelocity) +
	(c * m_end) +
	(d * m_endVelocity);

	return result;
}

vsColor
vsSplineColor::VelocityAtTime( float t ) const
{
	float tSquared = t * t;

	float a = 6.f * tSquared - 6.f * t;				// 6t^2 - 6t
	float b = 3.f * tSquared - 4.f * t + 1;			// 3t^2 - 4t + 1
	float c = -6.f * tSquared + 6.f * t;			// -6t^2 + 6t
	float d = 3.f * tSquared - 2.f * t;				// 3t^2 - 2t

	vsColor result = (a * m_start) +
	(b * m_startVelocity) +
	(c * m_end) +
	(d * m_endVelocity);

	return result;
}

vsSplineColor
vsSplineColor::Slice( float t1, float t2 ) const
{
	vsAssertF( t1 >= 0.f && t1 <= 1.f, "t1 value of %f is outside the range [0..1]!", t1 );
	vsAssertF( t2 >= 0.f && t2 <= 1.f, "t2 value of %f is outside the range [0..1]!", t1 );

	float newDuration = (t2 - t1);
	float velocityConversion = newDuration;

	vsSplineColor segment;
	segment.m_start = ColorAtTime(t1);
	segment.m_startVelocity = VelocityAtTime(t1) * velocityConversion;
	segment.m_end = ColorAtTime(t2);
	segment.m_endVelocity = VelocityAtTime(t2) * velocityConversion;

	return segment;
}

