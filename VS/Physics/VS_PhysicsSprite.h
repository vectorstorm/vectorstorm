/*
 *  PHYS_Sprite.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 4/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef PHYS_SPRITE_H
#define PHYS_SPRITE_H

#include "VS/Graphics/VS_Sprite.h"
#include "VS/Physics/VS_CollisionObject.h"

class vsPhysicsSprite : public vsSprite, public vsCollisionResponder
{
	typedef vsSprite Parent;

	int				m_colFlags;
	int				m_testFlags;

	colCircle		m_boundingCircle;

	bool			m_boxType;
	bool			m_angleLocked;

protected:

	vsCollisionObject *		m_object;

public:
	static vsPhysicsSprite * Load(const vsString &filename, float density, int colFlags, int testFlags);

	vsPhysicsSprite( vsDisplayList *list, float density, int colFlags, int testFlags, bool boxType = false, bool isStatic = false );
	virtual ~vsPhysicsSprite();

	vsCollisionObject * GetColObject() { return m_object; }

//	b2Body *			GetBody() { return m_body; }
//	b2BodyDef *			GetBodyDef() { return &m_bodyDef; }
//	void				SetBody( b2Body *body ) { m_body = body; if (m_body ) {m_body->m_userData = this;} }

//	float				GetMass() { return m_body->m_mass; }
//	float				GetMomentOfInertia() { return m_body->m_I; }

	void				SetCollisionsActive( bool active );

	virtual void			SetPosition( const vsVector2D &pos );
	virtual void			SetAngle( const vsAngle &ang );

	void				SetVelocity(const vsVector2D &v);	// these two functions should only be called when initially spawning an object!  Calling at any other time can lead to collision troubles!
	void				SetAngularVelocity(float v);		//

	colCircle		GetBoundingCircle() {return m_boundingCircle;}

	void			SetAngleLocked(bool locked);

	void			AddForce( const vsVector2D &force );
	void			AddTorque( float torque );

	vsVector2D			GetVelocity();
	float				GetAngularVelocity();

	void				ApplyImpulse( const vsVector2D &where, const vsVector2D &impulse );

	virtual void	Update( float timeStep );
	virtual void	Draw( vsRenderQueue *queue );

	virtual bool	CollisionCallback( const colEvent &event );
	virtual void	DestroyCallback();

	virtual bool IsOnSurface() { return m_object->IsOnSurface(); }

	void			SetDestroyed();
};

#endif // PHYS_SPRITE_H
