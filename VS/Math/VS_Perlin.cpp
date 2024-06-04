/*
 *  VS_Perlin.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 8/06/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Perlin.h"

#include "VS_Random.h"

vsPerlinOctave::vsPerlinOctave()
{
	m_a = 15731;
	m_b = 789221;
	m_c = 1376312589;

	m_a = vsRandom::GetInt(5000,40000);
	m_b = vsRandom::GetInt(500000,1000000);
	m_c = vsRandom::GetInt(100000000,2000000000);
}

float
vsPerlinOctave::Noise1D(int x)		// returns a value in the range [ -1 .. 1 ]
{
	int n = x;
	n = (n<<13) ^ n;
	return (float)( 1.0f - ( (n * (n * n * m_a + m_b) + m_c) & 0x7fffffff) / 1073741824.0f);

	// my random integer which is going to be a POSITIVE 32-bit integer.
	int part = ( (n * (n * n * m_a + m_b) + m_c) & 0x7fffffff);
	return (float)( 1.0f - part / 1073741824.0f);
}

float
vsPerlinOctave::SmoothedNoise1D(int x)
{
	float sides   = (Noise1D(x-1) + Noise1D(x+1)) /  4.f;
	float center  =  Noise1D(x) / 2.f;
	return sides + center;
}

float
vsPerlinOctave::InterpolatedNoise1D(float x)
{
	int integer_X		= vsFloor(x);
	float fractional_X	= x - integer_X;

	vsAssert( !vsIsNaN(x), "Maths error: vsPerlinOctave::InterpolatedNoise2D called with NaN" );
	vsAssertF( fractional_X >= 0.f && fractional_X < 1.f, "Maths error:  fractional_X out of bounds:  input x=%f, fractional_X=%f", x, fractional_X );

	fractional_X = (3.0f * fractional_X * fractional_X) - (2.0f * fractional_X * fractional_X * fractional_X);

	float v1 = SmoothedNoise1D(integer_X);
	float v2 = SmoothedNoise1D(integer_X + 1);

	return vsInterpolate(fractional_X, v1 , v2);
}


float
vsPerlinOctave::Noise2D(int x, int y, int wrap)
{
	if ( wrap != 0 )
	{
		while ( x < 0 )
		{
			x += wrap;
		}
		while ( y < 0 )
		{
			y += wrap;
		}
		x = x % wrap;
		y = y % wrap;
	}

	unsigned int n = x + (y * 57);
	n = (n<<13) ^ n;
	int intPart = ( (n * (n * n * m_a + m_b) + m_c) & 0x7fffffff); // [ 0 .. 2^31 ]
	float outVal = (float)( 1.0f -  intPart / 1073741824.0f); // [ -1 .. 1 ]
	return outVal;
}

float
vsPerlinOctave::SmoothedNoise2D(int x, int y, int wrap)
{
	float corners = ( Noise2D(x-1, y-1, wrap)+Noise2D(x+1, y-1, wrap)+Noise2D(x-1, y+1, wrap)+Noise2D(x+1, y+1, wrap) ) / 16.f;
	float sides   = ( Noise2D(x-1, y, wrap)  +Noise2D(x+1, y, wrap)  +Noise2D(x, y-1, wrap)  +Noise2D(x, y+1, wrap) ) /  8.f;
	float center  =  Noise2D(x, y, wrap) / 4.f;
	return corners + sides + center;
}

float
vsPerlinOctave::InterpolatedNoise2D(float x, float y, int wrap)
{
	int integer_X    = vsFloor(x);
	float fractional_X = x - integer_X;

	int integer_Y    = vsFloor(y);
	float fractional_Y = y - integer_Y;

	vsAssert( !vsIsNaN(x), "Maths error: vsPerlinOctave::InterpolatedNoise2D called with NaN" );
	vsAssert( !vsIsNaN(y), "Maths error: vsPerlinOctave::InterpolatedNoise2D called with NaN" );
	vsAssertF( fractional_X >= 0.f && fractional_X < 1.f, "Maths error:  fractional_X out of bounds:  input x=%f, fractional_X=%f", x, fractional_X );
	vsAssertF( fractional_Y >= 0.f && fractional_Y < 1.f, "Maths error:  fractional_Y out of bounds:  input y=%f, fractional_Y=%f", y, fractional_Y );

	fractional_X = (3.0f * fractional_X * fractional_X) - (2.0f * fractional_X * fractional_X * fractional_X);
	fractional_Y = (3.0f * fractional_Y * fractional_Y) - (2.0f * fractional_Y * fractional_Y * fractional_Y);

	float v1 = Noise2D(integer_X,     integer_Y, wrap);
	float v2 = Noise2D(integer_X + 1, integer_Y, wrap);
	float v3 = Noise2D(integer_X,     integer_Y + 1, wrap);
	float v4 = Noise2D(integer_X + 1, integer_Y + 1, wrap);

	float i1 = vsInterpolate(fractional_X, v1 , v2);
	float i2 = vsInterpolate(fractional_X, v3 , v4);

	return vsInterpolate(fractional_Y, i1 , i2);
}

vsPerlin::vsPerlin(int octaves, float persistence, float wrap):
	m_octave(new vsPerlinOctave *[octaves]),
	m_octaveCount(octaves),
	m_persistence(persistence),
	m_invTotalPossible(0.f),
	m_wrap(wrap)
{
	float totalPossible = 0.f;
	float totalFromThisOctave = 1.f;

	for ( int i = 0; i < m_octaveCount; i++ )
	{
		m_octave[i] = new vsPerlinOctave;
		totalPossible += totalFromThisOctave;
		totalFromThisOctave *= persistence;
	}

	m_invTotalPossible = 1.f / totalPossible;
}

vsPerlin::~vsPerlin()
{
	for ( int i = 0; i < m_octaveCount; i++ )
	{
		vsDelete(m_octave[i]);
	}
	vsDeleteArray(m_octave);
}

float
vsPerlin::Noise(float time)
{
	float total = 0.f;
	float p = m_persistence;
	float amplitude = 1.f;

	for ( int i = 0; i < m_octaveCount; i++ )
	{
		float frequency = (float)(1 << i);

		total += m_octave[i]->InterpolatedNoise1D(time * frequency) * amplitude;

		amplitude *= p;
	}

	total *= m_invTotalPossible;

	return total;
}

float
vsPerlin::Noise(const vsVector2D &pos)
{
	float total = 0.f;
	float p = m_persistence;
	float amplitude = 1.f;

	for ( int i = 0; i < m_octaveCount; i++ )
	{
		float frequency = (float)(1 << i);

		total += m_octave[i]->InterpolatedNoise2D(pos.x * frequency, pos.y * frequency, (int)(m_wrap * frequency)) * amplitude;

		amplitude *= p;
	}

	// now, our result could be anywhere in -1.f .. 1.f
	total *= m_invTotalPossible;

	return total;
}

