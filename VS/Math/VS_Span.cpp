/*
 *  VS_Span.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 02/12/2017
 *  Copyright 2017 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Span.h"

void
vsSpan::Union( const vsSpan& other )
{
	ExpandToInclude(other);
}

void
vsSpan::Intersect( const vsSpan& other )
{
	if ( !Intersects(other) )
	{
		min = max = 0.f;
		set = false;
	}
	else
	{
		min = vsMax( min, other.min );
		max = vsMin( max, other.max );
	}
}

bool
vsSpan::Contains( float pos ) const
{
	if ( pos >= min && pos <= max )
	{
		return set;
	}
	return false;
}

bool
vsSpan::Intersects(const vsSpan &other) const
{
	if ( min >= other.max || min >= other.max )
	{
		return false;
	}

	return true;
}

void
vsSpan::ExpandToInclude( float pos )
{
	if ( !set )
	{
		Set(pos,pos);
	}
	else
	{
		min = vsMin( min, pos );
		max = vsMax( max, pos );
	}
}

void
vsSpan::ExpandToInclude( const vsSpan &other )
{
	if ( !set )
		*this = other;
	else if ( other.set )
	{
		ExpandToInclude( other.min );
		ExpandToInclude( other.max );
	}
}

