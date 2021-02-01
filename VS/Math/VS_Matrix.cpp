/*
 *  VS_Matrix.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Matrix.h"

#include "VS_Quaternion.h"

vsMatrix4x4 vsMatrix4x4::Identity;
vsMatrix3x3 vsMatrix3x3::Identity;

vsMatrix3x3::vsMatrix3x3():
	x(1.f,0.f,0.f),
	y(0.f,1.f,0.f),
	z(0.f,0.f,1.f)
{
}

vsMatrix3x3::vsMatrix3x3( const vsVector3D &x_in, const vsVector3D &y_in, const vsVector3D &z_in ):
	x(x_in),
	y(y_in),
	z(z_in)
{
}

vsMatrix3x3::vsMatrix3x3( const vsVector3D &forward, const vsVector3D &up )
{
	Set(forward,up);
}

vsMatrix3x3::vsMatrix3x3( const vsQuaternion &q )
{
	Set(q);
}

void
vsMatrix3x3::Set( const vsVector3D &x_in, const vsVector3D &y_in, const vsVector3D &z_in )
{
	x = x_in;
	y = y_in;
	z = z_in;
}

void
vsMatrix3x3::Set( const vsVector3D &forward, const vsVector3D &up )
{
	vsVector3D f,u,r;

	f = forward;
	u = up;

	f.Normalise();
	u.Normalise();

	r = u.Cross(f);
	u = f.Cross(r);

	x = r;
	y = u;
	z = f;

	// done!
}

void
vsMatrix3x3::Set( const vsQuaternion &q )
{
    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	const vsQuaternion *quat = &q;

    // calculate coefficients
    x2 = quat->x + quat->x; y2 = quat->y + quat->y;
    z2 = quat->z + quat->z;
    xx = quat->x * x2; xy = quat->x * y2; xz = quat->x * z2;
    yy = quat->y * y2; yz = quat->y * z2; zz = quat->z * z2;
    wx = quat->w * x2; wy = quat->w * y2; wz = quat->w * z2;

    x.x = 1.0f - (yy + zz);
	y.x = xy - wz;
    z.x = xz + wy;

    x.y = xy + wz;
	y.y = 1.0f - (xx + zz);
    z.y = yz - wx;

    x.z = xz - wy;
	y.z = yz + wx;
    z.z = 1.0f - (xx + yy);


}

vsVector3D
vsMatrix3x3::ApplyTo( const vsVector3D &v ) const
{
	vsVector3D result;

	result.x = v.x * x.x + v.y * y.x + v.z * z.x;
	result.y = v.x * x.y + v.y * y.y + v.z * z.y;
	result.z = v.x * x.z + v.y * y.z + v.z * z.z;

	return result;
}

const vsVector3D &
vsMatrix3x3::operator[](int n) const
{
	if ( n == 0 )
	{
		return x;
	}
	else if ( n == 1 )
	{
		return y;
	}
	else if ( n == 2 )
	{
		return z;
	}

	vsAssert(0,"ILLEGAL INDEX");
	return vsVector3D::Zero;
}

float
vsMatrix3x3::Determinant() const
{
	float value;
	value =
		x.x * (y.y * z.z - y.z * z.y) -
		x.y * (y.x * z.z - y.z * z.x) +
		x.z * (y.x * z.y - y.y * z.x);
	return value;
}

vsMatrix3x3
vsMatrix3x3::Transpose() const
{
    // just a transpose
    vsMatrix3x3 result;

    result.x.x = x.x;
    result.x.y = y.x;
    result.x.z = z.x;

    result.y.x = x.y;
    result.y.y = y.y;
    result.y.z = z.y;

    result.z.x = x.z;
    result.z.y = y.z;
    result.z.z = z.z;

    return result;
}

vsMatrix3x3
vsMatrix3x3::Inverse() const
{
	float determinant = Determinant();

	if ( determinant == 0.f )
	{
		// no inverse!
		return *this;
	}

	float invdet = 1.0f/determinant;
	vsMatrix3x3 result;

	result.x.x = (y.y * z.z - z.y * y.z) * invdet;
	result.x.y = (x.z * z.y - x.y * z.z) * invdet;
	result.x.z = (x.y * y.z - x.z * y.y) * invdet;
	result.y.x = (y.z * z.x - y.x * z.z) * invdet;
	result.y.y = (x.x * z.z - x.z * z.x) * invdet;
	result.y.z = (y.x * x.z - x.x * y.z) * invdet;
	result.z.x = (y.x * z.y - z.x * y.y) * invdet;
	result.z.y = (z.x * x.y - x.x * z.y) * invdet;
	result.z.z = (x.x * y.y - y.x * x.y) * invdet;

    return result;
}

void vsMatrix3x3::Invert()
{
    *this = Inverse();
}

vsMatrix3x3
vsMatrix3x3::operator*(vsMatrix3x3& b) const
{
	return vsMatrix3x3(
			vsVector3D(
				x.x * b.x.x + x.y * b.y.x + x.z * b.z.x,
				x.x * b.x.y + x.y * b.y.y + x.z * b.z.y,
				x.x * b.x.z + x.y * b.y.z + x.z * b.z.z
				),
			vsVector3D(
				y.x * b.x.x + y.y * b.y.x + y.z * b.z.x,
				y.x * b.x.y + y.y * b.y.y + y.z * b.z.y,
				y.x * b.x.z + y.y * b.y.z + y.z * b.z.z
				),
			vsVector3D(
				z.x * b.x.x + z.y * b.y.x + z.z * b.z.x,
				z.x * b.x.y + z.y * b.y.y + z.z * b.z.y,
				z.x * b.x.z + z.y * b.y.z + z.z * b.z.z
				)
			);
}


vsMatrix4x4::vsMatrix4x4():
x(1.f,0.f,0.f,0.f),
y(0.f,1.f,0.f,0.f),
z(0.f,0.f,1.f,0.f),
w(0.f,0.f,0.f,1.f)
{
}

vsMatrix4x4::vsMatrix4x4( const vsVector3D &forward, const vsVector3D &up )
{
	vsVector3D f,u,r;

	f = forward;
	u = up;

	f.Normalise();
	u.Normalise();

	r = f.Cross(u);
	u = r.Cross(f);

	x = r;
	y = u;
	z = f;

	// done!
}

vsMatrix4x4::vsMatrix4x4( const vsVector4D &x_in, const vsVector4D &y_in, const vsVector4D &z_in, const vsVector4D &t_in ):
x(x_in),
y(y_in),
z(z_in),
w(t_in)
{
}

void
vsMatrix4x4::Set(const vsVector4D &x_in, const vsVector4D &y_in, const vsVector4D &z_in, const vsVector4D &t_in)
{
	x = x_in;
	y = y_in;
	z = z_in;
	w = t_in;
}

void
vsMatrix4x4::SetRotationMatrix( const vsMatrix3x3 &m )
{
	x = m.x;
	y = m.y;
	z = m.z;
}

void
vsMatrix4x4::SetTranslation( const vsVector3D &t_in )
{
	w = t_in;
	w.w = 1.f;
}

void
vsMatrix4x4::Scale( const vsVector3D &s )
{
	x.x *= s.x;
	x.y *= s.y;
	x.z *= s.z;

	y.x *= s.x;
	y.y *= s.y;
	y.z *= s.z;

	z.x *= s.x;
	z.y *= s.y;
	z.z *= s.z;
}

void
vsMatrix4x4::SetAsRotationAroundX( const vsAngle &a )
{
	x.Set(1.f,0.f,0.f,0.f);
	y.Set(0.f,a.Cos(),-a.Sin(),0.f);
	z.Set(0.f,a.Sin(),a.Cos(),0.f);
}

void
vsMatrix4x4::SetAsRotationAroundY( const vsAngle &a )
{
	x.Set(a.Cos(),0.f,a.Sin(),0.f);
	y.Set(0.f,1.f,0.f,0.f);
	z.Set(-a.Sin(),0.f,a.Cos(),0.f);
}

void
vsMatrix4x4::SetAsRotationAroundZ( const vsAngle &a )
{
	x.Set(a.Cos(),-a.Sin(),0.f,0.f);
	y.Set(a.Sin(),a.Cos(),0.f,0.f);
	z.Set(0.f,0.f,1.f,0.f);
}

void
vsMatrix4x4::Invert()
{
	*this = Inverse();
}

#define m11 (x.x)
#define m12 (y.x)
#define m13 (z.x)
#define m14 (w.x)
#define m21 (x.y)
#define m22 (y.y)
#define m23 (z.y)
#define m24 (w.y)
#define m31 (x.z)
#define m32 (y.z)
#define m33 (z.z)
#define m34 (w.z)
#define m41 (x.w)
#define m42 (y.w)
#define m43 (z.w)
#define m44 (w.w)

vsMatrix4x4
vsMatrix4x4::InverseGeneral() const
{
	float det;
	float d12, d13, d23, d24, d34, d41;

	/* Allow out == in, but don't do an extra copy */
	vsMatrix4x4 temp;

	/* Inverse = adjoint / det. (See linear algebra texts.)*/

	/* pre-compute 2x2 dets for last two rows when computing */
	/* cofactors of first two rows. */
	d12 = (m31*m42-m41*m32);
	d13 = (m31*m43-m41*m33);
	d23 = (m32*m43-m42*m33);
	d24 = (m32*m44-m42*m34);
	d34 = (m33*m44-m43*m34);
	d41 = (m34*m41-m44*m31);

	temp.x.x =  (m22 * d34 - m23 * d24 + m24 * d23);
	temp.x.y = -(m21 * d34 + m23 * d41 + m24 * d13);
	temp.x.z =  (m21 * d24 + m22 * d41 + m24 * d12);
	temp.x.w = -(m21 * d23 - m22 * d13 + m23 * d12);

	/* Compute determinant as early as possible using these cofactors. */
	det = m11 * temp.x.x + m12 * temp.x.y + m13 * temp.x.z + m14 * temp.x.w;

	/* Run singularity test. */
	if (det == 0.0f)
	{
		return vsMatrix4x4::Identity;
	}

	float invDet = 1.0f / det;
	/* Compute rest of inverse. */
	temp.x.x *= invDet;
	temp.x.y *= invDet;
	temp.x.z *= invDet;
	temp.x.w *= invDet;

	temp.y.x = -(m12 * d34 - m13 * d24 + m14 * d23) * invDet;
	temp.y.y =  (m11 * d34 + m13 * d41 + m14 * d13) * invDet;
	temp.y.z = -(m11 * d24 + m12 * d41 + m14 * d12) * invDet;
	temp.y.w =  (m11 * d23 - m12 * d13 + m13 * d12) * invDet;

	/* Pre-compute 2x2 dets for first two rows when computing */
	/* cofactors of last two rows. */
	d12 = m11*m22-m21*m12;
	d13 = m11*m23-m21*m13;
	d23 = m12*m23-m22*m13;
	d24 = m12*m24-m22*m14;
	d34 = m13*m24-m23*m14;
	d41 = m14*m21-m24*m11;

	temp.z.x =  (m42 * d34 - m43 * d24 + m44 * d23) * invDet;
	temp.z.y = -(m41 * d34 + m43 * d41 + m44 * d13) * invDet;
	temp.z.z =  (m41 * d24 + m42 * d41 + m44 * d12) * invDet;
	temp.z.w = -(m41 * d23 - m42 * d13 + m43 * d12) * invDet;
	temp.w.x = -(m32 * d34 - m33 * d24 + m34 * d23) * invDet;
	temp.w.y =  (m31 * d34 + m33 * d41 + m34 * d13) * invDet;
	temp.w.z = -(m31 * d24 + m32 * d41 + m34 * d12) * invDet;
	temp.w.w =  (m31 * d23 - m32 * d13 + m33 * d12) * invDet;

	return temp;
}

