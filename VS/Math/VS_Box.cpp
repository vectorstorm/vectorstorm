/*
 *  VS_Box.cpp
 *  Lord
 *
 *  Created by Trevor Powell on 9/02/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Box.h"

#include "VS_DisplayList.h"

bool
vsBox2D::Intersects(const vsBox2D &other) const
{
	if ( min.x >= other.max.x || min.y >= other.max.y ||
		max.x <= other.min.x || max.y <= other.min.y )
	{
		return false;
	}

	return true;
}

vsVector2D
vsBox2D::Corner(int i) const
{
	switch(i)
	{
		case 0:
			return min;
		case 1:
			return vsVector2D(max.x, min.y);
		case 2:
			return vsVector2D(min.x, max.y);
		case 3:
			return max;
		default:
			break;
	}
	vsAssert(0, "Illegal corner?");
	return vsVector2D::Zero;
}

bool
vsBox2D::ContainsPoint( const vsVector2D &pos ) const
{
	if ( pos.x >= min.x && pos.x <= max.x &&
		pos.y >= min.y && pos.y <= max.y )
	{
		return set;
	}
	return false;
}

void
vsBox2D::ExpandToInclude( const vsVector2D &pos )
{
	if ( !set )
	{
		Set(pos,pos);
	}
	else
	{
		min.x = vsMin( min.x, pos.x );
		min.y = vsMin( min.y, pos.y );

		max.x = vsMax( max.x, pos.x );
		max.y = vsMax( max.y, pos.y );
	}
}

void
vsBox2D::ExpandToInclude( const vsBox2D &other )
{
	ExpandToInclude( other.min );
	ExpandToInclude( other.max );
}

void
vsBox2D::Union( const vsBox2D& other )
{
	ExpandToInclude(other);
}

void
vsBox2D::Intersect( const vsBox2D& other )
{
	if ( !Intersects(other) )
		min = max = vsVector2D::Zero;
	else
	{
		min.x = vsMax( min.x, other.min.x );
		min.y = vsMax( min.y, other.min.y );
		max.x = vsMin( max.x, other.max.x );
		max.y = vsMin( max.y, other.max.y );
	}
}


vsBox2D vsInterpolate( float alpha, const vsBox2D& a, const vsBox2D& b )
{
	return vsBox2D( vsInterpolate( alpha, a.GetMin(), b.GetMin() ), vsInterpolate( alpha, a.GetMax(), b.GetMax() ) );
}

bool
vsBox3D::Intersects(const vsBox3D &other) const
{
	if ( min.x >= other.max.x || max.x <= other.min.x ||
		min.y >= other.max.y ||max.y <= other.min.y ||
		min.z >= other.max.z ||max.z <= other.min.z )
	{
		return false;
	}

	return true;
}

bool
vsBox3D::IntersectsXZ(const vsBox3D &other) const
{
	if ( min.x >= other.max.x || max.x <= other.min.x ||
		min.z >= other.max.z ||max.z <= other.min.z )
	{
		return false;
	}

	return true;
}

bool
vsBox3D::IntersectsSphere(const vsVector3D &center, float radius) const
{
	if ( ContainsPoint( center ) )
		return true;

	vsVector3D closestPoint;
	closestPoint.x = vsClamp( center.x, min.x, max.x );
	closestPoint.y = vsClamp( center.y, min.y, max.y );
	closestPoint.z = vsClamp( center.z, min.z, max.z );

	vsVector3D delta = center - closestPoint;
	if ( delta.SqLength() < radius * radius )
		return true;

	return false;
}

vsVector3D
vsBox3D::Corner(int i) const
{
	vsVector3D result;
	if ( i & 0x1 )
		result.x = min.x;
	else
		result.x = max.x;
	if ( i & 0x2 )
		result.y = min.y;
	else
		result.y = max.y;
	if ( i & 0x4 )
		result.z = min.z;
	else
		result.z = max.z;

	return result;
}


void
vsBox3D::DrawOutline( vsDisplayList *list )
{
	vsVector3D corners[8] = {
		vsVector3D(min.x,min.y,min.z),
		vsVector3D(min.x,min.y,max.z),
		vsVector3D(max.x,min.y,min.z),
		vsVector3D(max.x,min.y,max.z),
		vsVector3D(min.x,max.y,min.z),
		vsVector3D(min.x,max.y,max.z),
		vsVector3D(max.x,max.y,min.z),
		vsVector3D(max.x,max.y,max.z)
	};
	int array[24] = {
		0, 1,
		1, 3,
		2, 3,
		2, 0,

		4, 5,
		5, 7,
		6, 7,
		6, 4,

		0, 4,
		1, 5,
		2, 6,
		3, 7
	};

	list->VertexArray(corners,8);
	list->LineListArray(array,24);
}

void
vsBox3D::Set(vsVector3D *pointArray, int pointCount)
{
	if ( pointCount == 0 )
	{
		Set( vsVector3D::Zero, vsVector3D::Zero );
		set = false;
	}
	else
	{
		Set( pointArray[0], pointArray[0] );

		for ( int i = 0; i < pointCount; i++ )
		{
			ExpandToInclude( pointArray[i] );
		}
	}
}

bool
vsBox3D::ContainsPoint( const vsVector3D &pos ) const
{
	if ( pos.x >= min.x && pos.x <= max.x &&
		pos.y >= min.y && pos.y <= max.y &&
		pos.z >= min.z && pos.z <= max.z )
	{
		return set;
	}
	return false;
}

bool
vsBox3D::ContainsPointXZ( const vsVector3D &pos ) const
{
	if ( pos.x >= min.x && pos.x <= max.x &&
		pos.z >= min.z && pos.z <= max.z )
	{
		return set;
	}
	return false;
}


bool
vsBox3D::EncompassesBox(const vsBox3D &box) const
{
	return ContainsPoint(box.min) && ContainsPoint(box.max);
}

bool
vsBox3D::ContainsRay( const vsVector3D &pos, const vsVector3D &dir ) const
{
	// handle the easy case;  starting point is inside this box.
	if ( ContainsPoint(pos) )
	{
		return true;
	}
	// we now know that the ray does not begin inside the box.  So let's take
	// the vector from the start point to the centroid of the box, and see if
	// 'dir' is going in even vaguely the right direction.

	/*vsVector3D center = min + max * 0.5f;
	vsVector3D rayStartToCenter = center - pos;

	if ( rayStartToCenter.Dot( dir ) < 0.f )
	{
		// ray's going the wrong way;  we immediately know that it's not going to hit the box.
		return false;
	}*/

	// okay.  We've handled the easy cases.  Now we need to figure out T values at which we hit each
	// plane of the box.

	// box.min.x = pos.x + (dir.x*T)
	// dir.x*T = box.min.x - pos.x
	// T = (box.min.x - pos.x) / dir.x

	float inX, outX, inY, outY, inZ, outZ;

	if ( dir.x > 0.f )
	{
		inX = (min.x - pos.x) / dir.x;
		outX = (max.x - pos.x) / dir.x;
	}
	else if ( dir.x < 0.f )
	{
		inX = (max.x - pos.x) / dir.x;
		outX = (min.x - pos.x) / dir.x;
	}
	else
	{
		// no movement in 'x', so if the position 'x' isn't in our min/max.x, we don't collide.
		if ( pos.x < min.x || pos.x > max.x )
			return false;

		inX = -100000.f;		// arbitrary large values
		outX = 100000.f;
	}

	if ( dir.y > 0.f )
	{
		inY = (min.y - pos.y) / dir.y;
		outY = (max.y - pos.y) / dir.y;
	}
	else if ( dir.y < 0.f )
	{
		inY = (max.y - pos.y) / dir.y;
		outY = (min.y - pos.y) / dir.y;
	}
	else
	{
		if ( pos.y < min.y || pos.y > max.y )
			return false;

		inY = -100000.f;		// arbitrary large values
		outY = 100000.f;
	}

	if ( dir.z > 0.f )
	{
		inZ = (min.z - pos.z) / dir.z;
		outZ = (max.z - pos.z) / dir.z;
	}
	else if ( dir.z < 0.f )
	{
		inZ = (max.z - pos.z) / dir.z;
		outZ = (min.z - pos.z) / dir.z;
	}
	else
	{
		if ( pos.z < min.z || pos.z > max.z )
			return false;

		inZ = -100000.f;		// arbitrary large values
		outZ = 100000.f;
	}

	float maxIn = vsMax( inX, vsMax( inY, inZ ) );
	float minOut = vsMin( outX, vsMin( outY, outZ ) );

	if ( maxIn >= 0.f && maxIn <= minOut )
	{
		return true;	// all three axes go "in" before any go "out".
	}
	return false;
}

