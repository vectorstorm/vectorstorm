/*
 *  VS_Math.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MATH_H
#define VS_MATH_H

#include <math.h>

#define vsMin(a,b) (((a)<(b))?(a):(b))
#define vsMax(a,b) (((a)>(b))?(a):(b))
#define vsClamp(a,min,max) ( vsMin( (max), vsMax( (min), (a) ) ) )
#define vsSquared(a) (a*a)

inline bool	vsIsNaN(float x) { volatile float o_x = x; return (x != o_x); }	// NaN != NaN.  Any number which doesn't equal itself must be NaN!
inline int vsAbs( int in ) { return (in >= 0.f)?in:-in; }
inline float vsFabs( float in ) { return (in >= 0.f)?in:-in; }
inline float vsSqrt( float in ) { return sqrtf(in); }

float vsCos( float theta );
float vsSin( float theta );
float vsTan( float theta );
float vsACos( float theta );
float vsASin( float theta );
float vsATan2( float o, float a );

inline int vsFloor( float value )
{
	return (int)floor(value);
}

inline int vsCeil( float value )
{
	return (int)ceil(value);
}

// Badly named function;  I should really fix this.  This actually
// gives us the power of two that's EQUAL OR GREATER than the passed
// value, not (as is implied by the function name) strictly greater.
//
// So for example:
// 32 -> 32
// 33 -> 64
// 55 -> 64
// 64 -> 64
uint32_t		vsNextPowerOfTwo( uint32_t value );

// Returns the POSITION of the highest bit set in the value.
// For example:
// 0 -> 0 (by definition)
// 1 -> 0
// 2 -> 1
// 3 -> 1
// 4 -> 2
uint8_t		vsHighBitPosition( uint32_t value );

// Returns the value with ONLY its highest bit set.  This is equivalent
// to rounding DOWN to the nearest power of two.
// For example:
// 32 -> 32
// 33 -> 32
// 55 -> 32
// 64 -> 64
uint32_t		vsHighBit( uint32_t value );

uint32_t		vsCountSetBits( uint32_t value );

class vsVector2D;
class vsVector3D;
bool vsCollideRayVsTriangle( const vsVector3D &orig, const vsVector3D &dir, const vsVector3D &vert0, const vsVector3D &vert1, const vsVector3D &vert2, float *t, float *u, float *v);

float vsInterpolate( float alpha, float a, float b );
float vsProgressFraction( float value, float a, float b );	// returned value is what you'd pass as 'alpha' to vsInterpolate, to get back the 'value' value.
float vsProgressFraction_Clamped( float value, float a, float b ); // as above, but result is clamped into [0..1].

inline float vsRemapFloat( float from, float from_min, float from_max, float to_min, float to_max )
{
	return vsInterpolate(
			vsProgressFraction(from, from_min, from_max),
			to_min,
			to_max
			);
}
inline float vsRemapFloat_Clamped( float from, float from_min, float from_max, float to_min, float to_max )
{
	return vsInterpolate(
			vsProgressFraction_Clamped(from, from_min, from_max),
			to_min,
			to_max
			);
}

// vsFadeInOut is a utility function for mapping a single time value into a
// [0..1..0] sequence.
//
// returned value goes [0..1..0] as 'time' goes from [startFadeIn .. endFadeOut].
// it holds at 0 before 'startFadeIn' and after 'endFadeOut', and holds at 1 between 'endFadeIn' and 'startFadeOut'.
//
float vsFadeInOut( float time, float startFadeIn, float endFadeIn, float startFadeOut, float endFadeOut );

float vsSqDistanceBetweenLineSegments( const vsVector3D& startA, const vsVector3D& endA, const vsVector3D& startB, const vsVector3D& endB, vsVector3D *closestA = NULL, vsVector3D *closestB = NULL );
inline float vsDistanceBetweenLineSegments( const vsVector3D& startA, const vsVector3D& endA, const vsVector3D& startB, const vsVector3D& endB, vsVector3D *closestA = NULL, vsVector3D *closestB = NULL )
{
	return vsSqrt( vsSqDistanceBetweenLineSegments( startA, endA, startB, endB, closestA, closestB ) );
}
float vsSqDistanceBetweenLineSegments( const vsVector2D& startA, const vsVector2D& endA, const vsVector2D& startB, const vsVector2D& endB, vsVector2D *closestA, vsVector2D *closestB );
float vsSqDistanceBetweenRays( const vsVector2D& startA, const vsVector2D& endA, const vsVector2D& startB, const vsVector2D& endB, vsVector2D *closestA, vsVector2D *closestB );

// this finds the closest point between these two line segments, IGNORING Y.  Output
// variables will have 'y' component set to zero.
float vsSqDistanceBetweenLineSegments_XZ( const vsVector3D& startA, const vsVector3D& endA, const vsVector3D& startB, const vsVector3D& endB, vsVector3D *closestA = NULL, vsVector3D *closestB = NULL );

#endif // VS_MATH_H