vsMatrix4x4
vsMatrix4x4::Inverse() const
{
	/* NB. OpenGL Matrices are COLUMN major. */

	/* Here's some shorthand converting standard (row,column) to index. */

	if( m41 != 0.0f || m42 != 0.0f || m43 != 0.0f || m44 != 1.0f )
	{
		return InverseGeneral();
	}

	/* Inverse = adjoint / det. (See linear algebra texts.)*/

	/* Allow out == in, but don't do an extra copy */
	vsMatrix4x4 temp;

	temp.x.x= m22 * m33 - m23 * m32;
	temp.x.y= m23 * m31 - m21 * m33;
	temp.x.z= m21 * m32 - m22 * m31;

	/* Compute determinant as early as possible using these cofactors. */
	float det;
	det= m11 * temp.x.x + m12 * temp.x.y + m13 * temp.x.z;

	/* Run singularity test. */
	if (det == 0.0)
	{
		vsLog("invert_matrix: Warning: Singular matrix.");
		return vsMatrix4x4::Identity;
	}

	float d12, d13, d23, d24, d34, d41;
	float im11, im12, im13, im14;

	det= 1.0f / det;

	/* Compute rest of inverse. */
	temp.x.x *= det;
	temp.x.y *= det;
	temp.x.z *= det;
	temp.x.w  = 0.0f;

	im11= m11 * det;
	im12= m12 * det;
	im13= m13 * det;
	im14= m14 * det;
	temp.y.x = im13 * m32 - im12 * m33;
	temp.y.y = im11 * m33 - im13 * m31;
	temp.y.z = im12 * m31 - im11 * m32;
	temp.y.w = 0.0f;

	/* Pre-compute 2x2 dets for first two rows when computing */
	/* cofactors of last two rows. */
	d12 = im11*m22 - m21*im12;
	d13 = im11*m23 - m21*im13;
	d23 = im12*m23 - m22*im13;
	d24 = im12*m24 - m22*im14;
	d34 = im13*m24 - m23*im14;
	d41 = im14*m21 - m24*im11;

	temp.z.x =  d23;
	temp.z.y = -d13;
	temp.z.z = d12;
	temp.z.w = 0.0f;

	temp.w.x = -(m32 * d34 - m33 * d24 + m34 * d23);
	temp.w.y =  (m31 * d34 + m33 * d41 + m34 * d13);
	temp.w.z = -(m31 * d24 + m32 * d41 + m34 * d12);
	temp.w.w =  1.0f;


#undef m11
#undef m12
#undef m13
#undef m14
#undef m21
#undef m22
#undef m23
#undef m24
#undef m31
#undef m32
#undef m33
#undef m34
#undef m41
#undef m42
#undef m43
#undef m44
	return temp;
}



