/*
 *  COL_Object.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 31/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef COL_OBJECT_H
#define COL_OBJECT_H

#include "VS/Math/VS_Transform.h"

#include "VS_DisableDebugNew.h"
#include <Box2D.h>
#include "VS_EnableDebugNew.h"

enum{
	ColFlag_None		= 0,
	ColFlag_Player		= BIT(0),
	ColFlag_Shot		= BIT(1),
	ColFlag_Enemy		= BIT(2),
	ColFlag_Particle	= BIT(3),
	ColFlag_World		= BIT(4),
	ColFlag_Reserved0	= BIT(5),
	ColFlag_Reserved1	= BIT(6),
	ColFlag_Reserved2	= BIT(7),
	ColFlag_Reserved3	= BIT(8),

	ColFlag_User0		= BIT(9),
	ColFlag_User1		= BIT(10),
	ColFlag_User2		= BIT(11),
	ColFlag_User3		= BIT(12),
	ColFlag_User4		= BIT(13),
	ColFlag_User5		= BIT(14),
	ColFlag_User6		= BIT(15),

	ColFlag_All			= 0xffff
};



struct colLine
{
	vsVector2D	a;
	vsVector2D	b;

	float		length;
};

struct colCircle
{
	vsVector2D	center;
	float		radius;
};

class vsCollisionObject;

struct colEvent
{
	vsTransform2D collisionTransform;		// object transform at time of collision
	vsVector2D collisionPoint;			// precise point where collision happened
	vsVector2D collisionNormal;			// collision force direction
	vsVector2D collisionVelocity;		// velocity of other object
	float t;							// time through the frame at which the collision happened
	int colFlags;						// what collision type was the other object?

	vsCollisionObject *otherObject;				// what was the other object?
};

class vsCollisionResponder
{
public:
	virtual ~vsCollisionResponder() {}

	virtual bool		CollisionCallback( const colEvent &event ) = 0;
	virtual void		DestroyCallback() = 0;
};

class vsCollisionObject
{
#define MAX_BOXES (32)
#define MAX_CIRCLES (1)
#define MAX_JOINTS (4)

	b2FixtureDef	m_boxDef[MAX_BOXES];
	int				m_boxCount;
	b2FixtureDef	m_circleDef[MAX_CIRCLES];
	int				m_circleCount;
	b2BodyDef		m_bodyDef;
	b2Body *		m_body;
	b2RevoluteJoint *		m_joint[MAX_JOINTS];
	vsCollisionObject *		m_jointPartner[MAX_JOINTS];
	int				m_jointCount;

	void *			m_userData;

	int				m_colFlags;		// what type of object am I?
	int				m_testFlags;	// what type of objects should I test against?

	bool		m_destroyed;

	bool		m_static;			// if true, we can never, ever move.  :)

	vsCollisionResponder *	m_responder;

	void		RegisterObject();
	void		DeregisterObject();
	void		SetCollisionsActive_Internal( bool active );

public:
				vsCollisionObject(int colFlags, int testFlags, bool isStatic = false);
	virtual		~vsCollisionObject();

	void		NotifyCollision(vsCollisionObject *otherObject, const vsVector2D &where);

	void		SetCircle(colCircle *circle, float density);
	void		SetBox(const vsVector2D &topLeft, const vsVector2D &bottomRight, float density);

	void		SetPolygon(const vsVector2D *v, int vertexCount, const vsAngle &orientation, float density);
	void		SetBox(const vsVector2D &extents, const vsVector2D &position, const vsAngle &orientation, float density);
	void		AddBox(const vsVector2D &extents, const vsVector2D &position, const vsAngle &orientation, float density);

	void		AddRotationJointTo(vsCollisionObject *other, const vsVector2D &jointPos, float minAngle = -360.f, float maxAngle = 360.f);
	void		DestroyRotationJointTo(vsCollisionObject *other);

	void		NotifyJointRemoved(b2Joint *joint);
	void		NotifyJointAdded(b2Joint *joint, vsCollisionObject *other);

	void SetJointMotorSpeed( int jointId, float speed, float maxTorque );

	void		Init();
	void		Deinit();

	bool		IsActive() { return !!(m_body); }

	virtual void		Update( float timeStep );

	vsVector2D			GetPosition();
	vsAngle				GetAngle();
	void				SetPosition( const vsVector2D &position );
	void				SetAngle( const vsAngle &angle );

	void				SetAngleLocked( bool locked );

	void				AddForce( const vsVector2D &force );
	void				AddTorque( float torque );

	void				SetVelocity( const vsVector2D &velocity, float angularVelocity );
	void				SetCollisionsActive( bool active );

	void				SetFriction( float friction );
	void				SetRestitution( float restitution );

	b2Body *			GetBody() { return m_body; }
	b2BodyDef *			GetBodyDef() { return &m_bodyDef; }
	void				SetBody( b2Body *body ) { m_body = body; if ( m_body ) {m_body->SetUserData( this );} }

	float				GetMass() { return m_body->GetMass(); }
	float				GetMomentOfInertia() { return m_body->GetInertia(); }

	virtual bool		CollisionCallback( const colEvent &collision );
	virtual void		DestroyCallback();

	void				SetResponder( vsCollisionResponder *responder ) { m_responder = responder; }

	void				SetTransform( const vsTransform2D &t );
	const vsTransform2D &	GetTransform();

	void				SetVelocity( const vsVector2D &velocity );
	vsVector2D			GetVelocity();
	void				SetAngularVelocity( float rv );
	float				GetAngularVelocity();

	void				SetDestroyed();

	void				SetUserData( void * userData ) { m_userData = userData; }
	void *				GetUserData() { return m_userData; }

	bool				IsOnSurface();	// returns the object
};

#endif // COL_OBJECT_H
