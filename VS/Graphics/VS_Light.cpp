/*
 *  VS_Light.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Light.h"

vsLight::vsLight()
{
	m_type = Type_Ambient;
	m_position = vsVector3D::Zero;
	m_direction = vsVector3D::ZAxis;
	m_color = c_white;
	m_ambient = vsColor(0.f,0.f,0.f,0.f);
	m_specular = c_white;
}

vsLight::~vsLight()
{
}

