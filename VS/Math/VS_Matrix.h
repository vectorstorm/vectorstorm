/*
 *  VS_Matrix.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MATRIX_H
#define VS_MATRIX_H

#include "VS_Vector.h"
#include "VS_Angle.h"

class vsQuaternion;

class vsMatrix3x3
{
public:
	static vsMatrix3x3	Identity;

	vsVector3D		x;
	vsVector3D		y;
	vsVector3D		z;

	vsMatrix3x3();
	vsMatrix3x3(const vsVector3D &x_in, const vsVector3D &y_in, const vsVector3D &z_in);
	vsMatrix3x3(const vsVector3D &forward, const vsVector3D &up);
	vsMatrix3x3( const vsQuaternion &quat );

	void Set( const vsQuaternion &quat );
	void Set(const vsVector3D &x_in, const vsVector3D &y_in, const vsVector3D &z_in);
	void Set(const vsVector3D &forward, const vsVector3D &up);

	const vsVector3D &	operator[](int n) const;

	vsMatrix3x3 Transpose() const;
	float Determinant() const;
	vsMatrix3x3 Inverse() const;
	void Invert();

	vsVector3D		ApplyTo( const vsVector3D &v ) const;

	vsMatrix3x3 operator*(float b) const { return vsMatrix3x3(x*b, y*b, z*b); }
	vsMatrix3x3 operator*(vsMatrix3x3& b) const;
};

class vsMatrix4x4
{
	vsMatrix4x4 InverseGeneral() const;
public:
	static vsMatrix4x4	Identity;

	vsVector4D		x;
	vsVector4D		y;
	vsVector4D		z;
	vsVector4D		w;

	vsMatrix4x4();

	vsMatrix4x4( const vsVector3D &forward, const vsVector3D &up );	// from a forward and an up vector, we can create a rotation matrix!
	vsMatrix4x4( const vsVector4D &x, const vsVector4D &y, const vsVector4D &z, const vsVector4D &t );

	void			SetRotationMatrix( const vsMatrix3x3 &m );
	void			SetTranslation( const vsVector3D &t );
	void			Scale( const vsVector3D &s );

	void			SetAsRotationAroundX( const vsAngle &a );
	void			SetAsRotationAroundY( const vsAngle &a );
	void			SetAsRotationAroundZ( const vsAngle &a );

	//	bool			IsIdentity() { return (this == vsTransform3D::Identity); }

	vsMatrix4x4	Inverse() const;
	void		Invert();
	float		Determinant() const;
	vsMatrix4x4	Transpose() const;

	void			Set( const vsVector4D &x, const vsVector4D &y, const vsVector4D &z, const vsVector4D &t );

	vsVector3D		ApplyTo( const vsVector3D &v ) const;
	vsVector4D		ApplyTo( const vsVector4D &v ) const;
	vsVector3D		ApplyRotationTo( const vsVector3D &v ) const;
	vsVector3D		ApplyInverseTo( const vsVector3D &v ) const;

	vsVector4D &	operator[](int n);

	vsMatrix4x4	operator*( const vsMatrix4x4 &o ) const { return ApplyTo(o); }
	vsMatrix4x4	ApplyTo( const vsMatrix4x4 &o ) const;
	vsMatrix4x4	ApplyInverseTo( const vsMatrix4x4 &o ) const;

	bool operator==( const vsMatrix4x4 &o ) const { return ( x==o.x && y==o.y && z==o.z && w==o.w ); }
	bool operator!=( const vsMatrix4x4 &o ) const { return !((*this)==o); }
};

#endif // VS_MATRIX_H

