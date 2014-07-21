/*
 *  VS_Math.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Math.h"
#include "VS_Vector.h"
#include <math.h>

float vsCos(float theta)
{
	return cosf(theta);
}

float vsSin(float theta)
{
	return sinf(theta);
}

float vsTan(float theta)
{
	return tanf(theta);
}

float vsACos(float theta)
{
	return acosf(theta);
}

float vsASin(float theta)
{
	return asinf(theta);
}

float vsATan2(float opp, float adj)
{
	return atan2f(opp, adj);
}

int vsNextPowerOfTwo( int value )
{
	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value++;

	return value;	// it's like magic!
}

bool vsCollideRayVsTriangle( const vsVector3D &orig, const vsVector3D &dir, const vsVector3D &vert0, const vsVector3D &vert1, const vsVector3D &vert2, float *t, float *u, float *v)
{
	const float EPSILON = 0.000001f;

	vsVector3D edge1, edge2, tvec, pvec, qvec;
	float det,inv_det;

	/* find vectors for two edges sharing vert0 */
	edge1 = vert1 - vert0;
	edge2 = vert2 - vert0;

	/* begin calculating determinant - also used to calculate U parameter */
	pvec = dir.Cross(edge2);

	/* if determinant is near zero, ray lies in plane of triangle */
	det = edge1.Dot( pvec );

	if (det > -EPSILON && det < EPSILON)
		return false;
	inv_det = 1.0f / det;

	/* calculate distance from vert0 to ray origin */
	tvec = orig - vert0;

	/* calculate U parameter and test bounds */
	*u = tvec.Dot(pvec) * inv_det;
	if (*u < 0.0f || *u > 1.0f)
		return false;

	/* prepare to test V parameter */
	qvec = tvec.Cross( edge1);

	/* calculate V parameter and test bounds */
	*v = dir.Dot(qvec) * inv_det;
	if (*v < 0.0f || *u + *v > 1.0f)
		return false;

	/* calculate t, ray intersects triangle */
	*t = edge2.Dot(qvec) * inv_det;

	return (*t >= 0.f);
}

float vsProgressFraction( float value, float a, float b )
{
	if ( a == b )
	{
		return 0.f;
	}
	else
	{
		float delta = b - a;
		value -= a;

		return value / delta;
	}
}

float vsInterpolate( float alpha, float a, float b )
{
	return ((1.0f-alpha)*a) + (alpha*b);
}

float vsSqDistanceBetweenLineSegments( const vsVector3D& startA, const vsVector3D& endA, const vsVector3D& startB, const vsVector3D& endB, vsVector3D *closestA, vsVector3D *closestB )
{
	vsVector3D deltaA = endA - startA;
	vsVector3D deltaB = endB - startB;

	// perpendicular
	vsVector3D perp = deltaA.Cross(deltaB);

	// if perpendicular == 0, it means that our line segments are parallel.

	float timeA, timeB;

	if ( perp == vsVector3D::Zero )
	{
		// since our line segments are parallel, let's just
		timeA = vsClamp( (startB-startA).Cross(deltaB).Dot(perp), 0.f, 1.f );
		timeB = vsClamp( (startB-startA).Cross(deltaA).Dot(perp), 0.f, 1.f );
	}
	else
	{
		timeA = vsClamp( (startB-startA).Cross(deltaB).Dot(perp) / perp.Dot(perp), 0.f, 1.f );
		timeB = vsClamp( (startB-startA).Cross(deltaA).Dot(perp) / perp.Dot(perp), 0.f, 1.f );
	}

	vsVector3D closestPointA = startA + timeA * deltaA;
	vsVector3D closestPointB = startB + timeB * deltaB;

	if ( closestA )
		*closestA = closestPointA;
	if ( closestB )
		*closestB = closestPointB;

	float distance = (closestPointB-closestPointA).SqLength();

	return distance;
}


