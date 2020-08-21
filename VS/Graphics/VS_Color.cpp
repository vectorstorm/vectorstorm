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
vsColor::FromUInt64(uint64_t c)
{
	uint16_t *cp = reinterpret_cast<uint16_t*>(&c);
	return vsColor(cp[0]/65535.f, cp[1]/65535.f, cp[2]/65535.f, cp[3]/65535.f);
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

static float hue2rgb(float p, float q, float t)
{
	if(t < 0) t += 1;
	if(t > 1) t -= 1;
	if(t < 1.f/6.f) return p + (q - p) * 6 * t;
	if(t < 1.f/2.f) return q;
	if(t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6;
	return p;
}

vsColor::vsColor(const vsColorHSL& in)
{
	// This code (and its utility function 'hue2rgb', above) is adapted from
	// the HSL -> RGB conversion code provided here:
	//
	// http://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
	if(in.s == 0)
	{
		r = g = b = in.l; // achromatic
	}
	else
	{
		float q = in.l < 0.5 ? in.l * (1.f + in.s) : in.l + in.s - in.l * in.s;
		float p = 2.f * in.l - q;
		r = hue2rgb(p, q, in.h + 1.f/3.f);
		g = hue2rgb(p, q, in.h);
		b = hue2rgb(p, q, in.h - 1.f/3.f);
	}
	a = in.a;
}

vsColor
vsColor::FromHSV(float hue, float saturation, float value)
{
	vsColorHSV chsv(hue,saturation,value,1.f);
	return vsColor(chsv);
}

vsColor
vsColor::FromHSL(float hue, float saturation, float lightness)
{
	vsColorHSL chsl(hue,saturation,lightness,1.f);
	return vsColor(chsl);
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

uint64_t
vsColor::AsUInt64() const
{
	uint64_t rgba =
		static_cast<uint64_t>(r * 65535.f) |
		static_cast<uint64_t>(g * 65535.f) << 16 |
		static_cast<uint64_t>(b * 65535.f) << 32 |
		static_cast<uint64_t>(a * 65535.f) << 48;

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

vsColorHSL::vsColorHSL():
	h(0.f),
	s(0.f),
	l(0.f),
	a(0.f)
{
}

vsColorHSL::vsColorHSL( float h, float s, float l, float a ):
	h(h),
	s(s),
	l(l),
	a(a)
{
}

vsColorHSL::vsColorHSL( const vsColor& in )
{
	float M = vsMax( in.r, vsMax( in.g, in.b ) );
	float m = vsMin( in.r, vsMin( in.g, in.b ) );

	a = in.a;
	l = 0.5f * (M+m); // lightness is average of min and max color component.
	float delta = M - m;
	if (delta < 0.00001f)
	{
		s = 0.f;
		h = 0.f; // undefined, maybe nan?
	}
	else
	{
		// always true unless our color components are negative
		if( M > 0.f )
		{
			s = delta;
			s /= 1.f - vsFabs(2.f * l - 1);

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

float
vsColorHSL::SaturationToChroma(float saturation, float lightness)
{
	if ( lightness == 1.f )
		return 0.f;

	float maximumSaturation = (1.f - vsFabs(2.f * lightness - 1.f) );
	float chroma = saturation / maximumSaturation;
	chroma = vsClamp(0.f, 1.f, chroma);

	return chroma;
}

float
vsColorHSL::ChromaToSaturation(float chroma, float lightness)
{
	if ( lightness == 1.f )
		return 0.f;

	float maximumSaturation = (1.f - vsFabs(2.f * lightness - 1.f) );
	float saturation = chroma * maximumSaturation;
	return saturation;
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
