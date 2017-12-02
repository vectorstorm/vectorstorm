/*
 *  VS_Span.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 02/12/2017
 *  Copyright 2017 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SPAN_H
#define VS_SPAN_H

// A span is basically a 1-dimensional "box", following a similar interface
// to vsBox2D and vsBox3D.  Useful if you just want to push a bunch of values
// in and let it figure out the min and max values.

class vsSpan
{
	bool		set;

	float min;
	float max;

public:

	vsSpan() { min = max = 0.f; set = false; }
	vsSpan(const vsSpan &o) { *this = o; }
	vsSpan(float min_in, float max_in) { Set(min_in,max_in); }

	void Clear() { set = false; min = max = 0.f; }

	float	GetMin() const { return min; }
	float	GetMax() const { return max; }

	float		Length() const { return max - min; }
	float		Middle() const { return (min + max) * 0.5f; }
	float		Extents() const { return Length(); }

	float	Min() const { return min; }
	float	Max() const { return max; }
	void		Expand( float amt ) { min += -amt; max += amt; }
	void		Contract( float amt ) { Expand(-amt); }	// TODO:  Make sure we don't invert ourselves.

	void		Union( const vsSpan& other ); // sets me to the union of my span and that span.
	void		Intersect( const vsSpan& other ); // sets me to the intersection of my span and that span.

	void Set(float min_in, float max_in) { min = min_in; max = max_in; set = true; }

	bool Contains(float pos) const;
	bool Intersects(const vsSpan &other) const;

	void ExpandToInclude( float pos );
	void ExpandToInclude( const vsSpan& other );

	bool operator==(const vsSpan &b) const { return( min==b.min && max==b.max ); }
	bool operator!=(const vsSpan &b) const { return( min!=b.min || max!=b.max ); }
	vsSpan operator+(float v) const { return vsSpan(min+v, max+v); }
	vsSpan operator-(float v) const { return vsSpan(min-v, max-v); }
	vsSpan operator*(float f) const { return vsSpan(min*f, max*f); }

	vsSpan& operator+=(float v) { min+=v; max+=v; return *this; }
	vsSpan& operator-=(float v) { min-=v; max-=v; return *this; }
	vsSpan& operator*=(float f) { min*=f; max*=f; return *this; }
};

#endif // VS_SPAN_H

