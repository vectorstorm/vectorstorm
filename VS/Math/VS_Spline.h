/*
 *  VS_Spline.h
 *  MMORPG
 *
 *  Created by Trevor Powell on 12/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SPLINE_H
#define VS_SPLINE_H

#include "VS/Math/VS_Vector.h"
#include "VS/Graphics/VS_Color.h"

class vsSpline2D
{
	vsVector2D	m_start;
	vsVector2D	m_startVelocity;
	vsVector2D	m_end;
	vsVector2D	m_endVelocity;

public:

	vsSpline2D();
	vsSpline2D( const vsVector2D &start, const vsVector2D &startVelocity, const vsVector2D &end, const vsVector2D &endVelocity );

	void Set( const vsVector2D &start, const vsVector2D &startVelocity, const vsVector2D &end, const vsVector2D &endVelocity );

		// evaluates spline position at time 't'.
		// 't' must be in the range [0..1] from the start to the end of the spline
    const vsVector2D & GetStart() const { return m_start; }
    const vsVector2D & GetEnd() const { return m_end; }
    const vsVector2D & GetStartVelocity() const { return m_startVelocity; }
    const vsVector2D & GetEndVelocity() const { return m_endVelocity; }

	vsVector2D	PositionAtTime( float t );
	vsVector2D	VelocityAtTime( float t );		// TODO:  IMPLEMENT THIS!  It will be important later!

	bool operator==(const vsSpline2D& b) const
	{
		return ( m_start == b.m_start &&
				m_startVelocity == b.m_startVelocity &&
				m_end == b.m_end &&
				m_endVelocity == b.m_endVelocity );
	}
	bool operator!=(const vsSpline2D& b) const
	{
		return ( m_start != b.m_start ||
				m_startVelocity != b.m_startVelocity ||
				m_end != b.m_end ||
				m_endVelocity != b.m_endVelocity );
	}
};

class vsSpline3D
{
	vsVector3D	m_start;
	vsVector3D	m_startVelocity;
	vsVector3D	m_end;
	vsVector3D	m_endVelocity;

public:

	vsSpline3D();
	vsSpline3D( const vsVector3D &start, const vsVector3D &startVelocity, const vsVector3D &end, const vsVector3D &endVelocity );

	void Set( const vsVector3D &start, const vsVector3D &startVelocity, const vsVector3D &end, const vsVector3D &endVelocity );

	// evaluates spline position at time 't'.
	// 't' must be in the range [0..1] from the start to the end of the spline
    const vsVector3D & GetStart() const { return m_start; }
    const vsVector3D & GetEnd() const { return m_end; }
    const vsVector3D & GetStartVelocity() const { return m_startVelocity; }
    const vsVector3D & GetEndVelocity() const { return m_endVelocity; }

	vsVector3D PositionAtTime( float t ) const;
	vsVector3D VelocityAtTime( float t ) const;

	float Length();
	float TimeAtLength(float distance);

	// Note:  In general, use ClosestTimeTo in preference to ClosestPointTo,
	// as these are expensive functions, and once you have the time you can
	// get the position or whatever else you want.
	float ClosestTimeTo( const vsVector3D& position );
	vsVector3D ClosestPointTo( const vsVector3D& position );

	bool operator==(const vsSpline3D& b) const
	{
		return ( m_start == b.m_start &&
				m_startVelocity == b.m_startVelocity &&
				m_end == b.m_end &&
				m_endVelocity == b.m_endVelocity );
	}
	bool operator!=(const vsSpline3D& b) const
	{
		return ( m_start != b.m_start ||
				m_startVelocity != b.m_startVelocity ||
				m_end != b.m_end ||
				m_endVelocity != b.m_endVelocity );
	}
};


class vsSplineColor
{
	vsColor		m_start;
	vsColor		m_startVelocity;
	vsColor		m_end;
	vsColor		m_endVelocity;

public:

	vsSplineColor();
	vsSplineColor( const vsColor &start, const vsColor &middle, const vsColor &end );

	void Set( const vsColor &start, const vsColor &middle, const vsColor &end );

	const vsColor& Start() const { return m_start; }
	const vsColor& End() const { return m_end; }
	vsColor Middle() const { return m_start + m_startVelocity; }

	// evaluates spline position at time 't'.
	// 't' must be in the range [0..1] from the start to the end of the spline

	vsColor	ColorAtTime( float t ) const;
	vsColor	VelocityAtTime( float t ) const;

	bool operator==(const vsSplineColor& b) const
	{
		return ( m_start == b.m_start &&
				m_startVelocity == b.m_startVelocity &&
				m_end == b.m_end &&
				m_endVelocity == b.m_endVelocity );
	}
	bool operator!=(const vsSplineColor& b) const
	{
		return ( m_start != b.m_start ||
				m_startVelocity != b.m_startVelocity ||
				m_end != b.m_end ||
				m_endVelocity != b.m_endVelocity );
	}
};


#endif // VS_SPLINE_H