bool
vsBox3D::CollideRay( vsVector3D *result, float *resultT, const vsVector3D &pos, const vsVector3D &dir ) const
{
	// handle the easy case;  starting point is inside this box.
	if ( ContainsPoint(pos) )
	{
		*resultT = 0;
		*result = pos;
		return true;
	}
	// we now know that the ray does not begin inside the box.  So let's take
	// the vector from the start point to the centroid of the box, and see if
	// 'dir' is going in even vaguely the right direction.

	/*vsVector3D center = min + max * 0.5f;
	 vsVector3D rayStartToCenter = center - pos;

	 if ( rayStartToCenter.Dot( dir ) < 0.f )
	 {
	 // ray's going the wrong way;  we immediately know that it's not going to hit the box.
	 return false;
	 }*/

	// okay.  We've handled the easy cases.  Now we need to figure out T values at which we hit each
	// plane of the box.

	// box.min.x = pos.x + (dir.x*T)
	// dir.x*T = box.min.x - pos.x
	// T = (box.min.x - pos.x) / dir.x

	float inX, outX, inY, outY, inZ, outZ;

	if ( dir.x > 0.f )
	{
		inX = (min.x - pos.x) / dir.x;
		outX = (max.x - pos.x) / dir.x;
	}
	else if ( dir.x < 0.f )
	{
		inX = (max.x - pos.x) / dir.x;
		outX = (min.x - pos.x) / dir.x;
	}
	else
	{
		// no movement in 'x', so if the position 'x' isn't in our min/max.x, we don't collide.
		if ( pos.x < min.x || pos.x > max.x )
			return false;

		inX = -100000.f;		// arbitrary large values
		outX = 100000.f;
	}

	if ( dir.y > 0.f )
	{
		inY = (min.y - pos.y) / dir.y;
		outY = (max.y - pos.y) / dir.y;
	}
	else if ( dir.y < 0.f )
	{
		inY = (max.y - pos.y) / dir.y;
		outY = (min.y - pos.y) / dir.y;
	}
	else
	{
		if ( pos.y < min.y || pos.y > max.y )
			return false;

		inY = -100000.f;		// arbitrary large values
		outY = 100000.f;
	}

	if ( dir.z > 0.f )
	{
		inZ = (min.z - pos.z) / dir.z;
		outZ = (max.z - pos.z) / dir.z;
	}
	else if ( dir.z < 0.f )
	{
		inZ = (max.z - pos.z) / dir.z;
		outZ = (min.z - pos.z) / dir.z;
	}
	else
	{
		if ( pos.z < min.z || pos.z > max.z )
			return false;

		inZ = -100000.f;		// arbitrary large values
		outZ = 100000.f;
	}

	float maxIn = vsMax( inX, vsMax( inY, inZ ) );
	float minOut = vsMin( outX, vsMin( outY, outZ ) );

	if ( maxIn >= 0.f && maxIn <= minOut )
	{
		if ( maxIn < *resultT )	// are we all in before any previous collision?
		{
			*resultT = maxIn;
			*result = pos + (dir*maxIn);
			return true;	// all three axes go "in" before any go "out".
		}
	}
	return false;
}


