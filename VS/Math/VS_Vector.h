/*
 *  VS_Vector.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_VECTOR_H
#define VS_VECTOR_H

#include "VS/Math/VS_Math.h"

class vsVector2D;
class vsVector3D;
class vsVector4D;

		//  Standard 2D vector type
class vsVector2D
{
public:
	static vsVector2D Zero;
	static vsVector2D One;
	static vsVector2D XAxis;
	static vsVector2D YAxis;

	float x;
	float y;

	vsVector2D(): x(0.f), y(0.f) {}
	vsVector2D(const vsVector3D &b);
	vsVector2D(float x_in, float y_in): x(x_in), y(y_in) {}

	//vsVector2D	operator=( const vsVector2D &b ) {x=b.x; y=b.y; return *this;}
	const vsVector2D  operator+( const vsVector2D &b ) const {return vsVector2D(x+b.x, y+b.y);}
	const vsVector2D  operator-( const vsVector2D &b ) const {return vsVector2D(x-b.x, y-b.y);}
	const vsVector2D  operator*( float b ) const {return vsVector2D(x*b,y*b);}
	vsVector2D  operator*=( float b ) {x*=b; y*=b; return *this;}
	const vsVector2D  operator/( float b ) const {return vsVector2D(x/b,y/b);}
	vsVector2D  operator/=( float b ) {x/=b; y/=b; return *this;}
	vsVector2D  operator+=( const vsVector2D &b ) {*this = *this+b; return *this; }
	vsVector2D  operator-=( const vsVector2D &b ) {*this = *this-b; return *this; }

	bool operator==(const vsVector2D &b) const { return (x == b.x && y == b.y); }
	bool operator!=(const vsVector2D &b) const { return !(*this==b); }

	float	ApproximateLength() const;	// approximates length, without using a square root operation.  Accurate to within about 8%.
	inline float	Length() const { return vsSqrt( SqLength() ); }
	inline float	SqLength() const { return (x*x+y*y); }
	float	Magnitude() const { return Length(); }
	float	SqMagnitude() const { return SqLength(); }
	void	Normalise();
	void	NormaliseSafe();
	const vsVector2D Normalised() const;
	const vsVector2D NormalisedSafe() const;

	float	Dot( const vsVector2D &b ) const { return ( x*b.x +
										   y*b.y ); }
	float	Cross( const vsVector2D &b ) const { return x*b.y - y*b.x; }

	void	Set(float x_in, float y_in) {x=x_in; y=y_in;}

	vsVector2D XY() const { return *this; }
	vsVector2D YX() const { return vsVector2D(y,x); }
	vsVector2D XX() const { return vsVector2D(x,x); }
	vsVector2D YY() const { return vsVector2D(y,y); }
};



// VectorStorm is a 2D game engine, but if you have some strange need for a 3D
// vector, we've got a ready-made one.
class vsVector3D
{
public:
	static vsVector3D Zero;
	static vsVector3D One;
	static vsVector3D XAxis;
	static vsVector3D YAxis;
	static vsVector3D ZAxis;

	float x;
	float y;
	float z;

	vsVector3D(): x(0.f),y(0.f),z(0.f) {}
	vsVector3D(const vsVector2D &b): x(b.x), y(b.y), z(0.f) {}
	vsVector3D(const vsVector4D &b);
	vsVector3D(float x_in, float y_in, float z_in): x(x_in), y(y_in), z(z_in) {}

	inline const vsVector3D  operator+( const vsVector3D &b ) const {return vsVector3D(x+b.x, y+b.y, z+b.z);}
	inline const vsVector3D  operator-( const vsVector3D &b ) const {return vsVector3D(x-b.x, y-b.y, z-b.z);}
	inline const vsVector3D  operator*( float b ) const {return vsVector3D(x*b,y*b,z*b);}
	inline const vsVector3D&  operator*=( float b ) {x*=b; y*=b; z*=b; return *this;}
	inline const vsVector3D  operator/( float b ) const {return vsVector3D(x/b,y/b,z/b);}
	inline const vsVector3D&  operator/=( float b ) {x/=b; y/=b; z/=b; return *this;}
	inline const vsVector3D&  operator+=( const vsVector3D &b ) {x+=b.x; y+=b.y; z+=b.z; return *this; }
	inline const vsVector3D&  operator-=( const vsVector3D &b ) {x-=b.x; y-=b.y; z-=b.z; return *this; }

	inline bool operator==(const vsVector3D &b) const { return (this->x == b.x && this->y == b.y && this->z == b.z); }
	inline bool operator!=(const vsVector3D &b) const { return !(*this==b); }
	float operator[](int i) const;

	inline float	Length() const { return vsSqrt( SqLength() ); }
	inline float	SqLength() const { return (x*x+y*y+z*z); }
	void	Normalise();
	void	NormaliseSafe();
	const vsVector3D Normalised() const;
	const vsVector3D NormalisedSafe() const;

	const vsVector3D	Cross( const vsVector3D &b ) const { return vsVector3D( y*b.z - z*b.y,
														   z*b.x - x*b.z,
														   x*b.y - y*b.x ); }

	float	Dot( const vsVector3D &b ) const { return ( x*b.x +
										   y*b.y +
										   z*b.z ); }

	void	Set(float x_in, float y_in, float z_in) {x=x_in; y=y_in;z=z_in;}

    void    Floor();

	// Some swizzle operators.
	const vsVector2D XY() const { return vsVector2D(x,y); }
	const vsVector2D XZ() const { return vsVector2D(x,z); }
};

class vsNormalPacked
{
	int32_t m_value;

	void _Store( const vsVector3D& v );
	vsVector3D _Extract();
public:

	vsNormalPacked();
	vsNormalPacked(float x, float y, float z);
	vsNormalPacked(const vsVector3D &normal);

	void Set( const vsVector3D &normal );
	void Set( float x, float y, float z );

	void operator=( const vsVector3D& o ) { return Set(o); }
	operator vsVector3D() { return _Extract(); }
};

class vsPosition3D
{
	int32_t	m_x;
	int32_t	m_y;
	int32_t	m_z;

public:

	vsPosition3D();
	vsPosition3D(float x, float y, float z);
	vsPosition3D(const vsVector3D &pos);

	const vsPosition3D operator+( const vsVector3D &b ) const;
	const vsPosition3D operator+=( const vsVector3D &b );
	const vsVector3D operator-( const vsPosition3D &b ) const;
};

// VectorStorm is a 2D game engine, but if you have some strange need for a 3D
// vector, we've got a ready-made one.
class vsVector4D
{
public:
	float x;
	float y;
	float z;
	float w;

	vsVector4D(): x(0.f), y(0.f), z(0.f), w(0.f) {}
	vsVector4D(const vsVector3D &b): x(b.x), y(b.y), z(b.z), w(0.f) {}
	vsVector4D(const vsVector3D &b, float w_in): x(b.x), y(b.y), z(b.z), w(w_in) {}
	// vsVector4D(const vsVector4D &b): x(b.x), y(b.y), z(b.z), w(b.w) {}
	vsVector4D(float x_in, float y_in, float z_in, float w_in): x(x_in), y(y_in), z(z_in), w(w_in) {}

	const vsVector4D  operator+( const vsVector4D &b ) const {return vsVector4D(x+b.x, y+b.y, z+b.z, w+b.w);}
	const vsVector4D  operator-( const vsVector4D &b ) const {return vsVector4D(x-b.x, y-b.y, z-b.z, w-b.w);}
	const vsVector4D  operator*( float b ) const {return vsVector4D(x*b,y*b,z*b,w*b);}
	const vsVector4D  operator*=( float b ) {x*=b; y*=b; z*=b; w*=b; return *this;}
	const vsVector4D  operator/( float b ) const {return vsVector4D(x/b,y/b,z/b,w/b);}
	const vsVector4D  operator/=( float b ) {x/=b; y/=b; z/=b; w/=b; return *this;}
	const vsVector4D  operator+=( const vsVector4D &b ) {*this = *this+b; return *this; }
	const vsVector4D  operator-=( const vsVector4D &b ) {*this = *this-b; return *this; }
	float &		operator[]( int n );

	bool operator==(const vsVector4D &b) const { return (this->x == b.x && this->y == b.y && this->z == b.z && this->w == b.w); }
	bool operator!=(const vsVector4D &b) const { return !(*this==b); }

	inline float	Length() const { return vsSqrt( SqLength() ); }
	inline float	SqLength() const { return (x*x+y*y+z*z+w*w); }
	void	Normalise();
	const vsVector4D Normalised() const;

	float	Dot( const vsVector4D &b ) const { return ( x*b.x +
													   y*b.y +
													   z*b.z +
													   w*b.w); }

	void	Set(float x_in, float y_in, float z_in, float w_in) {x=x_in; y=y_in;z=z_in;w=w_in;}
};




const vsVector2D operator*(float b, const vsVector2D &vec);
const vsVector3D operator*(float b, const vsVector3D &vec);
const vsVector3D operator*(float b, const vsVector4D &vec);

const vsVector2D operator-(const vsVector2D &vec);
const vsVector3D operator-(const vsVector3D &vec);
const vsVector3D operator-(const vsVector4D &vec);

const vsVector2D vsInterpolate( float alpha, const vsVector2D &a, const vsVector2D &b );
const vsVector3D vsInterpolate( float alpha, const vsVector3D &a, const vsVector3D &b );
const vsVector4D vsInterpolate( float alpha, const vsVector4D &a, const vsVector4D &b );

#endif // VS_VECTOR_H
