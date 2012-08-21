/*
 *  VS_Color.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Color.h"

vsColor vsColor::PureWhite(1.0f,1.0f,1.0f,1.0f);
vsColor vsColor::White(1.0f,1.0f,1.0f,1.0f);
vsColor vsColor::Grey(0.5f,0.5f,0.5f,1.0f);
vsColor vsColor::Blue(0.2f,0.2f,1.0f,1.0f);
vsColor vsColor::LightBlue(0.5f,0.5f,1.0f,1.0f);
vsColor vsColor::DarkBlue(0.0f,0.0f,0.25f,1.0f);
vsColor vsColor::Red(1.0f,0.2f,0.2f,1.0f);
vsColor vsColor::Green(0.2f,1.0f,0.2f,1.0f);
vsColor vsColor::LightGreen(0.5f,1.0f,0.5f,1.0f);
vsColor vsColor::Yellow(1.0f,1.0f,0.2f,1.0f);
vsColor vsColor::Orange(1.0f,0.5f,0.2f,1.0f);
vsColor vsColor::Purple(1.0f,0.2f,1.0f,1.0f);
vsColor vsColor::Black(0.0f,0.0f,0.0f,1.0f);
vsColor vsColor::Clear(0.0f,0.0f,0.0f,0.0f);

vsColor
vsColor::FromBytes(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return vsColor(r/255.f, g/255.f, b/255.f, a/255.f);
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

	result += vsColor::PureWhite * (value - chroma);

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

vsColor  operator*( float scalar, const vsColor &color ) {return color * scalar;}


vsColor vsInterpolate( float alpha, const vsColor &a, const vsColor &b )
{
	return ((1.0f-alpha)*a) + (alpha*b);
}

