/*
 *  VS_Random.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RANDOM_H
#define VS_RANDOM_H

class vsBox2D;
class vsBox3D;
class vsColor;
class vsVector2D;
class vsVector3D;

class vsRandom
{
public:
	static void Init();
	static void InitWithSeed(uint32_t seed);
	static void InitWithSeed(const vsString & seed);

	static float GetFloat(float maxValue);
	static float GetFloat(float min, float max);
	static int			GetInt(int max);			// will return an integer in the range [0..max-1]
	static int			GetInt(int min, int max);	// will return an integer in the range [min..max]
	static bool			GetBool() { return !!GetInt(2); }

	static vsVector2D	GetVector2D(float maxLength);
	static vsVector3D	GetVector3D(float maxLength);
	static vsVector2D	GetVector2D(float minLength, float maxLength);
	static vsVector3D	GetVector3D(float minLength, float maxLength);
	static vsColor		GetColor(float minBrightness, float maxBrightness);

	static vsVector2D	GetVector2D(const vsVector2D &topLeft, const vsVector2D &bottomRight);
	static vsVector3D	GetVector3D(const vsVector3D &topLeft, const vsVector3D &bottomRight);

	static vsVector2D	GetVector2D(const vsBox2D &box);
	static vsVector3D	GetVector3D(const vsBox3D &box);
};

#endif // VS_RANDOM_H
