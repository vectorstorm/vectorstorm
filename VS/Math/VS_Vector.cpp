/*
 *  VS_Vector.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Vector.h"
//#include <math.h>

vsVector2D vsVector2D::Zero(0.0f,0.0f);
vsVector2D vsVector2D::One(1.0f,1.0f);
vsVector2D vsVector2D::XAxis(1.0f,0.0f);
vsVector2D vsVector2D::YAxis(0.0f,1.0f);

vsVector3D vsVector3D::Zero(0.f,0.f,0.f);
vsVector3D vsVector3D::One(1.f,1.f,1.f);

vsVector3D vsVector3D::XAxis(1.f,0.f,0.f);
vsVector3D vsVector3D::YAxis(0.f,1.f,0.f);
vsVector3D vsVector3D::ZAxis(0.f,0.f,1.f);

#define SQRT_TWO (1.41421356237f)

vsVector2D::vsVector2D( const vsVector3D &b )
{
	x = b.x;
	y = b.y;
}

float
vsVector2D::ApproximateLength() const
{
	const float factor = (1.0f + 1.0f/(4.0f-2.0f*SQRT_TWO))/2.0f;

	float ax = vsFabs(x);
	float ay = vsFabs(y);

	return factor * vsMin((1.0f / SQRT_TWO)*(ax+ay), vsMax(ax, ay));
}

void
vsVector2D::Normalise()
{
	float length = Length();

	if ( length == 0.f )
	{
		vsCheck(length != 0.f, "Tried to normalise zero-length vector!");
		length = 1.f; // avoid creating NaNs.
	}
	*this *= (1.0f/length);

	if ( vsIsNaN(x) || vsIsNaN(y) )
	{
		vsCheck(!vsIsNaN(x) && !vsIsNaN(y), "Error, NaN in normalised vector!");
		length = 1.f; // avoid creating NaNs.
		x = y = 0.f;
	}
}

void
vsVector2D::NormaliseSafe()
{
	if ( SqLength() > 0.f )
		Normalise();
}

vsVector2D
vsVector2D::Normalised()
{
	float length = Length();

	if ( length == 0.f )
	{
		vsAssert(length != 0.f, "Tried to normalise zero-length vector!");
		length = 1.f; // avoid creating NaNs.
	}
	return vsVector2D(x/length, y/length);
}

vsVector2D
vsVector2D::NormalisedSafe()
{
	if ( SqLength() > 0.f )
		return Normalised();
	return *this;
}

vsVector3D::vsVector3D( const vsVector4D &b ):
	x(b.x),
	y(b.y),
	z(b.z)
{
}

void
vsVector3D::Normalise()
{
	float length = Length();

	if ( length == 0.f )
	{
		vsCheck(length != 0.f, "Tried to normalise zero-length vector!");
		length = 1.f; // avoid creating NaNs.
	}
	*this *= (1.0f/length);
	if ( vsIsNaN(x) || vsIsNaN(y) || vsIsNaN(z) )
	{
		vsCheck( !vsIsNaN(x) && !vsIsNaN(y) && !vsIsNaN(z), "Error, NaN in normalised vector!");
		x = y = z = 0.f;
	}
}

void
vsVector3D::NormaliseSafe()
{
	if ( SqLength() > 0.f )
	{
		Normalise();
	}
}

vsVector3D
vsVector3D::Normalised()
{
	float length = Length();

	if ( length == 0.f )
	{
		vsCheck(length != 0.f, "Tried to normalise zero-length vector!");
		length = 1.f; // avoid creating NaNs.
	}
	return vsVector3D(x/length, y/length, z/length);
}

vsVector3D
vsVector3D::NormalisedSafe()
{
	if ( SqLength() > 0.f )
		return Normalised();
	return *this;
}

void
vsVector3D::Floor()
{
    x = (float)vsFloor(x);
    y = (float)vsFloor(y);
    z = (float)vsFloor(z);
}

float
vsVector3D::operator[](int n) const
{
	if ( n == 0 )
		return x;
	else if ( n == 1 )
		return y;
	else if ( n == 2 )
		return z;

	vsAssert(0,"Illegal index!");
	return 0.f;
}

void vsVector4D::Normalise()
{
	float length = Length();
	if ( length == 0.f )
	{
		vsCheck(length != 0.f, "Tried to normalise zero-length vector!");
		length = 1.f; // avoid creating NaNs.
	}
	*this *= (1.0f/Length());
}

vsVector4D
vsVector4D::Normalised()
{
	float length = Length();
	if ( length == 0.f )
	{
		vsCheck(length != 0.f, "Tried to normalise zero-length vector!");
		length = 1.f; // avoid creating NaNs.
	}

	return (*this * (1.0f/Length()));
}

float & vsVector4D::operator[]( int n )
{
	if ( n == 0 )
		return x;
	else if ( n == 1 )
		return y;
	else if ( n == 2 )
		return z;
	else if ( n == 3 )
		return w;

	vsAssert(0,"Illegal index!");
	return w;
}


vsVector2D operator*(float b, const vsVector2D &vec) { return vec * b; }
vsVector3D operator*(float b, const vsVector3D &vec) { return vec * b; }
vsVector3D operator*(float b, const vsVector4D &vec) { return vec * b; }

vsVector2D operator-(const vsVector2D &vec) { return vsVector2D(-vec.x,-vec.y); }
vsVector3D operator-(const vsVector3D &vec) { return vsVector3D(-vec.x,-vec.y,-vec.z); }
vsVector3D operator-(const vsVector4D &vec) { return vsVector4D(-vec.x,-vec.y,-vec.z,-vec.w); }


#define POSITION_BITS (16)
#define POSITION_FACTOR ((float)(1<<POSITION_BITS))
#define POSITION_INV_FACTOR (1.f / POSITION_FACTOR)

vsPosition3D::vsPosition3D():
	m_x(0),
	m_y(0),
	m_z(0)
{
}

vsPosition3D::vsPosition3D( float x, float y, float z )
{
	m_x = (int32_t)(x * POSITION_FACTOR);
	m_y = (int32_t)(y * POSITION_FACTOR);
	m_z = (int32_t)(z * POSITION_FACTOR);
}

vsPosition3D::vsPosition3D( const vsVector3D &pos )
{
	m_x = (int32_t)(pos.x * POSITION_FACTOR);
	m_y = (int32_t)(pos.y * POSITION_FACTOR);
	m_z = (int32_t)(pos.z * POSITION_FACTOR);
}

vsPosition3D
vsPosition3D::operator+( const vsVector3D &b ) const
{
	vsPosition3D result = *this;
	result += b;
	return result;
}

vsPosition3D
vsPosition3D::operator+=( const vsVector3D &b )
{
	m_x += (int32_t)(b.x * POSITION_FACTOR);
	m_y += (int32_t)(b.y * POSITION_FACTOR);
	m_z += (int32_t)(b.z * POSITION_FACTOR);

	return *this;
}

vsVector3D
vsPosition3D::operator-( const vsPosition3D &b ) const
{
	vsVector3D result( (float)m_x - b.m_x,
					  (float)m_y - b.m_y,
					  (float)m_z - b.m_z );
	result *= POSITION_INV_FACTOR;

	return result;
}


vsVector2D vsInterpolate( float alpha, const vsVector2D &a, const vsVector2D &b )
{
	return ((1.0f-alpha)*a) + (alpha*b);
}

vsVector3D vsInterpolate( float alpha, const vsVector3D &a, const vsVector3D &b )
{
	return ((1.0f-alpha)*a) + (alpha*b);
}

vsVector4D vsInterpolate( float alpha, const vsVector4D &a, const vsVector4D &b )
{
	return ((1.0f-alpha)*a) + (alpha*b);
}

