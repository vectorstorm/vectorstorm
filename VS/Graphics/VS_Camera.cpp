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
	if ( vsScreen::Instance() )
		m_aspectRatio = vsScreen::Instance()->GetAspectRatio();
	else
		m_aspectRatio = 1.66f;
}

vsCamera2D::~vsCamera2D()
{
}

vsMatrix4x4
vsCamera2D::GetProjectionMatrix() const
{
	float hh = m_transform.GetScale().x * -.5f;
	float hw = -hh * m_aspectRatio;
	float n, f;
	n = -1000.f;
	f = 1000.f;

	vsMatrix4x4 m(
			vsVector4D( 1.f / hw, 0.f, 0.f, 0.f ),
			vsVector4D( 0.f, 1.f / hh, 0.f, 0.f ),
			vsVector4D( 0.f, 0.f, -2.f / (f-n), -(f+n) / (f-n) ),
			vsVector4D( 0.f, 0.f, 0.f, 1.f )
			);

	return m;
}

vsMatrix4x4
vsCamera2D::GetWorldToViewMatrix() const
{
	vsTransform3D startingTransform = vsSystem::Instance()->GetOrientationTransform_2D();
	vsMatrix4x4 startingMatrix = startingTransform.GetMatrix();
	vsTransform2D cameraTransform = GetCameraTransform();

	// cameraTransform will have a scale on its members from the camera. (Since
	// that's where the 2D camera stores the FOV).
	//
	// We remove that here, since that eventually becomes part of the
	// PROJECTION transform, not the MODELVIEW transform, which is all we care
	// about here.

	cameraTransform.SetScale(vsVector2D(1.f,1.f));
	vsMatrix4x4 cameraMatrix = cameraTransform.GetMatrix();
	cameraMatrix.Invert();

	return cameraMatrix * startingMatrix;
}

bool
vsCamera2D::IsPositionVisible( const vsVector2D &pos, float r )
{
	vsVector2D finalPos = pos;

	if ( GetAngle().Get() != 0.f )
	{
		vsAngle reverseAngle;
		reverseAngle.Set( -GetAngle().Get() );
		finalPos = reverseAngle.ApplyRotationTo(pos);
	}
	finalPos -= GetPosition();

	float halfFov = GetFOV() * 0.5f;
	float halfHoriFov = m_aspectRatio * halfFov;
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
	vsVector2D finalPos = pos_in;

	finalPos -= GetPosition();
	if ( GetAngle().Get() != 0.f )
	{
		// convert from world to camera-local orientation
		vsAngle reverseAngle( -GetAngle().Get() );
		finalPos = reverseAngle.ApplyRotationTo(finalPos);
	}

	float horiFov = m_aspectRatio * GetFOV();
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
	m_aspectRatio = vsScreen::Instance()->GetAspectRatio();
}

vsCamera3D::~vsCamera3D()
{
}

vsMatrix4x4
vsCamera3D::GetProjectionMatrix() const
{
	if ( m_type == PT_Perspective )
	{
		float hh = vsTan(m_fov * .5f) * m_nearPlane;
		float hw = hh * m_aspectRatio;

		vsMatrix4x4 m(
				vsVector4D( (2.f*m_nearPlane) / (hw * 2.0f), 0.f, 0.f, 0.f ),
				vsVector4D( 0.f, (2.f*m_nearPlane) / (hh * 2.0f), 0.f, 0.f ),
				vsVector4D( 0.f, 0.f, (m_nearPlane + m_farPlane) / (m_farPlane - m_nearPlane), 1.f ),
				vsVector4D( 0.f, 0.f, -2.f * (m_farPlane * m_nearPlane) / (m_farPlane - m_nearPlane), 0.f )
				);

		return m;
	}
	else if ( m_type == PT_Orthographic )
	{
		float height = m_fov;
		float width = height * m_aspectRatio;
		float hh = 0.5f * height;
		float hw = 0.5f * width;
		float f = m_nearPlane;
		float n = m_farPlane;

		vsMatrix4x4 m(
				vsVector4D( 1.f / hw, 0.f, 0.f, 0.f ),
				vsVector4D( 0.f, 1.f / hh, 0.f, 0.f ),
				vsVector4D( 0.f, 0.f, -2.f / (f-n), 0.f ),
				vsVector4D( 0.f, 0.f, 0.f, 1.f )
				);

		return m;
	}
    vsAssert(0, "Unknown projection type??");
    return vsMatrix4x4::Identity;
}

void
vsCamera3D::LookAt( const vsVector3D &lookat, const vsVector3D &upDirection )
{
	vsQuaternion quat( lookat - GetPosition(), upDirection );
	SetOrientation(quat);
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

vsCamera3D::VisibilityType
vsCamera3D::ClassifyBox3D( const vsBox3D &box ) const
{
	VisibilityType result = VisibilityType_NotVisible;
	switch( m_frustum.ClassifyBox3D( box ) )
	{
		case vsFrustum::Inside:
			result = VisibilityType_AllVisible;
			break;
		case vsFrustum::Intersect:
			result = VisibilityType_PartiallyVisible;
			break;
		case vsFrustum::Outside:
			result = VisibilityType_NotVisible;
			break;
	}
	return result;
}

bool
vsCamera3D::IsBox3DVisible( const vsBox3D &box ) const
{
	return m_frustum.IsBox3DInside( box );
}

vsMatrix4x4
vsCamera3D::GetWorldToViewMatrix() const
{
	vsTransform3D startingTransform = vsSystem::Instance()->GetOrientationTransform_3D();
	vsMatrix4x4 startingMatrix = startingTransform.GetMatrix();

	vsMatrix4x4 requestedMatrix = vsMatrix4x4::Identity;

	requestedMatrix.w -= GetPosition();
	//
	vsMatrix4x4 myIdentity;
	myIdentity.x *= -1.f;
	//
	vsMatrix4x4 cameraMatrix = GetTransform().GetMatrix();

	vsVector3D forward = cameraMatrix.z;
	vsVector3D up = cameraMatrix.y;
	vsVector3D side = forward.Cross(up);

	cameraMatrix.x = side;
	cameraMatrix.y = up;
	cameraMatrix.z = forward;
	cameraMatrix.w.Set(0.f,0.f,0.f,1.f);
	cameraMatrix.Invert();

	cameraMatrix = startingMatrix * myIdentity * cameraMatrix;

	return cameraMatrix * requestedMatrix;
}

