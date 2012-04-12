/*
 *  UT_Spring.h
 *  MMORPG
 *
 *  Created by Trevor Powell on 16/10/08.
 *  Copyright 2008 Trevor Powell. All rights reserved.
 *
 */

#ifndef UT_SPRING_H
#define UT_SPRING_H

#include <VS/Math/VS_Vector.h>

class vsSpring
{
	float	m_stiffness;
	float	m_damping;

	float	m_center;
	float	m_position;
	float	m_velocity;

public:

	vsSpring( float stiffness, float dampingFactor );

	void	SetCenter(float c) { m_center = c; }
	void	SetPosition(float p) { m_position = p; }
	void	SetVelocity(float v) { m_velocity = v; }

	float	Update( float timeStep );
};

class vsSpring2D
{
	vsVector2D	m_stiffness;
	vsVector2D	m_damping;

	vsVector2D	m_center;
	vsVector2D	m_position;
	vsVector2D	m_velocity;

public:

	vsSpring2D( const vsVector2D &stiffness, float dampingFactor );
//	vsSpring2D( const vsVector2D &stiffness, const vsVector2D & dampingFactor );

	void	SetCenter(const vsVector2D & c) { m_center = c; }
	void	SetPosition(const vsVector2D & p) { m_position = p; }
	void	SetVelocity(const vsVector2D & v) { m_velocity = v; }

	const vsVector2D &	Update( float timeStep );
};

class vsSpring3D
{
	vsVector3D	m_stiffness;
	vsVector3D	m_damping;

	vsVector3D	m_center;
	vsVector3D	m_position;
	vsVector3D	m_velocity;

public:

	vsSpring3D( const vsVector3D &stiffness, float dampingFactor );
	//	vsSpring2D( const vsVector2D &stiffness, const vsVector2D & dampingFactor );

	void	SetCenter(const vsVector3D & c) { m_center = c; }
	void	SetPosition(const vsVector3D & p) { m_position = p; }
	void	SetVelocity(const vsVector3D & v) { m_velocity = v; }

	const vsVector3D &	Update( float timeStep );
};

#endif // UT_SPRING_H
