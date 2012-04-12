/*
 *  COL_System.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef COL_SYSTEM_H
#define COL_SYSTEM_H

#include "Core/CORE_GameSystem.h"
#include "VS/Utils/VS_Singleton.h"
#include "VS/Math/VS_Vector.h"
#include "VS/Math/VS_Transform.h"

class vsPhysicsSprite;
class b2World;
class vsCollisionObject;
class vsCollisionSystemDestructionListener;
class vsCollisionSystemBoundaryListener;
class vsCollisionSystemContactListener;

/*
struct colRecord
{
	vsCollisionObject *		a;
	vsCollisionObject *		b;

	vsTransform2D		collisionTransformA;
	vsTransform2D		collisionTransformB;

	vsVector2D		collisionPoint;

	vsVector2D		normalA;
	vsVector2D		normalB;

	float			impactTime;
};
*/

class vsCollisionSystem : public coreGameSystem
{
	static vsCollisionSystem *	s_instance;

	b2World *		m_world;
	vsCollisionSystemBoundaryListener *		m_boundaryListener;
	vsCollisionSystemDestructionListener *	m_destructionListener;
	vsCollisionSystemContactListener *		m_contactListener;

	static float	s_left;
	static float	s_right;
	static float	s_top;
	static float	s_bottom;

	static bool		s_border;

	float			m_timeBucket;

public:

					vsCollisionSystem();
	virtual			~vsCollisionSystem();

	static vsCollisionSystem *Instance() { return s_instance; }

	void			SetWorldBounds(float left, float right, float top, float bottom) { s_left = left; s_right = right; s_top = top; s_bottom = bottom; }	// must be called BEFORE Init()!
	void			SetBorder(bool border) { s_border = border; }	// must be called BEFORE Init()!
	void			SetNoBorder() { SetBorder(false); }				// must be called BEFORE Init()!

	virtual void	Init();
	virtual void	Deinit();

	int				FindColObjectsUnder( const vsVector2D &where, float radius, vsCollisionObject **resultArray, int maxResults );	// returns how many were found

	b2World *		GetWorld() { return m_world; }

	void			SetGravity( const vsVector2D &gravity );

	virtual void	Update( float timeStep );

//	void			RegisterObject( vsCollisionObject *sprite );
//	void			DeregisterObject( vsCollisionObject *sprite );

	//bool			CollisionTest( const vsVector2D &a, const vsVector2D &b, float r, int testFlags );
};

#endif // COL_SYSTEM_H
