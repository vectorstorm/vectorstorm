/*
 *  PHYS_Emitter.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef PHYS_EMITTER_H
#define PHYS_EMITTER_H

#include "VS/Graphics/VS_Color.h"
#include "VS/Math/VS_Vector.h"
#include "VS_CollisionObject.h"

class vsPhysicsSprite;
class vsDisplayList;

class vsPhysicsEmitter
{
protected:
	int m_scene;

	vsDisplayList *	m_baseList;

	vsPhysicsSprite **	m_particle;
	vsColor *		m_particleColor;
	bool *			m_particleInUse;
	float *			m_particleLife;

	int				m_particleCount;

	vsVector2D		m_position;
	float			m_radius;

	vsColor			m_color;
	vsVector2D		m_velocity;
	float			m_velRadius;

	float			m_spawnRate;
	float			m_hose;

	float			m_particleLifetime;

	void			Spawn(const vsVector2D &pos, const vsVector2D &vel, const vsColor &color = vsColor::White);

public:

	vsPhysicsEmitter(const vsString &filename, float mass, int maxParticleCount, int scene, int testAgainst = ColFlag_Enemy|ColFlag_Player|ColFlag_World);
	virtual ~vsPhysicsEmitter();

	void	SetSpawnPosition( const vsVector2D &pos, float radius = 0.f ) { m_position = pos; m_radius = radius; }
	void	SetSpawnVelocity( const vsVector2D &vel, float radius = 0.f ) { m_velocity = vel; m_velRadius = radius; }
	void	SetSpawnColor( const vsColor &c ) { m_color = c; }

	void	SetSpawnRate( float particlesPerSecond ) { m_spawnRate = particlesPerSecond; }
	void	SpawnImmediateBurst( int spawnCount, const vsVector2D &pos, const vsVector2D &vel, float speed, float radius = 0.f, const vsColor &color = vsColor::White );
	void	SpawnImmediateBurst( int spawnCount, const vsVector2D &pos, float speed, float radius = 0.f, const vsColor &color = vsColor::White );
	virtual void	Update( float timeStep );
};

#endif // PHYS_EMITTER_H
