/*
 *  VS_Camera.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 20/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Camera.h"
#include "VS_Screen.h"
#include "VS_System.h"
#include "VS_Matrix.h"

vsCamera2D::vsCamera2D()
{
	SetPosition( vsVector2D::Zero );
	SetAngle( vsAngle::Zero );
	SetFOV( 1000.f );
}

vsCamera2D::~vsCamera2D()
{
}
/*
vsMatrix4x4
vsCamera2D::GetProjectionMatrix( float aspectRatio )
{
	float hh = vsTan(m_fov * .5f) * m_nearPlane;
	float hw = hh * aspectRatio;

	vsMatrix4x4 m(
			vsVector4D( 2.f*m_nearPlane / (hw * 2.0f), 0.f, 0.f, 0.f ),
			vsVector4D( 0.f, 2.f*m_nearPlane / (hh * 2.0f), 0.f, 0.f ),
			vsVector4D( 0.f, 0.f, (m_nearPlane + m_farPlane) / (m_nearPlane - m_farPlane), (2.f * m_farPlane * m_nearPlane) / (m_nearPlane - m_farPlane) ),
			vsVector4D( 0.f, 0.f, -1.f, 0.f )
			);
}*/

bool
vsCamera2D::IsPositionVisible( const vsVector2D &pos, float r )
{
	float aspectRatio = vsSystem::GetScreen()->GetAspectRatio();
	vsVector2D finalPos = pos;

	if ( GetAngle().Get() != 0.f )
	{
		vsAngle reverseAngle;
		reverseAngle.Set( -GetAngle().Get() );
		finalPos = reverseAngle.ApplyRotationTo(pos);
	}
	finalPos -= GetPosition();

	float halfFov = GetFOV() * 0.5f;
	float halfHoriFov = aspectRatio * halfFov;
	bool visible = true;

	if ( finalPos.y - r > halfFov || finalPos.y + r < -halfFov )
		visible = false;
	if ( finalPos.x - r > halfHoriFov || finalPos.x + r < -halfHoriFov )
		visible = false;

	return visible;
}

bool
vsCamera2D::WrapAround( vsVector2D &pos_in, float r )
{
	float aspectRatio = vsSystem::GetScreen()->GetAspectRatio();
	vsVector2D finalPos = pos_in;

	finalPos -= GetPosition();
	if ( GetAngle().Get() != 0.f )
	{
		// convert from world to camera-local orientation
		vsAngle reverseAngle( -GetAngle().Get() );
		finalPos = reverseAngle.ApplyRotationTo(finalPos);
	}

	float horiFov = aspectRatio * GetFOV();
	float halfFov = GetFOV() * 0.5f;
	float halfHoriFov = horiFov * 0.5f;

	float wrapDist = halfFov + r;
	float horizWrapDist = halfHoriFov + r;
	bool wrapped = false;

	if ( finalPos.y - r > halfFov )
	{
		finalPos.y = -wrapDist;
		wrapped = true;
	}
	else if ( finalPos.y + r < -halfFov )
	{
		finalPos.y = wrapDist;
		wrapped = true;
	}

	if ( finalPos.x - r > halfHoriFov )
	{
		finalPos.x = -horizWrapDist;
		wrapped = true;
	}
	else if ( finalPos.x + r < -halfHoriFov )
	{
		finalPos.x = horizWrapDist;
		wrapped = true;
	}

	if ( wrapped )
	{
		if ( GetAngle().Get() != 0.f )
		{
			// convert back from local to world orientation
			finalPos = GetAngle().ApplyRotationTo(finalPos);
		}
		finalPos += GetPosition();
		pos_in = finalPos;
	}
	return wrapped;
}


vsCamera3D::vsCamera3D(ProjectionType type)
{
	m_fov = DEGREES(60.f);
	m_nearPlane = 1.f;
	m_farPlane = 2000.f;
	m_type = type;
	m_aspectRatio = vsSystem::GetScreen()->GetAspectRatio();
}

vsCamera3D::~vsCamera3D()
{
}

vsMatrix4x4
vsCamera3D::GetProjectionMatrix(float aspectRatio)
{
	if ( m_type == PT_Perspective )
	{
		float hh = vsTan(m_fov * .5f) * m_nearPlane;
		float hw = hh * aspectRatio;

		vsMatrix4x4 m(
				vsVector4D( 2.f*m_nearPlane / (hw * 2.0f), 0.f, 0.f, 0.f ),
				vsVector4D( 0.f, 2.f*m_nearPlane / (hh * 2.0f), 0.f, 0.f ),
				vsVector4D( 0.f, 0.f, (m_nearPlane + m_farPlane) / (m_nearPlane - m_farPlane), (2.f * m_farPlane * m_nearPlane) / (m_nearPlane - m_farPlane) ),
				vsVector4D( 0.f, 0.f, -1.f, 0.f )
				);

		return m;
	}
	else if ( m_type == PT_Orthographic )
	{
		float height = m_fov;
		float width = height * aspectRatio;
		vsMatrix4x4 m(
				vsVector4D( 2.f / width, 0.f, 0.f, 0.f ),
				vsVector4D( 0.f, 2.f / height, 0.f, 0.f ),
				vsVector4D( 0.f, 0.f, -2.f / (m_farPlane - m_nearPlane), 0.f ),
				vsVector4D( 0.f, 0.f,  -1.f * ((m_farPlane + m_nearPlane) / (m_farPlane - m_nearPlane)), 1.f )
				);

		return m;
	}
    vsAssert(0, "Unknown projection type??");
    return vsMatrix4x4::Identity;
}

void
vsCamera3D::UpdateFrustum()
{
	m_frustum.Set(this);
}

bool
vsCamera3D::IsPositionVisible( const vsVector3D &pos, float radius ) const
{
	// first, we need to extract the six planes of our view frustum

	return m_frustum.IsPointInside( pos, radius );
}

bool
vsCamera3D::IsBox3DVisible( const vsBox3D &box ) const
{
	return m_frustum.IsBox3DInside( box );
}

