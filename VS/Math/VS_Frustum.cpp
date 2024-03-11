/*
 *  VS_Frustum.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 10/06/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Frustum.h"

#include "VS/Graphics/VS_Camera.h"
#include "VS/Graphics/VS_Screen.h"
#include "VS/Utils/VS_System.h"

vsFrustum::vsFrustum()
{
}

void
vsFrustum::Set( vsCamera3D *camera, const vsVector2D& subrectMin /* = vsVector2D::Zero */, const vsVector2D& subrectMax /* = vsVector2D::One */ )
{
	float aspectRatio = camera->GetAspectRatio();
	if ( camera->GetProjectionType() == vsCamera3D::PT_Perspective )
	{
		// TODO:  consider setting up these frustum planes using projection matrix extraction.
		// would that be faster/simpler than this system?  (Do I even have access to the
		// necessary projection matrix from here?  That's currently only created inside the
		// renderer.)

		vsVector3D cameraPos = camera->GetPosition();

		for ( int i = 0; i < 6; i++ )
		{
			m_planePoint[i] = cameraPos;
		}

		const vsMatrix4x4 &mat = camera->GetTransform().GetMatrix();

		m_planeNormal[0] = mat.z;	// near plane normal (pointing inward)

		//vsVector3D nearCenter = camera->GetPosition() + m_planeNormal[0] * camera->GetNearPlane();
		vsVector3D farCenter = camera->GetPosition() + m_planeNormal[0] * camera->GetFarPlane();

		float hh = vsTan(camera->GetFOV() * .5f) * camera->GetFarPlane();
		float hw = hh * aspectRatio;

		// Get the min/max in the range of -1 to 1
		vsVector2D adjustedMin = subrectMin * 2.0f - vsVector2D::One;
		vsVector2D adjustedMax = subrectMax * 2.0f - vsVector2D::One;
		vsVector3D farTopLeft = farCenter + (mat.x * hw * adjustedMin.x) - (mat.y * hh * adjustedMin.y);
		vsVector3D farBottomRight = farCenter + (mat.x * hw * adjustedMax.x) - (mat.y * hh * adjustedMax.y);

		m_planePoint[0] = camera->GetPosition() + m_planeNormal[0] * camera->GetNearPlane();
		m_planeNormal[1] = -m_planeNormal[0];				// far plane
		m_planePoint[1] = farCenter;

		m_planeNormal[2] = vsVector3D(mat.y).Cross(farTopLeft-cameraPos);
		m_planeNormal[3] = vsVector3D(mat.x).Cross(farTopLeft-cameraPos);
		m_planeNormal[4] = (farBottomRight-cameraPos).Cross( mat.y );
		m_planeNormal[5] = (farBottomRight-cameraPos).Cross( mat.x );

		m_planeNormal[2].Normalise();
		m_planeNormal[3].Normalise();
		m_planeNormal[4].Normalise();
		m_planeNormal[5].Normalise();
	}
	else	// ortho 3D camera
	{
		// okay.  Need to set up all six planes for our orthographic view
		vsVector3D cameraPos = camera->GetPosition();
		const vsMatrix4x4 &mat = camera->GetTransform().GetMatrix();
		vsVector3D forward = mat.z;
		vsVector3D right = mat.x;
		vsVector3D up = mat.y;

		// near
		m_planePoint[0] = cameraPos + forward * camera->GetNearPlane();
		m_planeNormal[0] = forward;
		// far
		m_planePoint[1] = cameraPos + forward * camera->GetFarPlane();
		m_planeNormal[1] = -forward;
		// top
		m_planePoint[2] = cameraPos + up * camera->GetFOV();
		m_planeNormal[2] = -up;
		// bottom
		m_planePoint[3] = cameraPos - up * camera->GetFOV();
		m_planeNormal[3] = up;
		// left
		m_planePoint[4] = cameraPos - right * camera->GetFOV() * aspectRatio;
		m_planeNormal[4] = right;
		// right
		m_planePoint[5] = cameraPos + right * camera->GetFOV() * aspectRatio;
		m_planeNormal[5] = -right;

		for ( int i = 0; i < 6; i++ )
		{
			m_planeNormal[i].Normalise();
		}
	}
}

bool
vsFrustum::IsPointInside( const vsVector3D &position, float radius ) const
{
	for(int i=0; i < 6; i++)
	{
		float distance = (position-m_planePoint[i]).Dot(m_planeNormal[i]);
		if (distance < -radius)
			return false;
	}
	return true;
}

// Code structure for the below visibility calculations taken from the
// Lighthouse 3D OpenGL "View Frustum Culling Tutorial" at
// http://www.lighthouse3d.com/tutorials/view-frustum-culling/

bool
vsFrustum::IsBox3DInside( const vsBox3D &box ) const
{
	return ClassifyBox3D(box) != Outside;
}

vsFrustum::Classification
vsFrustum::ClassifySphere( const vsVector3D &center, float radius ) const
{
	float distance;
	Classification result = Inside;

	for(int i=0; i < 6; i++)
	{
		distance = (center-m_planePoint[i]).Dot(m_planeNormal[i]);
		if (distance < -radius)
			return Outside;
		else if (distance < radius)
			result = Intersect;
	}
	return result;
}

vsFrustum::Classification
vsFrustum::ClassifyBox3D( const vsBox3D &box ) const
{
	// make a sphere that encompasses our box.  If it's entirely
	// inside or entirely outside our view, just use that result.
	//
	// (Technically, we should be using half the diagonal as the radius.  But
	// since calculating that involves a square root, let's just get the
	// maximum dimension, and then assume that the box is a cube of that size)
	//
	// It's less accurate (it produces a larger sphere than is strictly
	// necessary), but it'll never claim that something is 'Outside' if it's
	// actually visible, and it'll never claim that it's entirely 'Inside' if
	// it's not actually inside.  And it'll still reject things which are
	// well-and-truly outside our view.
	//
	const float sqrtThree = 1.732051f; // approximately
	float longestDimension = vsMax(box.Width(), vsMax(box.Height(), box.Depth()));
	float radius = longestDimension * 0.5f * sqrtThree;
	vsVector3D middle = box.Middle();
	Classification sphereClassification = ClassifySphere( middle, radius );
	if ( sphereClassification != Intersect )
		return sphereClassification;

	Classification result = Inside;

	// for each plane do ...
	for(int i=0; i < 6; i++) {

		vsVector3D pCorner = box.PCorner( m_planeNormal[i] );
		vsVector3D nCorner = box.NCorner( m_planeNormal[i] );

		float distance = (pCorner-m_planePoint[i]).Dot(m_planeNormal[i]);
		if ( distance < 0.f )
			return Outside;
		distance = (nCorner-m_planePoint[i]).Dot(m_planeNormal[i]);
		if ( distance < 0.f )
			result = Intersect;
	}
	return result;
}

