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
#include <limits>

vsRandomSource vsRandomSource::Default;

vsRandomSource::vsRandomSource()
{
	Init();
}

vsRandomSource::~vsRandomSource()
{
}

void
vsRandomSource::InitWithSeed( uint64_t seed )
{
	// our Xorshift and Xoshiro pseudo-random number generators need a non-zero
	// seed in order to produce useful output, so if we've been given a zero
	// seed, let's set it to some other arbitrary (but repeatable) value.

	if ( seed == 0 )
	{
		seed = 4; // chosen by fair dice roll.
				  // guaranteed to be random.
	}

	// We actually need a lot more state data than just this 64 bits -- we're
	// going to be using Xoshiro256**, which requires 256 bits of state.  So
	// we're going to use our 64 bits of seed data as the state of an
	// Xorshift64* PRNG, and use that to generate the initial state for our
	// real, full PRNG.
	//
	// This Xorshift64* implementation is adapted from here:
	//
	// http://xorshift.di.unimi.it/xorshift64star.c

	for ( int i = 0; i < 4; i++ )
	{
		seed ^= seed >> 12; // a
		seed ^= seed << 25; // b
		seed ^= seed >> 27; // c
		s[i] = seed * 2685821657736338717LL;
	}
	p = 0;
}

static inline uint64_t rotate64(const uint64_t x, int k)
{
	return (x << k) | (x >> (64-k));
}

uint64_t
vsRandomSource::Next()
{
	// This is the core of our pseudo-random number generation.  This function
	// generates 64 bits of pseudo-random data, which can then be consumed by
	// the other (externally visible) functions on this class.
	//
	// What follows is an implementation of xoshiro256**;  an xoshiro-based
	// PRNG which uses 256 bits of state, adapted from the implementation
	// here:
	//
	// http://xoshiro.di.unimi.it/xoshiro256starstar.c

	const uint64_t result = rotate64(s[1] * 5, 7) * 9;
	const uint64_t t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotate64(s[3], 45);

	return result;
}

void
vsRandomSource::Init()
{
	InitWithSeed( (uint64_t)time(nullptr) );
}

void
vsRandomSource::InitWithSeed( const vsString &seed )
{
	uint32_t val = 0;
	for ( size_t i = 0; i < seed.size(); i++ )
	{
		val += seed[0];
	}
	InitWithSeed(val);
}

int
vsRandomSource::GetInt(int maxValue)
{
	return Next() % maxValue;
}

int
vsRandomSource::GetInt(int min, int max)
{
	int delta = (max - min)+1;

	return GetInt(delta) + min;
}

float
vsRandomSource::GetFloat(float maxValue)
{
	float result = Next() / (float)std::numeric_limits<uint64_t>::max();
	result *= maxValue;
	return result;
}

float
vsRandomSource::GetFloat(float min, float max)
{
	float delta = max - min;

	return GetFloat(delta) + min;
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

vsVector3D
vsRandomSource::GetVector3DInCone(const vsVector3D& forward, float angleRadians)
{
	// Based upon an answer here:
	// https://math.stackexchange.com/questions/56784/generate-a-random-direction-within-a-cone

	float z = GetFloat( vsCos(angleRadians), 1.f );
	float theta = GetFloat( 0.f, TWOPI );

	vsVector3D result(
			vsSqrt(1.f - z*z) * vsCos(theta),
			vsSqrt(1.f - z*z) * vsSin(theta),
			z
			);
	// 'result' is the direction along a (0,0,1) forward.  We need to make it go
	// in the requested direction!

	vsVector3D up = vsVector3D::YAxis;
	if ( forward == up )
		up = vsVector3D::ZAxis;

	vsQuaternion q(forward, vsVector3D::YAxis);
	result = q.ApplyTo( result );
	return result;
}

