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

class vsVector2D;
class vsVector3D;
class vsVector4D;

		//  Standard 2D vector type
class vsVector2D
{
public:
	static vsVector2D Zero;
	static vsVector2D One;
	
	float x;
	float y;
	
	vsVector2D() {x=0;y=0;}
	vsVector2D(const vsVector3D &b);
	vsVector2D(float x_in, float y_in) { x = x_in; y = y_in; }
	
	//vsVector2D	operator=( const vsVector2D &b ) {x=b.x; y=b.y; return *this;}
	vsVector2D  operator+( const vsVector2D &b ) const {return vsVector2D(x+b.x, y+b.y);}
	vsVector2D  operator-( const vsVector2D &b ) const {return vsVector2D(x-b.x, y-b.y);}
	vsVector2D  operator*( float b ) const {return vsVector2D(x*b,y*b);}
	vsVector2D  operator*=( float b ) {x*=b; y*=b; return *this;}
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
	
	float	Dot( const vsVector2D &b ) const { return ( x*b.x +
										   y*b.y ); }
	
	void	Set(float x_in, float y_in) {x=x_in; y=y_in;}
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
	
	vsVector3D() {x=0;y=0;z=0;}
	vsVector3D(const vsVector2D &b) {x=b.x;y=b.y;z=0;}
	vsVector3D(const vsVector4D &b);
	vsVector3D(float x_in, float y_in, float z_in) { x = x_in; y = y_in; z = z_in; }
	
	inline vsVector3D&	operator=( const vsVector3D &b ) {x=b.x; y=b.y; z = b.z; return *this;}
	inline vsVector3D  operator+( const vsVector3D &b ) const {return vsVector3D(x+b.x, y+b.y, z+b.z);}
	inline vsVector3D  operator-( const vsVector3D &b ) const {return vsVector3D(x-b.x, y-b.y, z-b.z);}
	inline vsVector3D  operator*( float b ) const {return vsVector3D(x*b,y*b,z*b);}
	inline vsVector3D&  operator*=( float b ) {x*=b; y*=b; z*=b; return *this;}
	inline vsVector3D&  operator+=( const vsVector3D &b ) {x+=b.x; y+=b.y; z+=b.z; return *this; }
	inline vsVector3D&  operator-=( const vsVector3D &b ) {x-=b.x; y-=b.y; z-=b.z; return *this; }
	
	inline bool operator==(const vsVector3D &b) const { return (this->x == b.x && this->y == b.y && this->z == b.z); }
	inline bool operator!=(const vsVector3D &b) const { return !(*this==b); }
	float operator[](int i) const;
	
	inline float	Length() const { return vsSqrt( SqLength() ); }
	inline float	SqLength() const { return (x*x+y*y+z*z); }
	void	Normalise();
	
	vsVector3D	Cross( const vsVector3D &b ) const { return vsVector3D( y*b.z - z*b.y,
														   z*b.x - x*b.z,
														   x*b.y - y*b.x ); }
	
	float	Dot( const vsVector3D &b ) const { return ( x*b.x +
										   y*b.y +
										   z*b.z ); }
	
	void	Set(float x_in, float y_in, float z_in) {x=x_in; y=y_in;z=z_in;}
    
    void    Floor();
};

class vsPosition3D
{
	int32	m_x;
	int32	m_y;
	int32	m_z;
public:
	
	vsPosition3D();
	vsPosition3D(float x, float y, float z);
	vsPosition3D(const vsVector3D &pos);
	
	vsPosition3D operator+( const vsVector3D &b ) const;
	vsPosition3D operator+=( const vsVector3D &b );
	vsVector3D operator-( const vsPosition3D &b ) const;
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
	
	vsVector4D() {x=0;y=0;z=0;w=0;}
	vsVector4D(const vsVector3D &b) {x=b.x;y=b.y;z=b.z;w=0;}
	vsVector4D(float x_in, float y_in, float z_in, float w_in) { x = x_in; y = y_in; z = z_in; w = w_in;}
	
	vsVector4D	operator=( const vsVector4D &b ) {x=b.x; y=b.y; z = b.z; w = b.w; return *this;}
	vsVector4D  operator+( const vsVector4D &b ) const {return vsVector4D(x+b.x, y+b.y, z+b.z, w+b.w);}
	vsVector4D  operator-( const vsVector4D &b ) const {return vsVector4D(x-b.x, y-b.y, z-b.z, w-b.w);}
	vsVector4D  operator*( float b ) const {return vsVector4D(x*b,y*b,z*b,w*b);}
	vsVector4D  operator*=( float b ) {x*=b; y*=b; z*=b; w*=b; return *this;}
	vsVector4D  operator+=( const vsVector4D &b ) {*this = *this+b; return *this; }
	vsVector4D  operator-=( const vsVector4D &b ) {*this = *this-b; return *this; }
	float &		operator[]( int n );
	
	bool operator==(const vsVector4D &b) const { return (this->x == b.x && this->y == b.y && this->z == b.z && this->w == b.w); }
	bool operator!=(const vsVector4D &b) const { return !(*this==b); }
	
	inline float	Length() const { return vsSqrt( SqLength() ); }
	inline float	SqLength() const { return (x*x+y*y+z*z+w*w); }
	void	Normalise();
	
	float	Dot( const vsVector4D &b ) const { return ( x*b.x +
													   y*b.y +
													   z*b.z +
													   w*b.w); }
	
	void	Set(float x_in, float y_in, float z_in, float w_in) {x=x_in; y=y_in;z=z_in;w=w_in;}
};




vsVector2D operator*(float b, const vsVector2D &vec);
vsVector3D operator*(float b, const vsVector3D &vec);
vsVector3D operator*(float b, const vsVector4D &vec);

vsVector2D operator-(const vsVector2D &vec);
vsVector3D operator-(const vsVector3D &vec);
vsVector3D operator-(const vsVector4D &vec);

float vsProgressFraction( float value, float a, float b );	// returned value is what you'd pass as 'alpha' to vsInterpolate, to get back the 'value' value.

float vsInterpolate( float alpha, float a, float b );

vsVector2D vsInterpolate( float alpha, const vsVector2D &a, const vsVector2D &b );
vsVector3D vsInterpolate( float alpha, const vsVector3D &a, const vsVector3D &b );
vsVector4D vsInterpolate( float alpha, const vsVector4D &a, const vsVector4D &b );

#endif // VS_VECTOR_H
