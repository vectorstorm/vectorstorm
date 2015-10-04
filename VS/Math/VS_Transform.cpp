/*
 *  VS_Transform.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Transform.h"

#include "VS_Quaternion.h"

vsTransform2D vsTransform2D::Zero;
vsTransform3D vsTransform3D::Identity( vsQuaternion(0.f,0.f,0.f,1.f), vsVector3D::Zero, vsVector3D(1.f,1.f,1.f) );

vsTransform2D::vsTransform2D():
	m_position( vsVector2D::Zero ),
	m_angle( vsAngle::Zero ),
	m_scale(1.0f, 1.0f)
{
}

vsTransform2D::vsTransform2D( const vsVector2D &pos, const vsAngle &angle ):
	m_position(pos),
	m_angle(angle),
	m_scale(1.f, 1.f)
{
}

vsTransform2D::vsTransform2D( const vsVector2D &pos, const vsAngle &angle, const vsVector2D &scale ):
	m_position(pos),
	m_angle(angle),
	m_scale(scale)
{
}

void
vsTransform2D::SetTranslation( const vsVector2D &translation )
{
	m_position = translation;
	m_dirty = true;
}

void
vsTransform2D::SetAngle( const vsAngle &angle )
{
	m_angle = angle;
	m_dirty = true;
}

void
vsTransform2D::SetScale( const vsVector2D &scale )
{
	m_scale = scale;
	m_dirty = true;
}


vsTransform2D vsInterpolate( float alpha, const vsTransform2D &a, const vsTransform2D &b )
{
	vsTransform2D result;

	result.SetTranslation( vsInterpolate( alpha, a.GetTranslation(), b.GetTranslation() ) );
	result.SetAngle( vsInterpolate( alpha, a.GetAngle(), b.GetAngle() ) );
	result.SetScale( vsInterpolate( alpha, a.GetScale(), b.GetScale() ) );

	return result;
}

vsVector2D
vsTransform2D::ApplyTo( const vsVector2D &v )
{
	vsVector2D result = m_angle.ApplyRotationTo(v);
	result.x *= m_scale.x;
	result.y *= m_scale.y;
	result += m_position;

	return result;
}

vsVector2D
vsTransform2D::ApplyInverseTo( const vsVector2D &v )
{
	vsVector2D result = v;
	vsAngle inverseAngle = -m_angle;

	result -= m_position;
	result.x /= m_scale.x;
	result.y /= m_scale.y;
	result = inverseAngle.ApplyRotationTo(result);

	return result;
}

vsTransform2D
vsTransform2D::operator*( const vsTransform2D &o )
{
	vsTransform2D result;

//	result = o;

	result.m_position = ApplyTo(o.m_position);
	result.m_angle = m_angle + o.m_angle;
	result.m_scale.Set( m_scale.x * o.m_scale.x, m_scale.y * o.m_scale.y );

	return result;
}

vsTransform2D
vsTransform2D::ApplyInverseTo( const vsTransform2D &o )
{
	vsTransform2D result;

	//	result = o;

	result.m_position = ApplyInverseTo(o.m_position);
	result.m_angle = o.m_angle - m_angle;
	result.m_scale.Set( o.m_scale.x / m_scale.x, o.m_scale.y / m_scale.y );

	return result;
}


const vsMatrix4x4 &
vsTransform2D::GetMatrix() const
{
	if ( m_dirty )
	{
		m_dirty = false;

		vsMatrix3x3 m;
		m.Set( vsQuaternion( vsVector3D::ZAxis, m_angle.Get() ) );

		m.x *= m_scale.x;
		m.y *= m_scale.y;
		m_matrix.SetRotationMatrix( m );
		m_matrix.SetTranslation( m_position );
	}

	return m_matrix;
}

vsTransform3D::vsTransform3D( const vsTransform3D& other ):
	m_quaternion(other.m_quaternion),
	m_translation(other.m_translation),
	m_scale(other.m_scale),
	m_dirty(true)
{
}

vsTransform3D::vsTransform3D( const vsQuaternion &quat, const vsVector3D &translation, const vsVector3D &scale):
	m_quaternion(quat),
	m_translation(translation),
	m_scale(scale),
	m_dirty(true)
{
}

vsVector3D
vsTransform3D::ApplyTo( const vsVector3D &v ) const
{
	return GetMatrix().ApplyTo(v);
}

const vsMatrix4x4 &
vsTransform3D::GetMatrix() const
{
	if ( m_dirty )
	{
		m_dirty = false;

		vsMatrix3x3 m;
		m.Set(m_quaternion);

		m_matrix.SetRotationMatrix( m );
		m_matrix.SetTranslation( m_translation );
		m_matrix.Scale( m_scale );
	}

	return m_matrix;
}


vsTransform3D vsInterpolate( float alpha, const vsTransform3D &a, const vsTransform3D &b )
{
	vsTransform3D result;

	result.SetTranslation( vsInterpolate( alpha, a.GetTranslation(), b.GetTranslation() ) );
	result.SetRotation( vsQuaternionSlerp( alpha, a.GetRotation(), b.GetRotation() ) );
	result.SetScale( vsInterpolate( alpha, a.GetScale(), b.GetScale() ) );

	return result;
}

/*
vsVector3D
vsTransform3D::ApplyTo( const vsVector3D &other ) const
{
	vsVector3D result = m_matrix.ApplyTo(other);
	result += m_translation;

	return result;
}*/

/*
vsTransform3D::vsTransform3D( const vsVector3D &forward, const vsVector3D &up )
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

vsTransform3D::vsTransform3D( const vsVector4D &x_in, const vsVector4D &y_in, const vsVector4D &z_in, const vsVector4D &t_in ):
	x(x_in),
	y(y_in),
	z(z_in),
	t(t_in)
{
}
*/
/*
void
vsTransform3D::Set(const vsVector4D &x_in, const vsVector4D &y_in, const vsVector4D &z_in, const vsVector4D &t_in)
{
	x = x_in;
	y = y_in;
	z = z_in;
	t = t_in;
}

void
vsTransform3D::SetAsRotationAroundX( const vsAngle &a )
{
	x.Set(1.f,0.f,0.f,0.f);
	y.Set(0.f,a.Cos(),-a.Sin(),0.f);
	z.Set(0.f,a.Sin(),a.Cos(),0.f);
}

void
vsTransform3D::SetAsRotationAroundY( const vsAngle &a )
{
	x.Set(a.Cos(),0.f,a.Sin(),0.f);
	y.Set(0.f,1.f,0.f,0.f);
	z.Set(-a.Sin(),0.f,a.Cos(),0.f);
}

void
vsTransform3D::SetAsRotationAroundZ( const vsAngle &a )
{
	x.Set(a.Cos(),-a.Sin(),0.f,0.f);
	y.Set(a.Sin(),a.Cos(),0.f,0.f);
	z.Set(0.f,0.f,1.f,0.f);
}

vsTransform3D
vsTransform3D::Inverse() const
{
	vsTransform3D result;

	result.x.Set(x.x, y.x, z.x, 0.f);
	result.y.Set(x.y, y.y, z.y, 0.f);
	result.z.Set(x.z, y.z, z.z, 0.f);
	result.t.Set(x.Dot(t), y.Dot(t), z.Dot(t), 1.f);

	return result;
}*/

