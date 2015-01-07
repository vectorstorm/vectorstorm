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
	uint32_t m_Q[4096];
	uint32_t m_c;
	uint32_t m_i;

	uint32_t Int32();
public:
	vsRandomSource();
	~vsRandomSource();

	void Init();
	void InitWithSeed(uint32_t seed);
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
	vsColor GetColor(float minBrightness, float maxBrightness);

	vsVector2D GetVector2D(const vsVector2D &topLeft, const vsVector2D &bottomRight);
	vsVector3D GetVector3D(const vsVector3D &topLeft, const vsVector3D &bottomRight);

	vsVector2D GetVector2D(const vsBox2D &box);
	vsVector3D GetVector3D(const vsBox3D &box);
};

class vsRandom
{
	static vsRandomSource s_source;
public:
	static void Init() { s_source.Init(); }
	static void InitWithSeed(uint32_t seed) { s_source.InitWithSeed(seed); }
	static void InitWithSeed(const vsString & seed) { s_source.InitWithSeed(seed); }

	static float GetFloat(float maxValue) { return s_source.GetFloat(maxValue); }
	static float GetFloat(float min, float max) { return s_source.GetFloat(min, max); }

	// will return an integer in the range [0..max-1]
	static int			GetInt(int max) { return s_source.GetInt(max); }

	// will return an integer in the range [min..max]
	static int			GetInt(int min, int max) { return s_source.GetInt(min,max); }
	static bool			GetBool() { return !!GetInt(2); }

	static vsVector2D	GetVector2D(float maxLength) { return s_source.GetVector2D(maxLength); }
	static vsVector3D	GetVector3D(float maxLength) { return s_source.GetVector3D(maxLength); }
	static vsVector2D	GetVector2D(float minLength, float maxLength) { return s_source.GetVector2D(minLength, maxLength); }
	static vsVector3D	GetVector3D(float minLength, float maxLength) { return s_source.GetVector3D(minLength, maxLength); }
	static vsColor		GetColor(float minBrightness, float maxBrightness) { return s_source.GetColor( minBrightness, maxBrightness ); }

	static vsVector2D	GetVector2D(const vsVector2D &topLeft, const vsVector2D &bottomRight) { return s_source.GetVector2D( topLeft, bottomRight ); }
	static vsVector3D	GetVector3D(const vsVector3D &topLeft, const vsVector3D &bottomRight) { return s_source.GetVector3D( topLeft, bottomRight ); }

	static vsVector2D	GetVector2D(const vsBox2D &box) { return s_source.GetVector2D(box); }
	static vsVector3D	GetVector3D(const vsBox3D &box) { return s_source.GetVector3D(box); }

	friend class vsRandomSource;
};

#endif // VS_RANDOM_H
