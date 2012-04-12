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
vsFrustum::Set( vsCamera3D *camera )
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
	
	vsVector3D nearCenter = camera->GetPosition() + m_planeNormal[0] * camera->GetNearPlane();
	vsVector3D farCenter = camera->GetPosition() + m_planeNormal[0] * camera->GetFarPlane();
	
	float hh = vsTan(camera->GetFOV() * .5f) * camera->GetFarPlane();
	float hw = hh * vsSystem::GetScreen()->GetAspectRatio();
	
	vsVector3D farTopLeft = farCenter - (mat.x * hw) + (mat.y * hh);
	vsVector3D farTopRight = farCenter + (mat.x * hw) + (mat.y * hh);
	vsVector3D farBottomLeft = farCenter - (mat.x * hw) - (mat.y * hh);
	vsVector3D farBottomRight = farCenter + (mat.x * hw) - (mat.y * hh);
	
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

// Code structure for IsBox3DInside taken from the Lighthouse 3D OpenGL 
// "View Frustum Culling Tutorial"
// at http://www.lighthouse3d.com/opengl/viewfrustum/index.php

bool
vsFrustum::IsBox3DInside( const vsBox3D &box ) const
{
	int in, out;
	
	vsVector3D boxVert[8];
	boxVert[0] = box.GetMin();
	boxVert[1].Set( box.GetMin().x, box.GetMin().y, box.GetMax().z );
	boxVert[2].Set( box.GetMin().x, box.GetMax().y, box.GetMin().z );
	boxVert[3].Set( box.GetMin().x, box.GetMax().y, box.GetMax().z );
	boxVert[4].Set( box.GetMax().x, box.GetMin().y, box.GetMin().z );
	boxVert[5].Set( box.GetMax().x, box.GetMin().y, box.GetMax().z );
	boxVert[6].Set( box.GetMax().x, box.GetMax().y, box.GetMin().z );
	boxVert[7].Set( box.GetMax().x, box.GetMax().y, box.GetMax().z );
	
	// for each plane do ...
	for(int i=0; i < 6; i++) {
		
		// reset counters for corners in and out
		out=0;in=0;
		// for each corner of the box do ...
		// get out of the cycle as soon as a box has corners
		// both inside and out of the frustum
		for (int k = 0; k < 8 && (in==0 || out==0); k++) {
			
			// is the corner outside or inside
			float distance = (boxVert[k]-m_planePoint[i]).Dot(m_planeNormal[i]);
			
			if (distance < 0)
				out++;
			else
			{
				in++;
				break;
			}
		}
		//if all corners are out
		if (!in)
			return false;
	}
	return true;
}

