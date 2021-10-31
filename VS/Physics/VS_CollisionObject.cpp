/*
 *  COL_Object.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 31/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "Core.h"
#include "CORE_Game.h"

#include "VS_System.h"

#include "VS_CollisionObject.h"
#include "VS_CollisionSystem.h"

vsCollisionObject::vsCollisionObject(int colFlags, int testFlags, bool isStatic):
m_responder(nullptr)
{
	m_colFlags = colFlags;
	m_testFlags = testFlags;

//	list->GetBoundingBox(topLeft,bottomRight);

	m_body = nullptr;
	m_userData = nullptr;

	m_destroyed = false;
	m_static = isStatic;

	m_boxCount = 0;
	m_circleCount = 0;
	m_jointCount = 0;

	for ( int i = 0; i < MAX_JOINTS; i++ )
		m_joint[i] = nullptr;

	m_bodyDef.userData = this;
	m_bodyDef.type = (isStatic)? b2_staticBody : b2_dynamicBody;
}

vsCollisionObject::~vsCollisionObject()
{
	SetCollisionsActive(false);

	for ( int i = 0; i < m_circleCount; i++ )
	{
		vsDelete( m_circleDef[i].shape );
	}
	for ( int i = 0; i < m_boxCount; i++ )
	{
		vsDelete( m_boxDef[i].shape );
	}
}

void
vsCollisionObject::SetCircle(colCircle *circle, float density)
{
	//	radius = circle->radius;

	b2CircleShape *cs = new b2CircleShape;
	cs->m_radius = circle->radius;
	m_circleDef[m_circleCount].shape = cs;
	m_circleDef[m_circleCount].density = density;
	m_circleDef[m_circleCount].filter.categoryBits = m_colFlags;
	m_circleDef[m_circleCount].filter.maskBits = m_testFlags;
	m_circleDef[m_circleCount].friction = 0.1f;
	m_circleDef[m_circleCount].restitution = 0.3f;
	m_circleCount++;
}

void
vsCollisionObject::SetBox(const vsVector2D &topLeft, const vsVector2D &bottomRight, float density)
{
	vsAssert(m_boxCount == 0, "Error, tried to setbox on something that was already set up!");
	vsVector2D avg = (topLeft + bottomRight) * 0.5f;

	float width = bottomRight.x - topLeft.x;
	float height = bottomRight.y - topLeft.y;

	width = vsMax( width, 0.1f );
	height = vsMax( height, 0.1f );

	b2PolygonShape *ps = new b2PolygonShape;
	ps->SetAsBox(width * 0.5f, height * 0.5f);

	m_boxDef[m_boxCount].shape = ps;
	m_boxDef[m_boxCount].density = density;
	m_boxDef[m_boxCount].filter.categoryBits = m_colFlags;
	m_boxDef[m_boxCount].filter.maskBits = m_testFlags;
	m_boxDef[m_boxCount].friction = 0.1f;
	m_boxDef[m_boxCount].restitution = 0.3f;
	m_boxCount++;

	m_bodyDef.position.Set(avg.x, avg.y);
}

void
vsCollisionObject::SetPolygon(const vsVector2D *v, int vertexCount, const vsAngle &orientation, float density)
{
	vsAssert( vertexCount < b2_maxPolygonVertices, "Too many vertices on collision polygon!" );
	b2PolygonShape *ps = new b2PolygonShape;
	for ( int i = 0; i < vertexCount; i++ )
	{
		ps->m_vertices[i].Set( v[i].x, v[i].y );
	}
	ps->m_count = vertexCount;

	m_boxDef[m_boxCount].shape = ps;
	m_boxDef[m_boxCount].density = density;
	m_boxDef[m_boxCount].filter.categoryBits = m_colFlags;
	m_boxDef[m_boxCount].filter.maskBits = m_testFlags;
	m_boxDef[m_boxCount].friction = 0.1f;
	m_boxDef[m_boxCount].restitution = 0.3f;
	m_boxCount++;
}

void
vsCollisionObject::SetBox(const vsVector2D &extents, const vsVector2D &position, const vsAngle &orientation, float density)
{
	float width = extents.x;
	float height = extents.y;

	b2PolygonShape *ps = new b2PolygonShape;
	ps->SetAsBox(width * 0.5f, height * 0.5f);

	m_boxDef[m_boxCount].shape = ps;
	m_boxDef[m_boxCount].density = density;
	m_boxDef[m_boxCount].filter.categoryBits = m_colFlags;
	m_boxDef[m_boxCount].filter.maskBits = m_testFlags;
	m_boxDef[m_boxCount].friction = 0.1f;
	m_boxDef[m_boxCount].restitution = 0.3f;

	m_bodyDef.position.Set(position.x, position.y);
	m_bodyDef.angle = orientation.Get();
	m_boxCount++;
}

void
vsCollisionObject::AddBox(const vsVector2D &extents, const vsVector2D &position, const vsAngle &orientation, float density)
{
	float width = extents.x;
	float height = extents.y;

	b2Vec2 center(position.x,position.y);
	float32 angle = orientation.Get();

	b2PolygonShape *ps = new b2PolygonShape;
	ps->SetAsBox(width * 0.5f, height * 0.5f, center, angle);

	m_boxDef[m_boxCount].shape = ps;
	m_boxDef[m_boxCount].density = density;
	m_boxDef[m_boxCount].filter.categoryBits = m_colFlags;
	m_boxDef[m_boxCount].filter.maskBits = m_testFlags;
	m_boxDef[m_boxCount].friction = 0.5f;
	m_boxDef[m_boxCount].restitution = 0.1f;

	m_boxCount++;
}

void
vsCollisionObject::AddRotationJointTo(vsCollisionObject *other, const vsVector2D &jointPos, float minAngle, float maxAngle)
{
	vsAssert(m_jointCount < MAX_JOINTS, "Ran out of joints!");

	b2RevoluteJointDef	jointDef;

	vsVector2D jointPos1 = jointPos - GetPosition();
	vsVector2D jointPos2 = jointPos - other->GetPosition();

	b2Vec2 jPos1( jointPos1.x, jointPos1.y );
	b2Vec2 jPos2( jointPos2.x, jointPos2.y );

	jointDef.localAnchorA = jPos1;
	jointDef.localAnchorB = jPos2;

	jointDef.bodyA = m_body;
	jointDef.bodyB = other->GetBody();
	if ( minAngle > -360.f || maxAngle < 360.f )
		jointDef.enableLimit = true;
	jointDef.lowerAngle = DEGREES(minAngle);
	jointDef.upperAngle = DEGREES(maxAngle);
	b2Joint* baseJoint = vsCollisionSystem::Instance()->GetWorld()->CreateJoint(&jointDef);
	m_joint[m_jointCount] = dynamic_cast<b2RevoluteJoint*>(baseJoint);
	m_jointPartner[m_jointCount] = other;
	m_jointPartner[m_jointCount]->NotifyJointAdded( m_joint[m_jointCount], this );

	m_jointCount++;
}

void
vsCollisionObject::DestroyRotationJointTo(vsCollisionObject *other)
{
	for ( int i = 0; i < m_jointCount; i++ )
	{
		if ( m_joint[i] && m_jointPartner[i] == other )
		{
			other->NotifyJointRemoved( m_joint[i] );
			vsCollisionSystem::Instance()->GetWorld()->DestroyJoint(m_joint[i]);
			m_joint[i] = nullptr;
		}
	}
}

void
vsCollisionObject::NotifyJointAdded(b2Joint *joint, vsCollisionObject *other)
{
	vsAssert(m_jointCount < MAX_JOINTS, "Ran out of joints!");

	m_joint[m_jointCount] = dynamic_cast<b2RevoluteJoint*>(joint);
	m_jointPartner[m_jointCount] = other;

	m_jointCount++;
}

void
vsCollisionObject::NotifyJointRemoved(b2Joint *joint)
{
	for ( int i = 0; i < m_jointCount; i++ )
	{
		if ( m_joint[i] == joint )
		{
			m_joint[i] = nullptr;
			m_jointPartner[i] = nullptr;
		}
	}
}

void
vsCollisionObject::Init()
{
}

void
vsCollisionObject::Deinit()
{
}

void
vsCollisionObject::Update( float timeStep )
{
	UNUSED(timeStep);

	// check for collisions last frame
	if ( !m_body )
		return;
	/*
	b2ContactNode *node = m_body->GetContactList();

	colEvent evt;
	while ( node )
	{
		void *ud = node->other->GetUserData();
		if ( ud )
		{
			evt.otherObject = ((vsCollisionObject *)ud);
			evt.colFlags = evt.otherObject->m_colFlags;
		}
		else
		{
			evt.otherObject = nullptr;
			evt.colFlags = ColFlag_World;
		}
		b2Contact *contact = node->contact;
		b2Manifold *m = contact->GetManifolds();

		b2ContactPoint *cp = &m->points[0];
		vsVector2D pos( cp->position.x, cp->position.y );

		evt.collisionPoint = pos;

		CollisionCallback(evt);

		node = node->next;
	}*/
	if ( m_destroyed )
	{
		DestroyCallback();
		SetCollisionsActive(false);
		m_destroyed = false;
		return;
	}
}

