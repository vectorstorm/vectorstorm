/*
 *  VS_Angle.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_ANGLE_H
#define VS_ANGLE_H

#define PI				(3.141593f)
#define TWOPI			(PI * 2.0f)
#define HALFPI			(PI * 0.5f)
#define DEGREES(x)		((x) * (PI/180.0f))
#define TO_DEGREES(x)	((x) * (180.0f/PI))

#include "VS_Vector.h"

class vsAngle
{
	float m_angle;
	
	mutable float m_cosValue;
	mutable float m_sinValue;
	mutable bool  m_trigCalculated;
	
	void	CalculateTrig() const;
	
public:
	static vsAngle Zero;
	static vsAngle FromForwardVector( const vsVector2D &forward );
		
			vsAngle(float angle_in=0.f);
	
	void	Set(float angle_in);
	void	Rotate(float rotation);
	
	bool	IsIdentity() { return (Get() == 0.0f); }
	
	float	Get() const { return m_angle; }
	float	GetDegrees() const { return TO_DEGREES(m_angle); } 
	float	Cos() const;
	float	Sin() const;
	
	vsVector2D	ApplyRotationTo( const vsVector2D &in ) const;
	vsVector2D	GetForwardVector() const { return ApplyRotationTo( vsVector2D(0.0f,-1.0f) ); }
	
	vsAngle		operator*(float s) const { return vsAngle(Get()*s); }

	vsAngle		operator+(const vsAngle &b) const { return vsAngle(Get()+b.Get()); }
	vsAngle		operator-(const vsAngle &b) const { return vsAngle(Get()-b.Get()); }

	vsAngle		operator+=(const vsAngle &b) { Set( Get() + b.Get() );  return *this; }
	vsAngle		operator-=(const vsAngle &b) { Set( Get() - b.Get() );  return *this; }
	vsAngle		operator*=(float s) { Set( Get() * s );  return *this; }
	bool		operator==(const vsAngle &b) const { return m_angle == b.m_angle; }
	bool		operator!=(const vsAngle &b) const { return m_angle != b.m_angle; }
};

vsAngle operator-(const vsAngle &ang);
vsAngle vsInterpolate( float alpha, const vsAngle &a, const vsAngle &b );

#endif // VS_ANGLE_H