float
vsMatrix4x4::Determinant() const
{
	float value;
	value =
	x.w * y.z * z.y * w.x - x.z * y.w * z.y * w.x - x.w * y.y * z.z * w.x+x.y * y.w * z.z * w.x+
	x.z * y.y * z.w * w.x - x.y * y.z * z.w * w.x - x.w * y.z * z.x * w.y+x.z * y.w * z.x * w.y+
	x.w * y.x * z.z * w.y - x.x * y.w * z.z * w.y - x.z * y.x * z.w * w.y+x.x * y.z * z.w * w.y+
	x.w * y.y * z.x * w.z - x.y * y.w * z.x * w.z - x.w * y.x * z.y * w.z+x.x * y.w * z.y * w.z+
	x.y * y.x * z.w * w.z - x.x * y.y * z.w * w.z - x.z * y.y * z.x * w.w+x.y * y.z * z.x * w.w+
	x.z * y.x * z.y * w.w - x.x * y.z * z.y * w.w - x.y * y.x * z.z * w.w+x.x * y.y * z.z * w.w;
	return value;
}

vsVector3D
vsMatrix4x4::ApplyTo( const vsVector3D &v ) const
{
	vsVector3D result;

	result.x = v.x * x.x + v.y * y.x + v.z * z.x;
	result.y = v.x * x.y + v.y * y.y + v.z * z.y;
	result.z = v.x * x.z + v.y * y.z + v.z * z.z;

	result += w;

	return result;
}

