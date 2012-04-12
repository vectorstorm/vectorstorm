/*
 *  UT_Spring.cpp
 *  MMORPG
 *
 *  Created by Trevor Powell on 16/10/08.
 *  Copyright 2008 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Spring.h"

vsSpring::vsSpring(float stiffness, float damping):
	m_stiffness(stiffness),
	m_damping(damping)
{
}

float
vsSpring::Update(float timeStep)
{
	float delta = m_center - m_position;

	m_velocity *= m_damping * timeStep;
	m_velocity += delta * m_stiffness * timeStep;

	m_position += m_velocity * timeStep;

	return m_position;
}



vsSpring2D::vsSpring2D(const vsVector2D &stiffness, float dampingFactor):
	m_stiffness(stiffness)
{
	m_damping.Set( dampingFactor, dampingFactor );
}

const vsVector2D &
vsSpring2D::Update(float timeStep)
{
	vsVector2D delta = m_center - m_position;

	m_velocity.x *= m_damping.x * timeStep;
	m_velocity.y *= m_damping.y * timeStep;

	m_velocity.x += delta.x * m_stiffness.x * timeStep;
	m_velocity.y += delta.y * m_stiffness.y * timeStep;

	m_position += m_velocity * timeStep;

	return m_position;
}



vsSpring3D::vsSpring3D(const vsVector3D &stiffness, float dampingFactor):
m_stiffness(stiffness)
{
	m_damping.Set( dampingFactor, dampingFactor, dampingFactor );
}

const vsVector3D &
vsSpring3D::Update(float timeStep)
{
	vsVector3D delta = m_center - m_position;

	m_velocity.x *= m_damping.x * timeStep;
	m_velocity.y *= m_damping.y * timeStep;
	m_velocity.z *= m_damping.z * timeStep;

	m_velocity.x += delta.x * m_stiffness.x * timeStep;
	m_velocity.y += delta.y * m_stiffness.y * timeStep;
	m_velocity.z += delta.z * m_stiffness.z * timeStep;

	m_position += m_velocity * timeStep;

	return m_position;
}


