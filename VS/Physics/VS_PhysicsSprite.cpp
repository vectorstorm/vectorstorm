/*
 *  PHYS_Sprite.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 4/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_PhysicsSprite.h"

#include "VS_CollisionSystem.h"
#include "VS_DisplayList.h"
#include "VS_RenderQueue.h"

vsPhysicsSprite *
vsPhysicsSprite::Load(const vsString &filename, float density, int colFlags, int testFlags)
{
	return new vsPhysicsSprite( vsDisplayList::Load(filename), density, colFlags, testFlags );
}

vsPhysicsSprite::vsPhysicsSprite(vsDisplayList *list, float density, int colFlags, int testFlags, bool boxType, bool isStatic) :
	vsSprite(list),
	m_boxType(boxType)
{
	m_colFlags = colFlags;
	m_testFlags = testFlags;

	m_object = new vsCollisionObject( colFlags, testFlags, isStatic );
	m_object->SetResponder(this);

	m_boundingCircle.center = vsVector2D::Zero;
	if ( list )
	{
		m_boundingCircle.radius = list->GetBoundingRadius();
		m_boundingCircle.radius *= 0.8f;

		if ( boxType )
		{
			vsVector2D topLeft;
			vsVector2D bottomRight;
			list->GetBoundingBox(topLeft,bottomRight);

//			m_offset = (topLeft + bottomRight) * 0.5f;

			m_object->SetBox(topLeft,bottomRight, density);
		}
		else
			m_object->SetCircle(&m_boundingCircle, density);
	}



	/*
	m_circleDef.radius = m_boundingCircle.radius;
	m_circleDef.density = density;
	m_circleDef.categoryBits = colFlags;
	m_circleDef.maskBits = testFlags;
	m_circleDef.friction = 0.1f;
	m_circleDef.restitution = 0.3f;

	vsVector2D topLeft;
	vsVector2D bottomRight;
	list->GetBoundingBox(topLeft,bottomRight);

	vsVector2D avg = (topLeft + bottomRight) * 0.5f;

	float width = bottomRight.x - topLeft.x;
	float height = bottomRight.y - topLeft.y;

	m_boxDef.extents.Set(width * 0.5f, height * 0.5f);
	m_boxDef.density = density;
	m_boxDef.categoryBits = colFlags;
	m_boxDef.maskBits = testFlags;
	m_boxDef.friction = 0.1f;
	m_boxDef.restitution = 0.3f;

	if ( m_boxType )
	{
		m_bodyDef.position.Set(avg.x, avg.y);
		m_bodyDef.AddShape( &m_boxDef );
	}
	else
	{
		m_bodyDef.position.Set(0.f,0.f);
		m_bodyDef.AddShape( &m_circleDef );
	}
	m_body = NULL;
	*/


	// ordinarily, our angular mass (also known as "moment of inertia" for some reason I don't understand) is defined as
	// ANGULAR_MASS = MASS * (RADIUS*RADIUS).
}

vsPhysicsSprite::~vsPhysicsSprite()
{
	SetCollisionsActive(false);

	vsDelete( m_object );
}

void
vsPhysicsSprite::Update( float timeStep )
{
	m_object->Update(timeStep);

	if ( m_object && m_object->IsActive() )
	{
		vsVector2D position = m_object->GetPosition();
		vsAngle rotation = m_object->GetAngle();

		SetPosition( vsVector2D( position.x, position.y ) );
		SetAngle( vsAngle( rotation ) );

	}

	Parent::Update(timeStep);
}

void
vsPhysicsSprite::SetPosition( const vsVector2D &pos )
{
	m_object->SetPosition( pos );

	Parent::SetPosition( pos );
}

void
vsPhysicsSprite::SetAngle( const vsAngle &ang )
{
	m_object->SetAngle( ang );

	Parent::SetAngle( ang );
}


void
vsPhysicsSprite::SetVelocity( const vsVector2D &velocity )
{
	m_object->SetVelocity( velocity );
}

vsVector2D
vsPhysicsSprite::GetVelocity()
{
	return m_object->GetVelocity();
}

void
vsPhysicsSprite::SetAngularVelocity(float angularVelocity)
{
	m_object->SetAngularVelocity(angularVelocity);
}

float
vsPhysicsSprite::GetAngularVelocity()
{
	return m_object->GetAngularVelocity();
}

void
vsPhysicsSprite::AddForce( const vsVector2D &force )
{
	m_object->AddForce(force);
}

void
vsPhysicsSprite::AddTorque( float torque )
{
	m_object->AddTorque(torque);
}

void
vsPhysicsSprite::SetCollisionsActive(bool active)
{
	m_object->SetCollisionsActive(active);
	m_object->SetUserData(this);
}

void
vsPhysicsSprite::SetAngleLocked(bool locked)
{
	m_object->SetAngleLocked(locked);
}

void
vsPhysicsSprite::Draw( vsRenderQueue *queue )
{
	Parent::Draw(queue);

#if defined(_DEBUG)

	vsDisplayList *list = queue->GetGenericList();

	vsTuneable bool drawVelocity = false;

	if ( drawVelocity )
	{
		list->SetColor( c_red );
		list->MoveTo( GetPosition() );
		list->LineTo( GetPosition() + GetVelocity() );
	}

#endif
}

bool
vsPhysicsSprite::CollisionCallback( const colEvent &collision )
{
	UNUSED(collision);
	return false;
}

void
vsPhysicsSprite::DestroyCallback()
{
}

void
vsPhysicsSprite::SetDestroyed()
{
	m_object->SetDestroyed();
}

