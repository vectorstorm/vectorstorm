/*
 *  VS_Quaternion.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Quaternion.h"

#include "VS_Angle.h"
#include "VS_EulerAngles.h"
#include "VS_Matrix.h"

vsQuaternion vsQuaternion::Identity(0.f,0.f,0.f,1.f);

vsQuaternion::vsQuaternion()
{
	x = y = z = 0.f;
	w = 1.f;
}

vsQuaternion::vsQuaternion(float x_in, float y_in, float z_in, float w_in):
	x(x_in),
	y(y_in),
	z(z_in),
	w(w_in)
{
}

vsQuaternion::vsQuaternion( const vsVector3D &axis, float angle_in )
{
	vsAngle angle( angle_in );
	vsAngle halfAngle( angle * 0.5f );

	w = halfAngle.Cos();
	x = axis.x * halfAngle.Sin();
	y = axis.y * halfAngle.Sin();
	z = axis.z * halfAngle.Sin();
}

vsQuaternion::vsQuaternion(const vsVector3D &forward, const vsVector3D &up)
{
	vsMatrix3x3 mat(forward,up);

	Set(mat);
}

vsQuaternion::vsQuaternion( const vsEulerAngles &ang )
{
	Set(ang);
}

void
vsQuaternion::Set( const vsMatrix3x3 &mat )
{
	float trace = mat.x.x + mat.y.y + mat.z.z;
	if( trace > 0.f )
	{
		float s = vsSqrt(trace+1.f);
		w = 0.5f * s;
		s = 0.5f / s;
		x = (mat[1][2] - mat[2][1]) * s;
		y = (mat[2][0] - mat[0][2]) * s;
		z = (mat[0][1] - mat[1][0]) * s;
	}
	else // diagonal is negative
	{

		int nxt[3] = {1,2,0};
		int i = 0;
		int j,k;
		float q[4];

		if ( mat[1][1] > mat[0][0] ) i = 1;
		if ( mat[2][2] > mat[i][i] ) i = 2;

		j = nxt[i];
		k = nxt[j];

		float s = vsSqrt( (mat[i][i] - (mat[j][j] + mat[k][k])) + 1.0f );
		q[i] = s * 0.5f;

		if ( s != 0.f )
			s = 0.5f / s;

		q[3] = (mat[j][k] - mat[k][j]) * s;
		q[j] = (mat[i][j] + mat[j][i]) * s;
		q[k] = (mat[i][k] + mat[k][i]) * s;

		x = q[0];
		y = q[1];
		z = q[2];
		w = q[3];
	}

	NormaliseIfNeeded();
}

void
vsQuaternion::Set( const vsEulerAngles &ang )
{
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;

	vsAngle halfPitch = ang.pitch * .5f;
	vsAngle halfYaw = ang.yaw * .5f;
	vsAngle halfBank = ang.bank * .5f;

	// pitch becomes bank
	// yaw becomes pitch
	// bank becomes yaw??

	// calculate trig identities
	cr = halfPitch.Cos();
	cp = halfYaw.Cos();
	cy = halfBank.Cos();

	sr = halfPitch.Sin();
	sp = halfYaw.Sin();
	sy = halfBank.Sin();


//  cr = halfBank.Cos();
//  cp = halfPitch.Cos();
//  cy = halfYaw.Cos();
//
//  sr = halfBank.Sin();
//  sp = halfPitch.Sin();
//  sy = halfYaw.Sin();

	cpcy = cp * cy;
	spsy = sp * sy;

	w = cr * cpcy + sr * spsy;
	x = sr * cpcy - cr * spsy;
	y = cr * sp * cy + sr * cp * sy;
	z = cr * cp * sy - sr * sp * cy;
}

vsEulerAngles vsEulerAnglesFromQuaternion( const vsQuaternion &q )
{
	vsEulerAngles result;

	double sqw = q.w*q.w;
	double sqx = q.x*q.x;
	double sqy = q.y*q.y;
	double sqz = q.z*q.z;

	result.bank = vsATan2((float)(2.0 * (q.x*q.y + q.z*q.w)),(float)((sqx - sqy - sqz + sqw)));
	result.pitch = vsATan2((float)(2.0 * (q.y*q.z + q.x*q.w)),(float)((-sqx - sqy + sqz + sqw)));
	result.yaw = vsASin((float)(-2.0 * (q.x*q.z - q.y*q.w)/(sqx + sqy + sqz + sqw)));

	return result;
}

void
vsQuaternion::Normalise()
{
	float sqLength = ( w*w + x*x + y*y + z*z );

	if ( sqLength != 1.f )
	{
		float length = vsSqrt(sqLength);
		float invLength = 1.f / length;

		w *= invLength;
		x *= invLength;
		y *= invLength;
		z *= invLength;
	}
}

void
vsQuaternion::NormaliseIfNeeded()
{
	float sqLength = ( w*w + x*x + y*y + z*z );

	if ( sqLength > 1.01f || sqLength < 0.99f )
	{
		float length = vsSqrt(sqLength);
		float invLength = 1.f / length;

		w *= invLength;
		x *= invLength;
		y *= invLength;
		z *= invLength;
	}
}

vsQuaternion
vsQuaternion::operator*(const vsQuaternion &b) const
{
	float A, B, C, D, E, F, G, H;


	A = (w + x)*(b.w + b.x);
	B = (z - y)*(b.y - b.z);
	C = (w - x)*(b.y + b.z);
	D = (y + z)*(b.w - b.x);
	E = (x + z)*(b.x + b.y);
	F = (x - z)*(b.x - b.y);
	G = (w + y)*(b.w - b.z);
	H = (w - y)*(b.w + b.z);

	vsQuaternion res;

	res.w = B + (-E - F + G + H) /2;
	res.x = A - (E + F + G + H)/2;
	res.y = C + (E - F + G - H)/2;
	res.z = D + (E - F - G + H)/2;

	res.NormaliseIfNeeded();

	return res;
}

vsQuaternion vsQuaternionSlerp( float alpha, const vsQuaternion &from, const vsQuaternion &to )
{
	float	to1[4];
	float	omega, cosom, sinom, scale0, scale1;


	// calc cosine
	cosom = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;


	// adjust signs (if necessary)
	if ( cosom <0.0 )
	{
		cosom = -cosom;
		to1[0] = - to.x;
		to1[1] = - to.y;
		to1[2] = - to.z;
		to1[3] = - to.w;
	} else  {
		to1[0] = to.x;
		to1[1] = to.y;
		to1[2] = to.z;
		to1[3] = to.w;
	}


	// calculate coefficients


	if ( (1.0f - cosom) > 0.05f ) {
		// standard case (slerp)
		omega = vsACos(cosom);
		sinom = vsSin(omega);
		scale0 = vsSin((1.0f - alpha) * omega) / sinom;
		scale1 = vsSin(alpha * omega) / sinom;


	} else {
		// "from" and "to" quaternions are very close
	    //  ... so we can do a linear interpolation
		scale0 = 1.0f - alpha;
		scale1 = alpha;
	}

	vsQuaternion result;
	// calculate final values
	result.x = scale0 * from.x + scale1 * to1[0];
	result.y = scale0 * from.y + scale1 * to1[1];
	result.z = scale0 * from.z + scale1 * to1[2];
	result.w = scale0 * from.w + scale1 * to1[3];

	result.NormaliseIfNeeded();

	return result;
}


vsVector3D
vsQuaternion::ApplyTo( const vsVector3D &in ) const
{
	// This math is based on Laurent Couvidou's answer here:
	//
	// https://gamedev.stackexchange.com/a/50545
	//
	// Previously we converted the quaternion into a vsMatrix3x3 and used that
	// to rotate the vector, but this approach is faster **for a single
	// vector multiplication**.
	//
	// If, on the other hand, we're going to apply a rotation to a **lot** of
	// vectors, then we're still better off converting the quaternion into a 3D
	// matrix and multiplying by that, instead!

	vsVector3D u(x,y,z);
	float s(w);

	return 2.f * u.Dot(in) * u +
		(s*s - u.Dot(u)) * in +
		2.f * s * u.Cross(in);
}


void
vsQuaternion::Invert()
{
	NormaliseIfNeeded();

	x *= -1.0f;
	y *= -1.0f;
	z *= -1.0f;
}

vsQuaternion
vsQuaternion::Inverse() const
{
	vsQuaternion b(*this);
	b.NormaliseIfNeeded();

	b.x *= -1.0f;
	b.y *= -1.0f;
	b.z *= -1.0f;
	return b;
}

