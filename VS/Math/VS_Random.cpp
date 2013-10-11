/*
 *  VS_Random.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Random.h"

#include "VS_Angle.h"
#include "VS_Box.h"
#include "VS_Color.h"
#include "VS_Random.h"
#include "VS_Vector.h"
#include "VS_Debug.h"

#include "MT/SFMT.h"

#include <time.h>

void
vsRandom::Init()
{
	InitWithSeed( (uint32_t)time(NULL) );
}

void
vsRandom::InitWithSeed( uint32_t seed )
{
	init_gen_rand( seed );

	int n = vsRandom::GetInt(2000);		// generate a bunch of random numbers, just to prime the random number generator.

	for ( int i = 0; i < n; i++ )
	{
		vsRandom::GetInt(10);		// generate a bunch of random numbers, just to prime the random number generator.
	}
}

void
vsRandom::InitWithSeed( const vsString &seed )
{
	size_t len = seed.length();
	size_t paddingNeeded = len % 4;

	vsString paddedString = seed;

	for ( size_t i = 0; i < paddingNeeded; i++ )
		paddedString += " ";

	len = paddedString.length();

	init_by_array( (uint32_t*)paddedString.c_str(), (int)(len / 4) );
}

float
vsRandom::GetFloat(float maxValue)
{
	float result = gen_rand32() / (float)0xffffffff;
	result *= maxValue;
	return result;
}

float
vsRandom::GetFloat(float min, float max)
{
	float delta = max - min;

	return GetFloat(delta) + min;
}

int
vsRandom::GetInt(int maxValue)
{
	return gen_rand32() % maxValue;
}

int
vsRandom::GetInt(int min, int max)
{
	int delta = (max - min)+1;

	return GetInt(delta) + min;
}


vsVector2D
vsRandom::GetVector2D(float maxLength)
{
	return GetVector2D(0.f, maxLength);
}

vsVector2D
vsRandom::GetVector2D(float minLength, float maxLength)
{
	vsAngle a( GetFloat(TWOPI) );

	vsVector2D result = a.GetForwardVector();

	result *= GetFloat(minLength,maxLength);

	return result;
}

vsVector2D
vsRandom::GetVector2D(const vsVector2D &topLeft, const vsVector2D &bottomRight)
{
	vsVector2D result( GetFloat(topLeft.x,bottomRight.x), GetFloat(topLeft.y,bottomRight.y) );

	return result;
}

vsVector3D
vsRandom::GetVector3D(float minLength, float maxLength)
{
	vsVector3D result( GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f) );

	result.Normalise();

	result *= vsRandom::GetFloat(minLength,maxLength);

	return result;
}

vsVector3D
vsRandom::GetVector3D(float maxLength)
{
	vsVector3D result( GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f) );

	if ( result.SqLength() > 1.0f )
		result.Normalise();

	result *= maxLength;

	return result;
}

vsVector3D
vsRandom::GetVector3D(const vsVector3D &topLeft, const vsVector3D &bottomRight)
{
	vsVector3D result( GetFloat(topLeft.x,bottomRight.x), GetFloat(topLeft.y,bottomRight.y), GetFloat(topLeft.z,bottomRight.z) );

	return result;
}

vsVector2D
vsRandom::GetVector2D(const vsBox2D &box)
{
	return GetVector2D( box.GetMin(), box.GetMax() );
}

vsVector3D
vsRandom::GetVector3D(const vsBox3D &box)
{
	return GetVector3D( box.GetMin(), box.GetMax() );
}


vsColor
vsRandom::GetColor(float min, float max)
{
	vsVector3D result( GetFloat(0.f,100.f), GetFloat(0.f,100.f), GetFloat(0.f,100.f) );

	if ( result.SqLength() > 1.0f )
		result.Normalise();

	float brightness = GetFloat(min,max);
	result *= brightness;

	return vsColor( result.x, result.y, result.z, 1.0f );
}
