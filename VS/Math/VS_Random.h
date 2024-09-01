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

#include "VS/Graphics/VS_Color.h"
#include "VS/Math/VS_Vector.h"
#include "VS/Math/VS_Box.h"
#include "VS/Utils/VS_String.h"


class vsRandomSource
{
	uint64_t s[4];
	int p;

	uint64_t Next();
public:

	vsRandomSource();
	~vsRandomSource();

	void Init();
	void InitWithSeed(uint64_t seed);
	void InitWithSeed(const vsString & seed);

	float GetFloat(float maxValue);
	float GetFloat(float min, float max);
	int GetInt(int max);			// will return an integer in the range [0..max-1]
	int GetInt(int min, int max);	// will return an integer in the range [min..max]
	bool GetBool() { return !!GetInt(2); }

	vsVector2D GetVector2D(float maxLength);
	vsVector3D GetVector3D(float maxLength);
	vsVector2D GetVector2D(float minLength, float maxLength);
	vsVector3D GetVector3D(float minLength, float maxLength);
	vsVector3D GetVector3DInCone(const vsVector3D& forward, float angleRadians);
	vsColor GetColor(float minBrightness, float maxBrightness);

	vsVector2D GetVector2D(const vsVector2D &topLeft, const vsVector2D &bottomRight);
	vsVector3D GetVector3D(const vsVector3D &topLeft, const vsVector3D &bottomRight);

	vsVector2D GetVector2D(const vsBox2D &box);
	vsVector3D GetVector3D(const vsBox3D &box);

	static vsRandomSource Default;
};

class vsRandom
{
public:
	static void Init() { vsRandomSource::Default.Init(); }
	static void InitWithSeed(uint32_t seed) { vsRandomSource::Default.InitWithSeed(seed); }
	static void InitWithSeed(const vsString & seed) { vsRandomSource::Default.InitWithSeed(seed); }

	static float GetFloat(float maxValue) { return vsRandomSource::Default.GetFloat(maxValue); }
	static float GetFloat(float min, float max) { return vsRandomSource::Default.GetFloat(min, max); }

	// will return an integer in the range [0..max-1]
	static int			GetInt(int max) { return vsRandomSource::Default.GetInt(max); }

	// will return an integer in the range [min..max]
	static int			GetInt(int min, int max) { return vsRandomSource::Default.GetInt(min,max); }
	static bool			GetBool() { return !!GetInt(2); }

	static vsVector2D	GetVector2D(float maxLength) { return vsRandomSource::Default.GetVector2D(maxLength); }
	static vsVector3D	GetVector3D(float maxLength) { return vsRandomSource::Default.GetVector3D(maxLength); }
	static vsVector2D	GetVector2D(float minLength, float maxLength) { return vsRandomSource::Default.GetVector2D(minLength, maxLength); }
	static vsVector3D	GetVector3D(float minLength, float maxLength) { return vsRandomSource::Default.GetVector3D(minLength, maxLength); }
	static vsVector3D	GetVector3DInCone(const vsVector3D& forward, float angleRadians) { return vsRandomSource::Default.GetVector3DInCone(forward, angleRadians); }
	static vsColor		GetColor(float minBrightness, float maxBrightness) { return vsRandomSource::Default.GetColor( minBrightness, maxBrightness ); }

	static vsVector2D	GetVector2D(const vsVector2D &topLeft, const vsVector2D &bottomRight) { return vsRandomSource::Default.GetVector2D( topLeft, bottomRight ); }
	static vsVector3D	GetVector3D(const vsVector3D &topLeft, const vsVector3D &bottomRight) { return vsRandomSource::Default.GetVector3D( topLeft, bottomRight ); }

	static vsVector2D	GetVector2D(const vsBox2D &box) { return vsRandomSource::Default.GetVector2D(box); }
	static vsVector3D	GetVector3D(const vsBox3D &box) { return vsRandomSource::Default.GetVector3D(box); }

	friend class vsRandomSource;
};

#endif // VS_RANDOM_H
