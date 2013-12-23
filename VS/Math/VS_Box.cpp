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
	list->LineList(array,24);
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