void
vsBox3D::ExpandToInclude( const vsVector3D &pos )
{
	if ( !set )
	{
		Set( pos, pos );
	}
	else
	{
		min.x = vsMin( min.x, pos.x );
		min.y = vsMin( min.y, pos.y );
		min.z = vsMin( min.z, pos.z );

		max.x = vsMax( max.x, pos.x );
		max.y = vsMax( max.y, pos.y );
		max.z = vsMax( max.z, pos.z );
	}
}

void
vsBox3D::ExpandToInclude( const vsBox3D &b )
{
	ExpandToInclude(b.min);
	ExpandToInclude(b.max);
}

float
vsBox3D::DistanceFrom( const vsVector3D &pos ) const
{
	float sqDistance = SqDistanceFrom(pos);

	if ( sqDistance > 0.f )
	{
		return vsSqrt( sqDistance );
	}

	return 0.f;
}

float
vsBox3D::SqDistanceFrom( const vsVector3D &pos ) const
{
	if ( ContainsPoint( pos ) )
	{
		return 0.f;
	}

	vsVector3D delta = vsVector3D::Zero;

	if ( pos.x < min.x )
	{
		delta.x = min.x - pos.x;
	}
	else if ( pos.x > max.x )
	{
		delta.x = pos.x - max.x;
	}

	if ( pos.y < min.y )
	{
		delta.y = min.y - pos.y;
	}
	else if ( pos.y > max.y )
	{
		delta.y = pos.y - max.y;
	}

	if ( pos.z < min.z )
	{
		delta.z = min.z - pos.z;
	}
	else if ( pos.z > max.z )
	{
		delta.z = pos.z - max.z;
	}

	return delta.SqLength();
}


