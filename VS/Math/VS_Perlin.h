/*
 *  VS_Perlin.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 8/06/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_PERLIN_H
#define VS_PERLIN_H

#include "VS_Vector.h"

class vsPerlinOctave
{
	int		m_a;
	int		m_b;
	int		m_c;
public:

			vsPerlinOctave();

	float	Noise1D(int x);
	float	SmoothedNoise1D(int x);
	float	InterpolatedNoise1D(float x);

	float	Noise2D(int x, int y, int wrap);
	float	SmoothedNoise2D(int x, int y, int wrap);
	float	InterpolatedNoise2D(float x, float y, int wrap);
};



class vsPerlin
{
	vsPerlinOctave **	m_octave;
	int					m_octaveCount;
	float				m_persistence;
	float				m_invTotalPossible;

	float				m_wrap;

public:

		// "octaves" specifies how many noise channels should be combined to
		// create the noise data.  Legal values: 1..inf.  (linear increase in
		// computation time as this increases).  Each 'octave' posesses twice
		// as much detail as the previous.  (That is, the first octave reaches
		// a new value at [0, 1, 2, 3...], while the second reaches a new value
		// at [0, 0.5, 1, 1.5, 2...], and the third reaches a new value at
		// [0, 0.25, 0.5, 0.75, 1...].  These octaves get combined together
		// to produce the noise output.
		//
		// "persistance" tells how the different octaves influence the final
		// noise data.  With a persistance value of 1, all have full effect.
		// At 0, only the first octave is used.  Good values are usually in the
		// range of 0.1 to 0.7, but this will vary a lot based upon how large
		// (and how dense) a sample of data you'll be taking from the noise
		// function.
		//
		// "wrap", if non-zero, specifies the value at which noise should
		// "wrap around" on each axis.
	vsPerlin(int octaves, float persistance, float wrap = 0.f);
	~vsPerlin();

	float	Noise( const vsVector2D &pos );		// returns [-1..1]
	float	Noise( float time );				// returns [-1..1]
};




#endif // VS_PERLIN_H