vsVector4D
vsMatrix4x4::ApplyTo( const vsVector4D &v ) const
{
	vsVector4D result;

	result.x = v.x * x.x + v.y * y.x + v.z * z.x + v.w * w.x;
	result.y = v.x * x.y + v.y * y.y + v.z * z.y + v.w * w.y;
	result.z = v.x * x.z + v.y * y.z + v.z * z.z + v.w * w.z;
	result.w = v.x * x.w + v.y * y.w + v.z * z.w + v.w * w.w;

	return result;
}

vsVector3D
vsMatrix4x4::ApplyRotationTo( const vsVector3D &v ) const
{
	vsVector3D result;

	result.x = v.x * x.x + v.y * y.x + v.z * z.x;
	result.y = v.x * x.y + v.y * y.y + v.z * z.y;
	result.z = v.x * x.z + v.y * y.z + v.z * z.z;

	return result;
}

vsMatrix4x4
vsMatrix4x4::ApplyInverseTo( const vsMatrix4x4 &o ) const
{
	// we could be heaps faster than this, but this fills in the functionality for now.
	return Inverse().ApplyTo( o );
}

vsVector4D &
vsMatrix4x4::operator[](int n)
{
	vsAssert(n >= 0 && n < 4, "Out of bounds!");

	if ( n == 0 )
		return x;
	if ( n == 1 )
		return y;
	if ( n == 2 )
		return z;

	return w;
}