void
vsCollisionObject::NotifyCollision( vsCollisionObject *other, const vsVector2D &where )
{
	colEvent evt;

	evt.otherObject = other;

	if ( other )
		evt.colFlags = evt.otherObject->m_colFlags;
	else
		evt.colFlags = ColFlag_World;

	evt.collisionPoint = where;

	CollisionCallback(evt);
}

bool
vsCollisionObject::IsOnSurface()
{
	bool result = false;

	b2ContactEdge *node = m_body->GetContactList();

	while ( node )
	{
		b2Contact *contact = node->contact;

		if ( contact->GetFixtureA()->GetFilterData().categoryBits & ColFlag_World )
			return true;
		if ( contact->GetFixtureB()->GetFilterData().categoryBits & ColFlag_World )
			return true;
		/*
		b2Manifold *m = contact->GetManifolds();

		b2ManifoldPoint *cp = &m->points[0];
		vsVector2D pos( cp->localPoint1.x, cp->localPoint1.y );

		if ( pos.y < GetPosition().y )
			result = true;*/

		node = node->next;
	}

	return result;
}

void
vsCollisionObject::SetPosition( const vsVector2D &pos )
{
	b2Vec2 p(pos.x,pos.y);

	m_body->SetTransform(p, m_body->GetAngle());
}

