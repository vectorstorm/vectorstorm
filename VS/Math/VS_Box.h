/*
 *  VS_Box.h
 *  Lord
 *
 *  Created by Trevor Powell on 9/02/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_BOX_H
#define VS_BOX_H

#include "VS/Math/VS_Vector.h"
#include "VS/Math/VS_Transform.h"

class vsDisplayList;

class vsBox2D
{
	bool		set;

	vsVector2D min;
	vsVector2D max;

public:

	vsBox2D() { min = max = vsVector2D::Zero; set = false; }
	vsBox2D(const vsBox2D &o) { *this = o; }
	vsBox2D(const vsVector2D &min_in, const vsVector2D &max_in) { Set(min_in,max_in); }

	static vsBox2D CenteredBox( const vsVector2D &dimensions ) { vsBox2D result(vsVector2D::Zero, dimensions); result -= 0.5f * dimensions; return result; }

	const vsVector2D &	GetMin() const { return min; }
	const vsVector2D &	GetMax() const { return max; }

	float		Width() const { return max.x - min.x; }
	float		Height() const { return max.y - min.y; }
	vsVector2D	Middle() const { return (min + max) * 0.5f; }

	vsVector2D	TopLeft() const { return min; }
	vsVector2D	TopRight() const { return vsVector2D(max.x, min.y); }
	vsVector2D	BottomLeft() const { return vsVector2D(min.x, max.y); }
	vsVector2D	BottomRight() const { return max; }
	vsVector2D	Corner(int i) const;
	void		Expand( float amt ) { min += -amt * vsVector2D::One; max += amt * vsVector2D::One; }
	void		Contract( float amt ) { Expand(-amt); }	// TODO:  Make sure we don't invert ourselves.

	void Set(const vsVector2D &min_in, const vsVector2D &max_in) { min = min_in; max = max_in; set = true; }

	bool ContainsPoint(const vsVector2D &pos) const;	// 'pos' must be in local coordinates!
	bool Intersects(const vsBox2D &other) const;

	void ExpandToInclude( const vsVector2D &pos );
	void ExpandToInclude( const vsBox2D &box );

	bool operator==(const vsBox2D &b) const { return( min==b.min && max==b.max ); }
	bool operator!=(const vsBox2D &b) const { return( min!=b.min || max!=b.max ); }
	vsBox2D operator+(const vsVector2D &v) const { return vsBox2D(min+v, max+v); }
	vsBox2D operator-(const vsVector2D &v) const { return vsBox2D(min-v, max-v); }
	vsBox2D operator*(float f) const { return vsBox2D(min*f, max*f); }

	vsBox2D& operator+=(const vsVector2D &v) { min+=v; max+=v; return *this; }
	vsBox2D& operator-=(const vsVector2D &v) { min-=v; max-=v; return *this; }
	vsBox2D& operator*=(float f) { min*=f; max*=f; return *this; }
};

vsBox2D vsInterpolate( float alpha, const vsBox2D& a, const vsBox2D& b );

class vsBox3D
{
	bool		set;

	vsVector3D	min;
	vsVector3D	max;

public:

				vsBox3D() { min = max = vsVector2D::Zero; set = false; }
				vsBox3D(const vsBox3D &o) { *this = o; set = true; }
				vsBox3D(const vsVector3D &min_in, const vsVector3D &max_in) { Set(min_in,max_in); set = true; }

	static vsBox3D CenteredBox( const vsVector3D &dimensions ) { vsBox3D result(vsVector3D::Zero, dimensions); result -= 0.5f * dimensions; return result; }

	void Clear() { set = false; min = max = vsVector3D::Zero; }

	const vsVector3D &	GetMin() const { return min; }
	const vsVector3D &	GetMax() const { return max; }

	void		DrawOutline( vsDisplayList *list );

	void		Set(const vsVector3D &min_in, const vsVector3D &max_in) { min = min_in; max = max_in; set = true; }
	void		Set(vsVector3D *pointArray, int pointCount);

	bool		ContainsPoint(const vsVector3D &pos) const;
	bool		ContainsPointXZ(const vsVector3D &pos) const;	// only tests x and z components.
	bool		ContainsRay(const vsVector3D &pos, const vsVector3D &dir) const;	// returns true if any portion of the passed ray is included inside this bounding box
	bool		Intersects(const vsBox3D &other) const;
	bool		IntersectsXZ(const vsBox3D &other) const;
	bool		IntersectsSphere(const vsVector3D &center, float radius) const;

	bool		EncompassesBox(const vsBox3D &box) const;

	bool		CollideRay(vsVector3D *result, float *resultT, const vsVector3D &pos, const vsVector3D &dir) const;

	void		ExpandToInclude( const vsVector3D &pos );
	void		ExpandToInclude( const vsBox3D &box );

	float		DistanceFrom( const vsVector3D &pos ) const;
	float		SqDistanceFrom( const vsVector3D &pos ) const;
	vsVector3D	OffsetFrom( const vsVector3D &pos ) const;

	float		Width() const { return max.x - min.x; }
	float		Height() const { return max.y - min.y; }
	float		Depth() const { return max.z - min.z; }
	vsVector3D	Middle() const { return (min + max) * 0.5f; }
	vsVector3D	Extents() const { return max - min; }

	vsVector3D	Corner(int i) const;

	void		Expand( float amt ) { min += -amt * vsVector3D::One; max += amt * vsVector3D::One; }
	void		Expand( const vsVector3D &amt ) { min -= amt; max += amt; }
	void		Contract( float amt ) { Expand(-amt); }	// TODO:  Make sure we don't invert ourselves.

	bool		operator==(const vsBox3D &b) const { return( min==b.min && max==b.max ); }
	bool		operator!=(const vsBox3D &b) const { return( min!=b.min || max!=b.max ); }

	vsBox3D		operator+(const vsVector3D &b) const { return vsBox3D(min+b, max+b); }
	vsBox3D &	operator+=(const vsVector3D &b) { min += b; max += b; return *this; }

	vsBox3D		operator-(const vsVector3D &b) const { return vsBox3D(min-b, max-b); }
	vsBox3D &	operator-=(const vsVector3D &b) { min -= b; max -= b; return *this; }

	vsBox3D		operator*(const float b) const { return vsBox3D(min*b, max*b); }
	vsBox3D &	operator*=(const float b) { min *= b; max *= b; return *this; }
};

class vsOrientedBox3D
{
	vsBox3D m_box;
	vsTransform3D m_transform;
	// As an optimisation, we're going to apply the orientation store the
	// vertices directly, here.  Note that our intersection test is customised
	// for boxes -- this isn't a generalised convex hull collision test
	// implementation!  (Although it shouldn't be too hard to write one based on
	// this general approach.  Might do that someday if/when I need one)
	vsVector3D m_corner[8];

	// run a single Separating Axis Theorem test, along the proposed axis
	bool SAT_Intersects( const vsOrientedBox3D& other, const vsVector3D& axis );
	bool SAT_Intersects( const vsVector3D* points, int pountCount, const vsVector3D& axis, float otherRadius = 0.f );

	// run a single test for containment, along the proposed axis
	bool SAT_Contains( const vsOrientedBox3D& other, const vsVector3D& axis );
	bool SAT_Contains( const vsVector3D* points, int pountCount, const vsVector3D& axis, float otherRadius = 0.f );
public:
	vsOrientedBox3D();
	vsOrientedBox3D( const vsBox3D& box, const vsTransform3D& transform );

	const vsVector3D& Corner(int i) const { return m_corner[i]; }

	bool Contains( const vsOrientedBox3D& other );
	bool Intersects( const vsOrientedBox3D& other );
	bool IntersectsLineStrip( const vsVector3D* point, int pointCount, float radius );
	bool IntersectsLineSegment( const vsVector3D& a, const vsVector3D& b, float radius );
	bool IntersectsSphere( const vsVector3D& point, float radius );
	bool ContainsPoint( const vsVector3D& point );
};

vsBox3D vsInterpolate( float alpha, const vsBox3D& a, const vsBox3D& b );



#endif // VS_BOX_H

