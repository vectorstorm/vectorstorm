/*
 *  VS_Color.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_COLOR_H
#define VS_COLOR_H

#include "Math/VS_Math.h"

class vsColorHSV;

class vsColor
{
public:
	float r, g, b, a;

	vsColor(float red=0.f, float green=0.f, float blue=0.f, float alpha=1.f) { r=red; g=green; b=blue; a=alpha; }
	vsColor(const vsColorHSV& hsv);

	static vsColor FromHSV(float hue, float saturation, float value);
	static vsColor FromBytes(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	static vsColor FromUInt32(uint32_t rgba);
	float GetHue() const;
	float GetSaturation() const;
	float GetValue() const;
	vsColorHSV GetHSV() const;
	uint32_t AsUInt32() const;

	vsColor  operator+( const vsColor &o ) const {return vsColor(r+o.r,g+o.g,b+o.b,a+o.a);}
	vsColor  operator-( const vsColor &o ) const {return vsColor(r-o.r,g-o.g,b-o.b,a-o.a);}
	vsColor & operator+=( const vsColor &o ) {r+=o.r; g+=o.g; b+=o.b; a+=o.a; return *this;}
	vsColor  operator*( float scalar ) const {return vsColor(r*scalar,g*scalar,b*scalar, a*scalar);}
	vsColor &  operator*=( float scalar ) {r*=scalar; g*=scalar; b*=scalar; a*=scalar; return *this;}
	vsColor  operator*( const vsColor &o ) const {return vsColor(r*o.r,g*o.g,b*o.b, a*o.a);}
	vsColor & operator*=( const vsColor &o ) {r*=o.r; g*=o.g; b*=o.b; a*=o.a; return *this;}

	bool	operator==( const vsColor &o ) const { return (r==o.r && b==o.b && g==o.g && a==o.a); }
	bool	operator!=( const vsColor &o ) const { return !(*this==o); }

	float	Magnitude() { return vsSqrt( r*r + g*g + b*b + a*a ); }

	void Set(float red=0.f, float green=0.f, float blue=0.f, float alpha=1.f) { r=red; g=green; b=blue; a=alpha; };
};

class vsColorHSV
{
public:
	float h, s, v, a;

	vsColorHSV();
	vsColorHSV( float h, float s, float v, float a );
	vsColorHSV( const vsColor& other );
};


extern const vsColor c_white;
extern const vsColor c_grey;
extern const vsColor c_blue;
extern const vsColor c_green;
extern const vsColor c_lightGreen;
extern const vsColor c_yellow;
extern const vsColor c_orange;
extern const vsColor c_lightBlue;
extern const vsColor c_darkBlue;
extern const vsColor c_red;
extern const vsColor c_purple;
extern const vsColor c_black;
extern const vsColor c_clear;

vsColor  operator*( float scalar, const vsColor &color );
vsColor vsInterpolate( float alpha, const vsColor &a, const vsColor &b );
vsColor vsInterpolateHSV( float alpha, const vsColor &a, const vsColor &b );

#endif // VS_COLOR_H
