/*
 *  VS_Transform.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_TRANSFORM_H
#define VS_TRANSFORM_H

#include "VS_Angle.h"
#include "VS_Matrix.h"
#include "VS_Vector.h"
#include "VS_Quaternion.h"

class vsTransform2D
{
	mutable bool			m_dirty;
	mutable vsMatrix4x4		m_matrix;

	vsVector2D		m_position;
	vsAngle			m_angle;
	vsVector2D		m_scale;

public:

	static vsTransform2D	Zero;

	vsTransform2D();
	vsTransform2D(const vsVector2D &pos, const vsAngle &angle);
	vsTransform2D(const vsVector2D &pos, const vsAngle &angle, const vsVector2D &scale);

    void            SetTranslation( const vsVector2D &translation );
    void            SetTranslation( float x, float y ) { SetTranslation( vsVector2D(x,y) ); }
    void            SetAngle( const vsAngle &angle );
    void            SetScale( const vsVector2D &scale );
    void            SetScale( float x, float y ) { SetScale( vsVector2D(x,y) ); }

    const vsVector2D& GetTranslation() const    { return m_position; }
    const vsAngle&  GetAngle() const            { return m_angle; }
    const vsVector2D& GetScale() const          { return m_scale; }

	vsVector2D		ApplyTo( const vsVector2D &v ) const;
	vsVector2D		ApplyInverseTo( const vsVector2D &v ) const;

	vsTransform2D operator*( const vsTransform2D &o ) const;
	vsTransform2D	ApplyInverseTo( const vsTransform2D &o ) const;

	const vsMatrix4x4 & GetMatrix() const;
};

class vsTransform3D
{
	vsQuaternion	m_quaternion;
	vsVector3D		m_translation;
	vsVector3D		m_scale;

	mutable bool			m_dirty;
	mutable vsMatrix4x4		m_matrix;

public:
	static vsTransform3D	Identity;

	vsTransform3D( const vsTransform3D& other );
	vsTransform3D( const vsQuaternion &quat = vsQuaternion::Identity, const vsVector3D &translation = vsVector3D::Zero, const vsVector3D &scale = vsVector3D::One );

	const vsVector3D &		GetTranslation() const		{ return m_translation; }
	const vsQuaternion &	GetRotation() const			{ return m_quaternion; }
	const vsVector3D &		GetScale() const			{ return m_scale; }

	void					SetTranslation( const vsVector3D &t ) { m_translation = t; m_dirty = true; }
	void					SetRotation( const vsQuaternion &q )	{ m_quaternion = q; m_dirty = true; }
	void					SetScale( const vsVector3D &s )	{ m_scale = s; m_dirty = true; }
	void					SetScale( float s ) { m_scale.Set(s,s,s); m_dirty = true; }

	const vsMatrix4x4 &		GetMatrix() const;

//	bool			IsIdentity() { return (this == vsTransform3D::Identity); }

	vsTransform3D	Inverse() const;
	vsTransform3D	Transpose() const;

	void			Set( const vsVector4D &x, const vsVector4D &y, const vsVector4D &z, const vsVector4D &t );

	vsVector3D		ApplyTo( const vsVector3D &v ) const;
	vsVector3D		ApplyInverseTo( const vsVector3D &v ) const;

	vsTransform3D	operator*( const vsTransform3D &o ) const { return ApplyTo(o); }
	vsTransform3D	ApplyTo( const vsTransform3D &o ) const;
	vsTransform3D	ApplyInverseTo( const vsTransform3D &o ) const;

	bool			operator==( const vsTransform3D &b ) const { return (m_quaternion==b.m_quaternion && m_translation==b.m_translation && m_scale==b.m_scale); }
	bool			operator!=( const vsTransform3D &b ) const { return !(*this==b); }
};

vsTransform2D vsInterpolate( float alpha, const vsTransform2D &a, const vsTransform2D &b );
vsTransform3D vsInterpolate( float alpha, const vsTransform3D &a, const vsTransform3D &b );

#endif // VS_TRANSFORM_H
