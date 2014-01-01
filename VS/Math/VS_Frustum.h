/*
 *  VS_Frustum.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 10/06/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_FRUSTUM_H
#define VS_FRUSTUM_H

#include "VS_Box.h"
#include "VS_Vector.h"

class vsCamera3D;

class vsFrustum
{
	vsVector3D		m_planePoint[6];
	vsVector3D		m_planeNormal[6];

public:

	vsFrustum();

	void	Set( vsCamera3D *camera );

	bool	IsPointInside( const vsVector3D &position, float radius=0.f ) const;
	bool	IsBox3DInside( const vsBox3D &box ) const;

	enum Classification
	{
		Outside,
		Intersect,
		Inside
	};
	Classification ClassifyBox3D( const vsBox3D &box ) const;
	Classification ClassifySphere( const vsVector3D &position, float radius ) const;
};

#endif // VS_FRUSTUM_H

