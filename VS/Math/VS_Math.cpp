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

float vsProgressFraction_Clamped( float value, float a, float b )
{
	return vsClamp( vsProgressFraction(value,a,b), 0.f, 1.f );
}

float vsFadeInOut( float time, float startFadeIn, float endFadeIn, float startFadeOut, float endFadeOut )
{
	if ( time < startFadeIn || time > endFadeOut )
		return 0.f;

	if ( startFadeOut < endFadeIn ) // Bad case; we're supposed to start fading out before we finish fading in!
	{
		// for now, just swap the two values so the rest of our logic works.
		// But we could be smarter than this;  only fade partway in before
		// starting to fade out again.  But that's a bit more complicated to
		// think about.
	}

	if ( time > startFadeOut )
		return vsProgressFraction( time, endFadeOut, startFadeOut );
	else if ( time > endFadeIn )
		return 1.f;
	else
		return vsProgressFraction( time, startFadeIn, endFadeIn );

	return 0.f;
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

bool vsLineSegmentsIntersect( const vsVector2D& startA, const vsVector2D& endA, const vsVector2D& startB, const vsVector2D& endB, vsVector2D *where )
{
	vsVector2D deltaA = endA - startA;
	vsVector2D deltaB = endB - startB;
	float delta = deltaB.x * deltaA.y - deltaB.y * deltaA.x;
	if ( delta == 0.0 )
		return false; // parallel

	float s = (deltaA.x * (deltaB.y - deltaA.y) + deltaA.y * (deltaA.x - deltaB.x)) / delta;
	float t = (deltaB.x * (deltaA.y - deltaB.y) + deltaB.y * (deltaB.x - deltaA.x)) / (-delta);
	return ( s >= 0.f && s <= 1.f &&
			t >= 0.f && t <= 1.f);
}

float vsSqDistanceBetweenRays( const vsVector2D& startA, const vsVector2D& endA, const vsVector2D& startB, const vsVector2D& endB, vsVector2D *closestA, vsVector2D *closestB )
{
	vsVector2D dirA = endA - startA;
	vsVector2D dirB = endB - startB;
	dirA.NormaliseSafe();
	dirB.NormaliseSafe();

	// Okay.  We need to calculate the intersection point between two rays:
	//
	// point P = startA + dirA * u
	//   AND
	//       P = startB + dirB * v
	//
	// Or to put that differently:
	//
	//       0 = (startA + dirA * u) - (startB + dirB * v)
	//
	// SO:

	vsVector2D delta = startB - startA;
	float det = dirB.x * dirA.y - dirB.y * dirA.x;
	if ( det == 0.0f ) // no intersection!
	{
		return 0.0;
	}
	float u = (delta.y * dirB.x - delta.x * dirB.y) / det;
	float v = (delta.y * dirA.x - delta.x * dirA.y) / det;

	vsVector2D closestPointOnA = startA + dirA * u;
	vsVector2D closestPointOnB = startB + dirB * v;

	if ( closestA )
		*closestA = closestPointOnA;
	if ( closestB )
		*closestB = closestPointOnB;

	return (closestPointOnB - closestPointOnA).SqLength();
}

float vsSqDistanceBetweenLineSegments( const vsVector2D& startA, const vsVector2D& endA, const vsVector2D& startB, const vsVector2D& endB, vsVector2D *closestA, vsVector2D *closestB )
{
	vsVector2D deltaA = endA - startA;
	vsVector2D deltaB = endB - startB;
	vsVector2D dirA = deltaA;
	vsVector2D dirB = deltaB;
	float lenA = dirA.Length();
	float lenB = dirB.Length();
	dirA.NormaliseSafe();
	dirB.NormaliseSafe();

	// Okay.  We need to calculate the intersection point between two rays:
	//
	// point P = startA + dirA * u
	//   AND
	//       P = startB + dirB * v
	//
	// Or to put that differently:
	//
	//       0 = (startA + dirA * u) - (startB + dirB * v)
	//
	// SO:

	vsVector2D delta = startB - startA;
	float det = dirB.x * dirA.y - dirB.y * dirA.x;
	if ( det == 0.0f ) // no intersection!
	{
		return 0.0;
	}
	float u = vsClamp(0.f, (delta.y * dirB.x - delta.x * dirB.y) / det, lenA);
	float v = vsClamp(0.f, (delta.y * dirA.x - delta.x * dirA.y) / det, lenB);

	vsVector2D closestPointOnA = startA + dirA * u;
	vsVector2D closestPointOnB = startB + dirB * v;

	if ( closestA )
		*closestA = closestPointOnA;
	if ( closestB )
		*closestB = closestPointOnB;

	return (closestPointOnB - closestPointOnA).SqLength();
}

