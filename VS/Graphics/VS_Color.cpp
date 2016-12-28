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

vsColor::vsColor(const vsColorPacked& packed):
	r(packed.r/255.f),
	g(packed.g/255.f),
	b(packed.b/255.f),
	a(packed.a/255.f)
{
}

vsColor::vsColor(const vsColorHSV& in)
{
	float hh, p, q, t, ff;
	long i;

	if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
		r = in.v;
		g = in.v;
		b = in.v;
		a = in.a;
		return;
	}
	hh = in.h * 360.f;
	if(hh >= 360.f) hh = 0.0;
	hh /= 60.f;
	i = vsFloor(hh);
	ff = hh - i;
	p = in.v * (1.f - in.s);
	q = in.v * (1.f - (in.s * ff));
	t = in.v * (1.f - (in.s * (1.f - ff)));

	switch(i) {
		case 0:
			r = in.v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = in.v;
			b = p;
			break;
		case 2:
			r = p;
			g = in.v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = in.v;
			break;
		case 4:
			r = t;
			g = p;
			b = in.v;
			break;
		case 5:
		default:
			r = in.v;
			g = p;
			b = q;
			break;
	}
	a = in.a;
}

vsColor
vsColor::FromHSV(float hue, float saturation, float value)
{
	vsColorHSV chsv(hue,saturation,value,1.f);
	return vsColor(chsv);
}

float
vsColor::GetHue() const
{
	vsColorHSV hsvc(*this);
	return hsvc.h;
}

float
vsColor::GetSaturation() const
{
	vsColorHSV hsvc(*this);
	return hsvc.s;
}

float
vsColor::GetValue() const
{
	vsColorHSV hsvc(*this);
	return hsvc.v;
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

vsColorHSV::vsColorHSV():
	h(0.f),
	s(0.f),
	v(0.f),
	a(0.f)
{
}

vsColorHSV::vsColorHSV( float h_in, float s_in, float v_in, float a_in ):
	h(h_in),
	s(s_in),
	v(v_in),
	a(a_in)
{
}

vsColorHSV::vsColorHSV( const vsColor& in )
{
	float M = vsMax( in.r, vsMax( in.g, in.b ) );
	float m = vsMin( in.r, vsMin( in.g, in.b ) );

	a = in.a;
	v = M;                                // v
	float delta = M - m;
	if (delta < 0.00001f)
	{
		s = 0.f;
		h = 0.f; // undefined, maybe nan?
	}
	else
	{
		if( M > 0.f ) { // always true unless some of our color components are negative
			s = (delta / M);

			if( in.r >= M )                           // > is bogus, just keeps compilor happy
				h = ( in.g - in.b ) / delta;        // between yellow & magenta
			else
				if( in.g >= M )
					h = 2.f + ( in.b - in.r ) / delta;  // between cyan & yellow
				else
					h = 4.f + ( in.r - in.g ) / delta;  // between magenta & cyan

			h *= 60.f;                              // degrees

			if( h < 0.f )
				h += 360.f;

			h = h / 360.f;
		} else {
			// if M is 0, then r = g = b = 0
			// s = 0, v is undefined
			s = 0.f;
			h = 0.f;
		}
	}
}

vsColorPacked::vsColorPacked(const vsColor& c):
	r(c.r * 255),
	g(c.g * 255),
	b(c.b * 255),
	a(c.a * 255)
{
}

vsColor
vsColorPacked::operator+( const vsColor &o ) const
{
	return vsColor(*this) + o;
}