vsVector3D
vsBox3D::OffsetFrom( const vsVector3D &pos ) const
{
	if ( ContainsPoint( pos ) )
	{
		return vsVector3D::Zero;
	}

	vsVector3D delta = vsVector3D::Zero;

	if ( pos.x < min.x )
	{
		delta.x = min.x - pos.x;
	}
	else if ( pos.x > max.x )
	{
		delta.x = pos.x - max.x;
	}

	if ( pos.y < min.y )
	{
		delta.y = min.y - pos.y;
	}
	else if ( pos.y > max.y )
	{
		delta.y = pos.y - max.y;
	}

	if ( pos.z < min.z )
	{
		delta.z = min.z - pos.z;
	}
	else if ( pos.z > max.z )
	{
		delta.z = pos.z - max.z;
	}

	return delta;
}


vsBox3D vsInterpolate( float alpha, const vsBox3D& a, const vsBox3D& b )
{
	return vsBox3D( vsInterpolate( alpha, a.GetMin(), b.GetMin() ), vsInterpolate( alpha, a.GetMax(), b.GetMax() ) );
}

vsOrientedBox3D::vsOrientedBox3D()
{
}

vsOrientedBox3D::vsOrientedBox3D( const vsBox3D& box, const vsTransform3D& transform ):
	m_box(box),
	m_transform(transform)
{
	for ( int i = 0; i < 8; i++ )
	{
		m_corner[i] = m_transform.ApplyTo(m_box.Corner(i));
	}
}

bool
vsOrientedBox3D::Contains( const vsOrientedBox3D& other )
{
	const vsVector3D &ax = m_transform.GetMatrix().x;
	const vsVector3D &ay = m_transform.GetMatrix().y;
	const vsVector3D &az = m_transform.GetMatrix().z;
	return (
			SAT_Contains( other, ax ) &&
			SAT_Contains( other, ay ) &&
			SAT_Contains( other, az ) );
}

bool
vsOrientedBox3D::Intersects( const vsOrientedBox3D& other )
{
	// we're going to check a whole heap of directions to see whether we can
	// find one which doesn't result in our vertices intersecting.  If we can
	// find any direction on which our vertices don't mingle, then our two
	// objects cannot be intersecting!
	//
	const vsVector3D &ax = m_transform.GetMatrix().x;
	const vsVector3D &ay = m_transform.GetMatrix().y;
	const vsVector3D &az = m_transform.GetMatrix().z;
	const vsVector3D &bx = other.m_transform.GetMatrix().x;
	const vsVector3D &by = other.m_transform.GetMatrix().y;
	const vsVector3D &bz = other.m_transform.GetMatrix().z;
	if (
			!SAT_Intersects( other, ax ) ||
			!SAT_Intersects( other, ay ) ||
			!SAT_Intersects( other, az ) ||
			!SAT_Intersects( other, bx ) ||
			!SAT_Intersects( other, by ) ||
			!SAT_Intersects( other, bz )
	   )
	{
		return false;
	}
	if ( ax != bx && !SAT_Intersects( other, ax.Cross(bx) ) )
		return false;
	if ( ax != by && !SAT_Intersects( other, ax.Cross(by) ) )
		return false;
	if ( ax != bz && !SAT_Intersects( other, ax.Cross(bz) ) )
		return false;
	if ( ay != bx && !SAT_Intersects( other, ay.Cross(bx) ) )
		return false;
	if ( ay != by && !SAT_Intersects( other, ay.Cross(by) ) )
		return false;
	if ( ay != bz && !SAT_Intersects( other, ay.Cross(bz) ) )
		return false;
	if ( az != bx && !SAT_Intersects( other, az.Cross(bx) ) )
		return false;
	if ( az != by && !SAT_Intersects( other, az.Cross(by) ) )
		return false;
	if ( az != bz && !SAT_Intersects( other, az.Cross(bz) ) )
		return false;

	return true;
}

bool
vsOrientedBox3D::SAT_Contains( const vsOrientedBox3D& other, const vsVector3D& axis )
{
	return SAT_Contains( other.m_corner, 8, axis );
}

