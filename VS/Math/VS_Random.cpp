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

#include <time.h>

#include <stdint.h>

#define PHI 0x9e3779b9

vsRandomSource vsRandom::s_source;

vsRandomSource::vsRandomSource()
{
	Init();
}

vsRandomSource::~vsRandomSource()
{
}

void
vsRandomSource::Init()
{
	InitWithSeed( (uint32_t)time(NULL) );
}

void
vsRandomSource::InitWithSeed( uint32_t seed )
{
	x = seed;
	y = seed + PHI;
	z = y + PHI;
	w = x ^ y ^ PHI;
}

void
vsRandomSource::InitWithSeed( const vsString &seed )
{
	uint32_t val = 0;
	for ( int i = 0; i < seed.size(); i++ )
	{
		val += seed[0];
	}
	InitWithSeed(val);
}

float
vsRandomSource::GetFloat(float maxValue)
{
	float result = Int32() / (float)0xffffffff;
	result *= maxValue;
	return result;
}

float
vsRandomSource::GetFloat(float min, float max)
{
	float delta = max - min;

	return GetFloat(delta) + min;
}

uint32_t
vsRandomSource::Int32()
{
	uint32_t t = x ^ (x << 11);

	x = y;
	y = z;
	z = w;

	return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

int
vsRandomSource::GetInt(int maxValue)
{
	return Int32() % maxValue;
}

int
vsRandomSource::GetInt(int min, int max)
{
	int delta = (max - min)+1;

	return GetInt(delta) + min;
}


vsVector2D
vsRandomSource::GetVector2D(float maxLength)
{
	return GetVector2D(0.f, maxLength);
}

vsVector2D
vsRandomSource::GetVector2D(float minLength, float maxLength)
{
	vsAngle a( GetFloat(TWOPI) );

	vsVector2D result = a.GetForwardVector();

	result *= GetFloat(minLength,maxLength);

	return result;
}

vsVector2D
vsRandomSource::GetVector2D(const vsVector2D &topLeft, const vsVector2D &bottomRight)
{
	vsVector2D result( GetFloat(topLeft.x,bottomRight.x), GetFloat(topLeft.y,bottomRight.y) );

	return result;
}

vsVector3D
vsRandomSource::GetVector3D(float minLength, float maxLength)
{
	vsVector3D result( GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f) );

	result.Normalise();

	result *= vsRandom::GetFloat(minLength,maxLength);

	return result;
}

vsVector3D
vsRandomSource::GetVector3D(float maxLength)
{
	vsVector3D result( GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f), GetFloat(-1.0f,1.0f) );

	if ( result.SqLength() > 1.0f )
		result.Normalise();

	result *= maxLength;

	return result;
}

vsVector3D
vsRandomSource::GetVector3D(const vsVector3D &topLeft, const vsVector3D &bottomRight)
{
	vsVector3D result( GetFloat(topLeft.x,bottomRight.x), GetFloat(topLeft.y,bottomRight.y), GetFloat(topLeft.z,bottomRight.z) );

	return result;
}

vsVector2D
vsRandomSource::GetVector2D(const vsBox2D &box)
{
	return GetVector2D( box.GetMin(), box.GetMax() );
}

vsVector3D
vsRandomSource::GetVector3D(const vsBox3D &box)
{
	return GetVector3D( box.GetMin(), box.GetMax() );
}


vsColor
vsRandomSource::GetColor(float min, float max)
{
	vsVector3D result( GetFloat(0.f,100.f), GetFloat(0.f,100.f), GetFloat(0.f,100.f) );

	if ( result.SqLength() > 1.0f )
		result.Normalise();

	float brightness = GetFloat(min,max);
	result *= brightness;

	return vsColor( result.x, result.y, result.z, 1.0f );
}


