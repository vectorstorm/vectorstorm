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

#define vsMin(a,b) ((a<b)?a:b)
#define vsMax(a,b) ((a>b)?a:b)
#define vsClamp(a,min,max) ( vsMin( (max), vsMax( (min), (a) ) ) )

inline bool	vsIsNaN(float x) { volatile float o_x = x; return (x != o_x); }	// NaN != NaN.  Any number which doesn't equal itself must be NaN!

inline float vsFabs( float in ) { return (in >= 0.f)?in:-in; }

float vsSqrt( float in );

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

int		vsNextPowerOfTwo( int value );

class vsVector3D;
bool vsCollideRayVsTriangle( const vsVector3D &orig, const vsVector3D &dir, const vsVector3D &vert0, const vsVector3D &vert1, const vsVector3D &vert2, float *t, float *u, float *v);

float vsInterpolate( float alpha, float a, float b );
float vsProgressFraction( float value, float a, float b );	// returned value is what you'd pass as 'alpha' to vsInterpolate, to get back the 'value' value.


#endif // VS_MATH_H