vsMatrix4x4
vsMatrix4x4::ApplyTo( const vsMatrix4x4 &o ) const
{
	vsMatrix4x4 result;

	result.x.x = x.x * o.x.x + y.x * o.x.y + z.x * o.x.z + w.x * o.x.w;
	result.x.y = x.y * o.x.x + y.y * o.x.y + z.y * o.x.z + w.y * o.x.w;
	result.x.z = x.z * o.x.x + y.z * o.x.y + z.z * o.x.z + w.z * o.x.w;
	result.x.w = x.w * o.x.x + y.w * o.x.y + z.w * o.x.z + w.w * o.x.w;

	result.y.x = x.x * o.y.x + y.x * o.y.y + z.x * o.y.z + w.x * o.y.w;
	result.y.y = x.y * o.y.x + y.y * o.y.y + z.y * o.y.z + w.y * o.y.w;
	result.y.z = x.z * o.y.x + y.z * o.y.y + z.z * o.y.z + w.z * o.y.w;
	result.y.w = x.w * o.y.x + y.w * o.y.y + z.w * o.y.z + w.w * o.y.w;

	result.z.x = x.x * o.z.x + y.x * o.z.y + z.x * o.z.z + w.x * o.z.w;
	result.z.y = x.y * o.z.x + y.y * o.z.y + z.y * o.z.z + w.y * o.z.w;
	result.z.z = x.z * o.z.x + y.z * o.z.y + z.z * o.z.z + w.z * o.z.w;
	result.z.w = x.w * o.z.x + y.w * o.z.y + z.w * o.z.z + w.w * o.z.w;

	result.w.x = x.x * o.w.x + y.x * o.w.y + z.x * o.w.z + w.x * o.w.w;
	result.w.y = x.y * o.w.x + y.y * o.w.y + z.y * o.w.z + w.y * o.w.w;
	result.w.z = x.z * o.w.x + y.z * o.w.y + z.z * o.w.z + w.z * o.w.w;
	result.w.w = x.w * o.w.x + y.w * o.w.y + z.w * o.w.z + w.w * o.w.w;

	return result;
}

vsMatrix4x4
vsMatrix4x4::Transpose() const
{
    // just a transpose
    vsMatrix4x4 result;

    result.x.x = x.x;
    result.x.y = y.x;
    result.x.z = z.x;
    result.x.w = w.x;

    result.y.x = x.y;
    result.y.y = y.y;
    result.y.z = z.y;
    result.y.w = w.y;

    result.z.x = x.z;
    result.z.y = y.z;
    result.z.z = z.z;
    result.z.w = w.z;

    result.w.x = x.w;
    result.w.y = y.w;
    result.w.z = z.w;
    result.w.w = w.w;

    return result;
}

