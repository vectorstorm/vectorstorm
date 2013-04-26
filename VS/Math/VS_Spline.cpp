/*
 *  VS_Spline.cpp
 *  MMORPG
 *
 *  Created by Trevor Powell on 12/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Spline.h"

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
vsSpline2D::PositionAtTime( float t )
{
	float tSquared = t * t;
	float tCubed = tSquared * t;

	float a = 2.f * tCubed - 3.f * tSquared + 1.f;	// 2t^3 - 3t^2 + 1
	float b = tCubed - 2.f * tSquared + t;			// t^3 - 2t^2 + t
	float c = -2.f * tCubed + 3.f * tSquared;		// -2t^3 + 3t^2
	float d = tCubed - tSquared;					// t^3 - t^2

	vsVector2D result = (a * m_start) +
						(b * m_startVelocity) +
						(c * m_end) +
						(d * m_endVelocity);

	return result;
}

vsVector2D
vsSpline2D::VelocityAtTime( float t )
{
	float tSquared = t * t;

	float a = 6.f * tSquared - 6.f * t;				// 6t^2 - 6t
	float b = 3.f * tSquared - 4.f * t + 1;			// 3t^2 - 4t + 1
	float c = -6.f * tSquared + 6.f * t;			// -6t^2 + 6t
	float d = 3.f * tSquared - 2.f * t;				// 3t^2 - 2t

	vsVector2D result = (a * m_start) +
	(b * m_startVelocity) +
	(c * m_end) +
	(d * m_endVelocity);

	return result;
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

vsVector3D
vsSpline3D::ClosestPointTo( const vsVector3D& position )
{
	//lastBestT is a composite value;
	//the whole part is the "before" knot, and the fraction
	//is the 't' for that segment.

	float lastBestT = 0.5f;
	float lastMove = 1.f;
	float scale = 0.75f;
	vsVector3D lastBestPoint;
	vsVector3D lastBestTangent;
	int iterations = 0;

	while ( lastMove > 0.0001f )
	{
		lastBestPoint = PositionAtTime(lastBestT);
		lastBestTangent = VelocityAtTime(lastBestT);

		vsVector3D delta = position - lastBestPoint;
		float move = lastBestTangent.Dot(delta) / lastBestTangent.SqLength();

		move = vsClamp(move, -lastMove*scale, lastMove*scale);
		lastBestT += move;
		lastMove = vsFabs(move);
		iterations++;
	}
	return lastBestPoint;
}

vsSplineColor::vsSplineColor()
{
	Set( vsColor::PureWhite, vsColor::PureWhite, vsColor::PureWhite );
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
