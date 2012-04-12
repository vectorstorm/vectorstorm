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


vsCamera3D::vsCamera3D()
{
	m_fov = DEGREES(60.f);
	m_nearPlane = 1.f;
	m_farPlane = 2000.f;
}

vsCamera3D::~vsCamera3D()
{
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

