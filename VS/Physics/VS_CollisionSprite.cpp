/*
 *  COL_Sprite.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/04/07.
 *  Copyright 2007 PanicKitten Softworks. All rights reserved.
 *
 */

#include "VS_CollisionSprite.h"

#include "VS_CollisionObject.h"
#include "VS_CollisionSystem.h"
#include "Core.h"
#include "CORE_Game.h"

#include "VS_DisplayList.h"

vsCollisionSprite *
vsCollisionSprite::Load(const vsString &filename, int colFlags, int testFlags)
{
	return new vsCollisionSprite( vsDisplayList::Load(filename), colFlags, testFlags );
}

vsCollisionSprite::vsCollisionSprite(vsDisplayList *list, int colFlags, int testFlags) : vsSprite(list), m_vsCollisionObject(nullptr)
{
	m_colObject = new colObject(colFlags, testFlags);
	m_colObject->SetResponder(this);
	colCircle circle;

	list->GetBoundingCircle( circle.center, circle.radius );
	//circle.center = vsVector2D::Zero;
	circle.radius *= 0.8f;
	m_colObject->SetBoundingCircle( circle );
}

vsCollisionSprite::~vsCollisionSprite()
{
	if ( m_colObject )
		delete m_colObject;
}

void
vsCollisionSprite::Teleport()
{
	m_colObject->Teleport();
}

void
vsCollisionSprite::Update( float timeStep )
{
	m_colObject->SetTransform( m_transform );

	Parent::Update(timeStep);
}

bool
vsCollisionSprite::CollisionCallback( const colEvent &collision )
{
	m_transform = collision.collisionTransform;
	m_colObject->SetTransform(m_transform);
	m_colObject->Teleport();

	return true;
}

void
vsCollisionSprite::DestroyCallback()
{
}
