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

class vsPackedColor;
class vsColor
{
public:
	float r, g, b, a;

	static vsColor PureWhite;
	static vsColor White;
	static vsColor Grey;
	static vsColor Blue;
	static vsColor Green;
	static vsColor LightGreen;
	static vsColor Yellow;
	static vsColor Orange;
	static vsColor LightBlue;
	static vsColor DarkBlue;
	static vsColor Red;
	static vsColor Purple;
	static vsColor Black;
	static vsColor Clear;

	vsColor(float red=0.f, float green=0.f, float blue=0.f, float alpha=1.f) { r=red; g=green; b=blue; a=alpha; }
	vsColor(const vsPackedColor& other);

	static vsColor FromHSV(float hue, float saturation, float value);
	float GetHue() const;
	float GetSaturation() const;
	float GetValue() const;

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

class vsPackedColor
{
public:
	uint8_t r, g, b, a;

	vsPackedColor();
	vsPackedColor(const vsColor& other);
	void Set(float red=0.f, float green=0.f, float blue=0.f, float alpha=1.f) { *this = vsColor(red,green,blue,alpha); };
};

vsColor  operator*( float scalar, const vsColor &color );
vsColor vsInterpolate( float alpha, const vsColor &a, const vsColor &b );

#endif // VS_COLOR_H