vsVector2D
vsCollisionObject::GetPosition()
{
	b2Vec2 p = m_body->GetPosition();

	return vsVector2D(p.x,p.y);
}

void
vsCollisionObject::SetAngle( const vsAngle &ang )
{
	m_body->SetTransform(m_body->GetPosition(), ang.Get());
}

vsAngle
vsCollisionObject::GetAngle()
{
	float r = m_body->GetAngle();

	return vsAngle(r);
}


void
vsCollisionObject::SetVelocity( const vsVector2D &velocity, float angularVelocity )
{
	SetVelocity( velocity );
	SetAngularVelocity( angularVelocity );
}

void
vsCollisionObject::SetVelocity( const vsVector2D &velocity )
{
	b2Vec2 v( velocity.x, velocity.y );

	m_body->SetLinearVelocity(v);
}

void
vsCollisionObject::SetAngularVelocity( float angularVelocity )
{
	m_body->SetAngularVelocity(angularVelocity);
}

void
vsCollisionObject::SetAngleLocked(bool locked)
{
	m_bodyDef.fixedRotation = locked;
}

void
vsCollisionObject::SetFriction(float friction)
{
	for ( int i = 0; i < m_boxCount; i++ )
		m_boxDef[i].friction = friction;
	for ( int i = 0; i < m_circleCount; i++ )
		m_circleDef[i].friction = friction;
}

void
vsCollisionObject::SetRestitution(float restitution)
{
	for ( int i = 0; i < m_boxCount; i++ )
		m_boxDef[i].restitution = restitution;
	for ( int i = 0; i < m_circleCount; i++ )
		m_circleDef[i].restitution = restitution;
}

vsVector2D
vsCollisionObject::GetVelocity()
{
	b2Vec2 v = m_body->GetLinearVelocity();
	vsVector2D vel(v.x,v.y);

	return vel;
}

float
vsCollisionObject::GetAngularVelocity()
{
	return m_body->GetAngularVelocity();
}

bool
vsCollisionObject::CollisionCallback(const colEvent &collision)
{
	bool handled = false;

	if ( m_responder )
	{
		handled = m_responder->CollisionCallback( collision );
	}

	return handled;
}

void
vsCollisionObject::SetDestroyed()
{
	m_destroyed = true;
}

void
vsCollisionObject::DestroyCallback()
{
	if ( m_responder )
		m_responder->DestroyCallback();
}

void
vsCollisionObject::AddForce( const vsVector2D &force )
{
	vsAssert(m_body, "Tried to add force without a body!");
	b2Vec2 f(force.x, force.y);
	b2Vec2 w = m_body->GetWorldCenter();
	m_body->ApplyForce( f, w, true );
}

void
vsCollisionObject::AddTorque( float torque )
{
	vsAssert(m_body, "Tried to apply torque without a body!");
	m_body->ApplyTorque( torque, true );
}

void
vsCollisionObject::SetCollisionsActive(bool active)
{
	bool currentlyActive = (m_body != nullptr);

	if ( currentlyActive != active )
	{
		if ( active )
		{
			RegisterObject();
		}
		else
		{
			for ( int i = 0; i < m_jointCount; i++ )
			{
				if ( m_joint[i] )
				{
					m_jointPartner[i]->NotifyJointRemoved(m_joint[i]);
					vsCollisionSystem::Instance()->GetWorld()->DestroyJoint(m_joint[i]);
					m_joint[i] = nullptr;
				}
			}
			DeregisterObject();
			m_body = nullptr;
		}
	}

	m_destroyed = false;
}

void
vsCollisionObject::RegisterObject()
{
	b2World *w = vsCollisionSystem::Instance()->GetWorld();

	if ( m_static )
		m_body = w->CreateBody( GetBodyDef() );
	else
		m_body = w->CreateBody( GetBodyDef() );

	for ( int i = 0; i < m_boxCount; i++ )
		m_body->CreateFixture(&m_boxDef[i]);
	for ( int i = 0; i < m_circleCount; i++ )
		m_body->CreateFixture(&m_circleDef[i]);

	m_body->ResetMassData();
}

void
vsCollisionObject::DeregisterObject()
{
	b2World *w = vsCollisionSystem::Instance()->GetWorld();

	w->DestroyBody( m_body );

	m_body = nullptr;
	m_jointCount = 0;
}

void
vsCollisionObject::SetJointMotorSpeed( int jointId, float speed, float maxTorque )
{
	m_body->SetAwake(true);
	m_joint[jointId]->EnableMotor(true);
	m_joint[jointId]->SetMotorSpeed(speed);
	m_joint[jointId]->SetMaxMotorTorque(maxTorque);
}

