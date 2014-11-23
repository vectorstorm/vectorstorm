/*
 *  VS_Color.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Color.h"
#include "VS_Vector.h"

const vsColor c_white(1.0f,1.0f,1.0f,1.0f);
const vsColor c_grey(0.5f,0.5f,0.5f,1.0f);
const vsColor c_blue(0.2f,0.2f,1.0f,1.0f);
const vsColor c_lightBlue(0.5f,0.5f,1.0f,1.0f);
const vsColor c_darkBlue(0.0f,0.0f,0.25f,1.0f);
const vsColor c_red(1.0f,0.2f,0.2f,1.0f);
const vsColor c_green(0.2f,1.0f,0.2f,1.0f);
const vsColor c_lightGreen(0.5f,1.0f,0.5f,1.0f);
const vsColor c_yellow(1.0f,1.0f,0.2f,1.0f);
const vsColor c_orange(1.0f,0.5f,0.2f,1.0f);
const vsColor c_purple(1.0f,0.2f,1.0f,1.0f);
const vsColor c_black(0.0f,0.0f,0.0f,1.0f);
const vsColor c_clear(0.0f,0.0f,0.0f,0.0f);

vsColor
vsColor::FromBytes(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return vsColor(r/255.f, g/255.f, b/255.f, a/255.f);
}

vsColor
vsColor::FromUInt32(uint32_t c)
{
	uint8_t *cp = reinterpret_cast<uint8_t*>(&c);

	return FromBytes(cp[0], cp[1], cp[2], cp[3]);
}

vsColor
vsColor::FromHSV(float hue, float saturation, float value)
{
	float chroma = value * saturation;
	float hueSegment = hue * 6.f;
	int hueSegmentInt = vsFloor(hueSegment);
	float hueSegmentModTwo = fmodf(hueSegment, 2.0f);
	float secondaryChroma = chroma * ( 1 - vsFabs(hueSegmentModTwo - 1) );

	vsColor result;

	switch ( hueSegmentInt )
	{
		case 0:
			result = vsColor(chroma, secondaryChroma, 0.f, 1.f);
			break;
		case 1:
			result = vsColor(secondaryChroma, chroma, 0.f, 1.f);
			break;
		case 2:
			result = vsColor(0.f, chroma, secondaryChroma, 1.f);
			break;
		case 3:
			result = vsColor(0.f, secondaryChroma, chroma, 1.f);
			break;
		case 4:
			result = vsColor(secondaryChroma, 0.f, chroma, 1.f);
			break;
		case 5:
		default:
			result = vsColor(chroma, 0.f, secondaryChroma, 1.f);
			break;
	}

	result += c_white * (value - chroma);

	return result;
}
float
vsColor::GetHue() const
{
	float M = vsMax( r, vsMax( g, b ) );
	float m = vsMin( r, vsMin( g, b ) );
	float C = M - m;
	float hueSegment = 0.f;

	if ( C == 0.f )
		hueSegment = 0.f;
	if ( M == r )
	{
		hueSegment = fmodf( (g-b) / C, 6.f );
	}
	else if ( M == g )
	{
		hueSegment = (b-r)/C + 2;
	}
	else if ( M == b )
	{
		hueSegment = (r-g)/C + 4;
	}
	return hueSegment / 6.f;
}

float
vsColor::GetSaturation() const
{
	float M = vsMax( r, vsMax( g, b ) );
	float m = vsMin( r, vsMin( g, b ) );
	float C = M - m;

	float V = GetValue();

	if ( C == 0.f )
		return 0.f;
	else
	{
		return C / V;
	}
}

float
vsColor::GetValue() const
{
	return 0.3333f * (r+g+b);
}

uint32_t
vsColor::AsUInt32() const
{
	uint32_t rgba =
		static_cast<uint8_t>(r * 255.f) |
		static_cast<uint8_t>(g * 255.f) << 8 |
		static_cast<uint8_t>(b * 255.f) << 16 |
		static_cast<uint8_t>(a * 255.f) << 24;

	return rgba;
}

vsColor  operator*( float scalar, const vsColor &color ) {return color * scalar;}


vsColor vsInterpolate( float alpha, const vsColor &a, const vsColor &b )
{
	return ((1.0f-alpha)*a) + (alpha*b);
}

vsColor vsInterpolateHSV( float alpha, const vsColor &a, const vsColor &b )
{
	float aHue = a.GetHue();
	float bHue = b.GetHue();
	float hue;
	if ( fabs(bHue - aHue) < 0.5f )
	{
		hue = vsInterpolate(alpha, aHue, bHue);
	}
	else
	{
		if ( aHue > bHue )
			aHue -= 1.f;
		else
			bHue -= 1.f;

		hue = vsInterpolate(alpha, aHue, bHue);

		// now verify that 'hue' is inside [0..1]
		if ( hue < 1.f )
			hue += 1.f;
	}
	float sat = vsInterpolate(alpha, a.GetSaturation(), b.GetSaturation());
	float val = vsInterpolate(alpha, a.GetValue(), b.GetValue());
	return vsColor::FromHSV(hue, sat, val);
}

