/*
 *  VS_Quaternion.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_QUATERNION_H
#define VS_QUATERNION_H

#include "VS_Vector.h"

class vsMatrix3x3;
class vsEulerAngles;

class vsQuaternion
{
public:

	static vsQuaternion Identity;

	float x;
	float y;
	float z;
	float w;

	vsQuaternion();
	vsQuaternion(float x, float y, float z, float w);
	vsQuaternion(const vsVector3D &axis, float angle );	// intentionally not using a vsAngle, because we need to support spins of more than 180 degrees around an axis!
	vsQuaternion(const vsVector3D &forward, const vsVector3D &up);	// these do NOT have to be normalised vectors;  we normalise them inside the engine.
	vsQuaternion(const vsEulerAngles &ang);

	vsQuaternion operator*(const vsQuaternion &b) const;
	const vsQuaternion &	operator*=(const vsQuaternion &b) {*this = (*this)*b; return *this; };

	bool	operator==(const vsQuaternion &b) const { return (x==b.x && y==b.y && z==b.z && w==b.w); }
	bool	operator!=(const vsQuaternion &b) const { return !(*this==b); }

	void	Set( const vsMatrix3x3 &mat );
	void	Set( const vsEulerAngles &ang );

	void	Normalise();
	void	NormaliseIfNeeded();

	vsVector3D	ApplyTo( const vsVector3D &in ) const;

	void			Invert();
	vsQuaternion	Inverse() const;
};

// Spherical linear linearpolation from one quaternion to another.
vsQuaternion vsQuaternionSlerp( float alpha, const vsQuaternion &a, const vsQuaternion &b );
vsEulerAngles vsEulerAnglesFromQuaternion( const vsQuaternion &q );

#endif // VS_QUATERNION_H