bool
vsOrientedBox3D::SAT_Contains( const vsVector3D* points, int pointCount, const vsVector3D& axis, float otherRadius )
{
	// we're going to project each of MY corners onto this axis and get the 1D range
	// of distances.
	//
	// next, we'll project each of the provided points onto this same axis, and get
	// its range of distances along the axis.
	//
	// Then we check that I have BOTH the minimum and maximum points.

	float aMin, aMax, bMin, bMax;
	aMin = aMax = m_corner[0].Dot(axis);
	bMin = points[0].Dot(axis) - otherRadius;
	bMax = points[0].Dot(axis) + otherRadius;
	for ( int i = 1; i < 8; i++ )
	{
		float distance = m_corner[i].Dot(axis);
		aMin = vsMin( aMin, distance );
		aMax = vsMax( aMax, distance );
	}
	for ( int i = 1; i < pointCount; i++ )
	{
		float distance = points[i].Dot(axis);
		bMin = vsMin( bMin, distance - otherRadius );
		bMax = vsMax( bMax, distance + otherRadius );
	}

	if ( aMax > bMax && aMin < bMin )
		return true;

	return false;
}
bool
vsOrientedBox3D::SAT_Intersects( const vsVector3D* points, int pointCount, const vsVector3D& axis, float otherRadius )
{
	// we're going to project each of MY corners onto this axis and get the 1D range
	// of distances.
	//
	// next, we'll project each of the provided points onto this same axis, and get
	// its range of distances along the axis.
	//
	// Then we check for overlap between those ranges.

	float aMin, aMax, bMin, bMax;
	aMin = aMax = m_corner[0].Dot(axis);
	bMin = points[0].Dot(axis) - otherRadius;
	bMax = points[0].Dot(axis) + otherRadius;
	for ( int i = 1; i < 8; i++ )
	{
		float distance = m_corner[i].Dot(axis);
		aMin = vsMin( aMin, distance );
		aMax = vsMax( aMax, distance );
	}
	for ( int i = 1; i < pointCount; i++ )
	{
		float distance = points[i].Dot(axis);
		bMin = vsMin( bMin, distance - otherRadius );
		bMax = vsMax( bMax, distance + otherRadius );
	}

	if ( aMax < bMin || aMin > bMax )
		return false;

	return true;
}

bool
vsOrientedBox3D::SAT_Intersects( const vsOrientedBox3D& other, const vsVector3D& axis )
{
	return SAT_Intersects( other.m_corner, 8, axis );
}

bool
vsOrientedBox3D::IntersectsLineStrip( const vsVector3D* point, int pointCount, float radius )
{
	for ( int i = 0; i < pointCount-1; i++ )
	{
		// for each line segment in the strip..
		if ( IntersectsLineSegment(point[i], point[i+1], radius) )
			return true;
	}
	return false;
}

bool
vsOrientedBox3D::IntersectsLineSegment( const vsVector3D& a, const vsVector3D& b, float radius )
{
	// check this line against each of my axes, see if there is one where we don't intersect.
	const vsVector3D &ax = m_transform.GetMatrix().x;
	const vsVector3D &ay = m_transform.GetMatrix().y;
	const vsVector3D &az = m_transform.GetMatrix().z;

	vsVector3D direction = b-a;
	direction.Normalise();

	vsVector3D segment[2] = { a, b };
	if (
			!SAT_Intersects( segment, 2, ax, radius ) ||
			!SAT_Intersects( segment, 2, ay, radius ) ||
			!SAT_Intersects( segment, 2, az, radius )
	   )
	{
		return false;
	}
	if ( direction != ax && !SAT_Intersects( segment, 2, direction.Cross(ax), radius ) )
		return false;
	if ( direction != ay && !SAT_Intersects( segment, 2, direction.Cross(ay), radius ) )
		return false;
	if ( direction != az && !SAT_Intersects( segment, 2, direction.Cross(az), radius ) )
		return false;
	return true;
}

bool
vsOrientedBox3D::IntersectsSphere( const vsVector3D& center, float radius )
{
	if ( ContainsPoint( center ) )
		return true;

	const vsVector3D &ax = m_transform.GetMatrix().x;
	const vsVector3D &ay = m_transform.GetMatrix().y;
	const vsVector3D &az = m_transform.GetMatrix().z;
	return (SAT_Intersects( &center, 1, ax, radius ) &&
			SAT_Intersects( &center, 1, ay, radius ) &&
			SAT_Intersects( &center, 1, az, radius ) );
}

bool
vsOrientedBox3D::ContainsPoint( const vsVector3D& point )
{
	const vsVector3D &ax = m_transform.GetMatrix().x;
	const vsVector3D &ay = m_transform.GetMatrix().y;
	const vsVector3D &az = m_transform.GetMatrix().z;
	return (
			SAT_Contains( &point, 1, ax ) &&
			SAT_Contains( &point, 1, ay ) &&
			SAT_Contains( &point, 1, az ) );
}


