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

uint32_t vsLowBit( uint32_t n )
{
	return n &= -n ;
}

uint32_t vsHighBit( uint32_t n )
{
	n |= (n >>  1);
    n |= (n >>  2);
    n |= (n >>  4);
    n |= (n >>  8);
    n |= (n >> 16);
    return n - (n >> 1);
}

uint32_t vsNextPowerOfTwo( uint32_t value )
{
	uint32_t n = vsHighBit(value);
	return  n << (!!(value ^ n)); // it's over 9000!
}


uint8_t vsHighBitPosition( uint32_t b )
{
	static const uint32_t deBruijnMagic = 0x06EB14F9;
	static const uint8_t deBruijnTable[64] = {
	     0,  0,  0,  1,  0, 16,  2,  0, 29,  0, 17,  0,  0,  3,  0, 22,
	    30,  0,  0, 20, 18,  0, 11,  0, 13,  0,  0,  4,  0,  7,  0, 23,
	    31,  0, 15,  0, 28,  0,  0, 21,  0, 19,  0, 10, 12,  0,  6,  0,
	     0, 14, 27,  0,  0,  9,  0,  5,  0, 26,  8,  0, 25,  0, 24,  0,
	 };
	return deBruijnTable[(vsHighBit(b) * deBruijnMagic) >> 26];
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

bool vsIsPointInsideTriangle( const vsVector3D &point, const vsVector3D &vert0, const vsVector3D &vert1, const vsVector3D &vert2)
{
#if 1

	// Check whether the given point is inside all of our edges.

	vsVector3D vert[3] = { vert0, vert1, vert2 };
	bool inside = true;
	for ( int i = 0; i < 3 && inside; i++ )
	{
		int ni = (i+1)%3;
		int pi = (i+2)%3;

		vsVector3D cp1 = (vert[ni]-vert[i]).Cross( point - vert[i] );
		vsVector3D cp2 = (vert[ni]-vert[i]).Cross( vert[pi] - vert[i] );

		if ( cp1.Dot(cp2) < 0.f )
		{
			inside = false;
		}
	}
	return inside;
#else

	// calculate barycentric coordinates, and if we have them (that is, if 's'
	// and 't' are both in the range of [0..1]), then we're inside the
	// triangle.  Otherwise, not!
	//
	// 'point' is assumed to be within the plane of the triangle.
	//
	vsVector3D edgeA = vert1-vert0;
	vsVector3D edgeB = vert2-vert0;
	vsVector3D w = point - vert0;

	float uu = edgeA.Dot(edgeA);
	float uv = edgeA.Dot(edgeB);
	float vv = edgeB.Dot(edgeB);
	float wu = w.Dot(edgeA);
	float wv = w.Dot(edgeB);
	float d = uv * uv - uu * vv;

	float invD = 1.0 / d;
	float s = (uv * wv - vv * wu) * invD;
	if ( s < 0.f || s > 1.f )
		return false;
	float t = (uv * wu - uu * wv) * invD;
	if ( t < 0.f || t > 1.f )
		return false;

	return true;
#endif
}

static bool getLowestRoot( float a, float b, float c, float maxR, float* root)
{
	float determinant = b * b - 4.f * a * c;
	if ( determinant < 0.f ) // no solutions
		return false;

	float sqrtD = vsSqrt(determinant);
	float r1 = (-b - sqrtD) / (2.f*a);
	float r2 = (-b + sqrtD) / (2.f*a);

	if ( r1 > r2 )
	{
		float t = r1;
		r1 = r2;
		r2 = t;
	}

	if ( r1 > 0.f && r1 < maxR )
	{
		*root = r1;
		return true;
	}

	if ( r2 > 0 && r2 < maxR )
	{
		*root = r2;
		return true;
	}

	return false; // no valid solutions
}

bool vsCollideSweptSphereVsTriangle( const vsVector3D &sphereCenter, float sphereRadius,
		const vsVector3D &dir, const vsVector3D &vert0, const vsVector3D &vert1, const vsVector3D &vert2,
		float *outputT, vsVector3D *contactPoint)
{
	// This is an experimental swept-sphere-vs-triangle implementation *very* loosely
	// based on an algorithm as described at:
	//
	// https://www.braynzarsoft.net/viewtutorial/q16390-31-sliding-camera-collision-detection
	//
	vsVector3D direction = dir;
	direction.NormaliseSafe();

	// float minDist = std::numeric_limits<float>::max();
	bool hit = false;

	// float hitT = std::numeric_limits<float>::max();
	float hitT = *outputT;

	vsVector3D sideA = (vert1-vert0);
	vsVector3D sideB = (vert2-vert0);
	vsVector3D triNormal = (sideB.Normalised()).Cross(sideA.Normalised());
	if ( triNormal.SqLength() <= 0.f ) // ignore infinitely thin "triangles"
		return false;

	triNormal.Normalise();

	// first, check if we're moving the wrong direction to hit this triangle.
	if ( triNormal.Dot( dir ) > 0.0f )
		return false; // no hit;  we'd travel through the triangle without collision from this side!

	// Test whether the sphere is currently above the triangle's plane, but
	// is going to enter or pass through the triangle's plane and that's going
	// to happen inside the bounds of the triangle.

	{
		float t0, t1;
		float planeConstant = - vert0.Dot(triNormal);
		float distanceFromSphereCenterToTriPlane = sphereCenter.Dot(triNormal) + planeConstant;
		float planeNormalDotDirection = triNormal.Dot(dir);

		if ( planeNormalDotDirection == 0.f ) // parallel with plane
		{
			if ( vsFabs( distanceFromSphereCenterToTriPlane ) >= sphereRadius )
				return false; // sphere not in plane and velocity is parallel to plane;  no collision
							  // otherwise, we're in the plane and will skip the "hitting center" test
							  // since we would have to hit a vertex or an edge first.
		}
		else
		{
			t0 = ( sphereRadius - distanceFromSphereCenterToTriPlane) / planeNormalDotDirection;
			t1 = ( -sphereRadius - distanceFromSphereCenterToTriPlane) / planeNormalDotDirection;

			// make sure t0 is lower than t1
			if ( t0 > t1 || t0 < 0.f )
			{
				float tmp = t0;
				t0 = t1;
				t1 = tmp;
			}

			vsVector3D planeIntersectionPoint = sphereCenter + t0 * dir - (sphereRadius * triNormal);
			if ( vsIsPointInsideTriangle( planeIntersectionPoint, vert0, vert1, vert2 ) )
			{
				if ( t0 > 0.f && t0 < hitT )
				{
					hitT = t0;
					if ( contactPoint )
						*contactPoint = planeIntersectionPoint;
					hit = true;
				}
			}
		}
	}

	// vsVector3D triToSphereCenter = sphereCenter - vert0;
	// float heightOverPlane = triToSphereCenter.Dot( triNormal );
	// if ( heightOverPlane < -sphereRadius )
	// 	return false; // we're already behind this triangle;  no collision!
	// else if ( heightOverPlane > sphereRadius )
	// {
	// 	// we're in front of this triangle, so now we have to check whether we're
	// 	// close enough to hit.
    //
	// 	heightOverPlane -= sphereRadius; // how high our closest point is over the triangle plane
	// 	float dot = triNormal.Dot( dir );
	// 	if ( dot != 0.f )
	// 	{
	// 		float testT = -heightOverPlane / dot;
	// 		// vsVector3D positionOnPlane = sphereCenter + dir * (testT+sphereRadius);
	// 		vsVector3D positionOnPlane = sphereCenter + dir * testT - (triNormal * sphereRadius);
	// 		if ( vsIsPointInsideTriangle( positionOnPlane, vert0, vert1, vert2 ) )
	// 		{
	// 			// technically this next condition always passes because it's
	// 			// our first hit test that doesn't make us exit immediately.
	// 			if ( testT < hitT )
	// 			{
	// 				hitT = testT;
	// 				if (contactPoint)
	// 					*contactPoint = positionOnPlane;
	// 				hit = true;
	// 				// vsLog("CASE 0 HIT");
	// 			}
	// 		}
	// 	}
	// }

	// Okay, so we've tested whether the sphere is going to hit the triangle based
	// on its centerpoint travelling through the center of the triangle plane.
	// Next, we need to test whether it's going to hit by passing through one
	// of the triangle vertices.
	vsVector3D p[3] = {vert0,vert1,vert2};
	for ( int i = 0; i < 3; i++ )
	{
		float a,b,c;

		float dirLengthSquared = 1.f;
		float thisT;
		a = dirLengthSquared;

		vsVector3D delta =  sphereCenter - p[i];
		b = 2.f * dir.Dot(delta);
		c = delta.SqLength() - sphereRadius*sphereRadius;

		if ( getLowestRoot(a,b,c,hitT,&thisT) )
		{
			// vsLog("HIT VERTEX");
			hitT = thisT;
			hit = true;
			*contactPoint = p[i];
		}
	}

	// Okay, now that we've tested the sphere vs. the triangle plane and vs.
	// the triangle vertices, now we need to test it vs. the triangle edges.
	for ( int i = 0; i < 3; i++ )
	{
		vsVector3D edge0 = p[i];
		vsVector3D edge1 = p[(i+1)%3];
		vsVector3D edge = edge1-edge0;
		vsVector3D sphereToEdge = p[i] - sphereCenter;
		float edgeLength = edge.Length();
		float edgeDotDirection = edge.Dot(direction);
		float edgeDotSpherePositionToEdge = edge.Dot(sphereToEdge);
		float spherePositionToEdgeLengthSquared = sphereToEdge.SqLength();

		float edgeLengthSquared = edgeLength * edgeLength;
		float directionLengthSquared = 1.f;

		float a = edgeLengthSquared * -directionLengthSquared + (edgeDotDirection * edgeDotDirection);
		float b = edgeLengthSquared * (2.f * dir.Dot(sphereToEdge)) - (2.f * edgeDotDirection * edgeDotSpherePositionToEdge);
		float c = edgeLengthSquared * (sphereRadius*sphereRadius - spherePositionToEdgeLengthSquared) + (edgeDotSpherePositionToEdge * edgeDotSpherePositionToEdge);
		float thisT = 0.f;

		if ( getLowestRoot(a,b,c,hitT,&thisT) )
		{
			// we hit the edge.

			float f = (edgeDotDirection * thisT - edgeDotSpherePositionToEdge) / edgeLengthSquared;
			if ( f >= 0.f && f <= 1.f ) {
				// vsLog("HIT EDGE");
				// if this collision happened, set results.
				hitT = thisT;
				hit = true;
				*contactPoint = edge0 + f * edge;
			}
		}

		// vsVector3D edgeDir = (edge1-edge0);
		// float edgeLength = edgeDir.Length();
		// edgeDir *= 1.f / edgeLength;
        //
		// // now we're going to define a plane on this edge that's swept by
		// // the direction of travel, and see whether our stationary sphere
		// // hits that swept edge.
        //
		// vsVector3D deltaA = (edge1-edge0).Normalised();
		// vsVector3D deltaB = (sphereCenter-edge0).Normalised();
        //
		// vsVector3D planeNormal = deltaA.Cross(deltaB);
		// float height = planeNormal.Dot( sphereCenter );
        //
		// if ( height > sphereRadius || height < -sphereRadius )
		// 	continue; // we're not intersecting that swept edge at all.
        //
		// // now we figure out the circle of the sphere which exists on this swept edge.
		// float circleRadius = vsSqrt(sphereRadius * sphereRadius - height * height);
		// vsVector3D circleCenter = sphereCenter - planeNormal * height;
        //
		// float projection = vsClamp((circleCenter-edge0).Dot( edgeDir ), 0, edgeLength);
		// vsVector3D onLine = edge0 + projection * edgeDir;
		// vsVector3D delta = onLine - circleCenter;
		// delta.Normalise();
		// vsVector3D circleCollisionPoint = delta * circleRadius + circleCenter; // point on sphere which maybe collides with edge


	}

	if ( hit )
	{
		*outputT = hitT;
	}
	return hit;
}

bool vsCollideLineVsSphere( const vsVector3D &sphereCenter, float sphereRadius, const vsVector3D &vert0, const vsVector3D &vert1, int* intersectionCount, float* tA, float* tB )
{
	vsVector3D delta = vert1-vert0;
	vsVector3D vert0ToSphere = sphereCenter-vert0;

	float a, b, c, i;
	a = delta.SqLength();
	b = 2.0f * ( delta.Dot( vert0ToSphere ) );
	c = sphereCenter.SqLength() + vert0.SqLength() -
		2.0f * vert0.Dot(sphereCenter) - sphereRadius*sphereRadius;

	// yay it's our old friend the quadratic formula!  I missed you! <3
	i = b * b - 4 * a * c;

	if ( i < 0 )
		return false; // no collision
	if ( i == 0 )
	{
		*intersectionCount = 1;
		*tA = -b / (2*a);
	}
	else
	{
		*intersectionCount = 2;
		*tA = -b + ( vsSqrt( b*b - 4*a*c )) / (2*a);
		*tB = -b - ( vsSqrt( b*b - 4*a*c )) / (2*a);
	}
	return true;
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

	vsVector3D StartAToStartB = startB-startA;

	if ( perp == vsVector3D::Zero )
	{
		// since our line segments are parallel, let's just
		timeA = timeB = 0.0f;
	}
	else
	{
		float perpDotPerp = perp.Dot(perp);
		timeA = vsClamp( StartAToStartB.Cross(deltaB).Dot(perp) / perpDotPerp, 0.f, 1.f );
		timeB = vsClamp( StartAToStartB.Cross(deltaA).Dot(perp) / perpDotPerp, 0.f, 1.f );
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

float vsSqDistanceBetweenLineSegments_XZ( const vsVector3D& startA, const vsVector3D& endA, const vsVector3D& startB, const vsVector3D& endB, vsVector3D *closestA, vsVector3D *closestB )
{
	if ( closestA || closestB )
	{
		vsVector2D closestA2D, closestB2D;
		float result = vsSqDistanceBetweenLineSegments( startA.XZ(), endA.XZ(),
				startB.XZ(), endB.XZ(),
				&closestA2D, &closestB2D );

		if ( closestA )
			closestA->Set( closestA2D.x, 0.f, closestA2D.y );
		if ( closestB )
			closestB->Set( closestB2D.x, 0.f, closestB2D.y );
		return result;
	}
	return vsSqDistanceBetweenLineSegments( startA.XZ(), endA.XZ(),
			startB.XZ(), endB.XZ(),
			nullptr, nullptr );
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

float vsSqDistanceFromPointToLineSegment(
		const vsVector2D& point,
		const vsVector2D& startLine,
		const vsVector2D& endLine,
		vsVector2D* closest )
{
	vsVector2D direction = endLine-startLine;
	float len = direction.Length();
	direction.NormaliseSafe();

	float projection = direction.Dot( point - startLine );
	projection = vsClamp(0, projection, len);
	vsVector2D closestPoint = startLine + direction * projection;
	if ( closest )
	{
		*closest = closestPoint;
	}
	return (closestPoint-point).SqLength();
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
	if ( det == 0.0f ) // no intersection;  parallel!
	{
		// our two lines are parallel, so there's no intersection point for us
		// to find (which would have made it easy!).  Instead, we need to find
		// the distances from startA and endA to a point on B, and then
		// the distances from startB and endB to a point on A.  We'll take
		// the shortest distance out of those!

		vsVector2D closestOnA = startA;
		vsVector2D closestOnB = startB;
		float closestSq = std::numeric_limits<float>::max();

		vsVector2D thisPoint;
		{
			float thisDistanceSq = vsSqDistanceFromPointToLineSegment( startA, startB, endB, &thisPoint );
			if ( thisDistanceSq < closestSq )
			{
				closestSq = thisDistanceSq;
				closestOnA = startA;
				closestOnB = thisPoint;
			}
		}
		{
			float thisDistanceSq = vsSqDistanceFromPointToLineSegment( endA, startB, endB, &thisPoint );
			if ( thisDistanceSq < closestSq )
			{
				closestSq = thisDistanceSq;
				closestOnA = endA;
				closestOnB = thisPoint;
			}
		}
		{
			float thisDistanceSq = vsSqDistanceFromPointToLineSegment( startB, startA, endA, &thisPoint );
			if ( thisDistanceSq < closestSq )
			{
				closestSq = thisDistanceSq;
				closestOnA = thisPoint;
				closestOnB = startB;
			}
		}
		{
			float thisDistanceSq = vsSqDistanceFromPointToLineSegment( startA, startB, endB, &thisPoint );
			if ( thisDistanceSq < closestSq )
			{
				closestSq = thisDistanceSq;
				closestOnA = thisPoint;
				closestOnB = endB;
			}
		}

		if ( closestA )
			*closestA = closestOnA;
		if ( closestB )
			*closestB = closestOnB;

		return closestSq;

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

float vsSqDistanceBetweenLines( const vsVector3D& startA, const vsVector3D& endA, const vsVector3D& startB, const vsVector3D& endB, vsVector3D *closestA, vsVector3D *closestB )
{
	vsVector3D deltaA = endA - startA;
	vsVector3D deltaB = endB - startB;

	// perpendicular
	vsVector3D perp = deltaA.Cross(deltaB);

	// if perpendicular == 0, it means that our line segments are parallel.

	float timeA, timeB;

	vsVector3D StartAToStartB = startB-startA;

	if ( perp == vsVector3D::Zero )
	{
		// since our line segments are parallel, let's just
		timeA = timeB = 0.0f;
	}
	else
	{
		float perpDotPerp = perp.Dot(perp);
		timeA = StartAToStartB.Cross(deltaB).Dot(perp) / perpDotPerp;
		timeB = StartAToStartB.Cross(deltaA).Dot(perp) / perpDotPerp;
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

// This code for counting bits is based on a StackOverflow answer, here:
// https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
//
uint32_t vsCountSetBits( uint32_t i )
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
