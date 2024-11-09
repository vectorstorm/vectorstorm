/*
 *  VS_Entity.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Entity.h"
#include "VS_DisplayList.h"
#include "VS_Scene.h"
#include "VS_Screen.h"
#include "VS_System.h"

vsEntity::vsEntity():
	m_name( vsEmptyString ),
	m_parent(nullptr),
	m_child(nullptr),
	m_registeredScene(nullptr),
	m_visible(true),
	m_processing(false),
	m_extractQueued(false)
{
	m_next = m_prev = this;
	m_clickable = true;
}

vsEntity::~vsEntity()
{
	if ( m_parent )
		m_parent->RemoveChild(this);
	else
	{
		vsAssert( !m_processing, "Destroyed while processing this entity??" );
		DoExtract();
	}

	// now, delete our children.  Remember that they'll be extracting
	// themselves from our child list as they're deleted, so we need to check
	// what the next child is before we delete the previous child!

	vsEntity *child = m_child;

	while(child)
	{
		vsEntity *next = child->m_next;

		if ( next == child )
			next = nullptr;

		delete child;
		child = next;
	}
}

void
vsEntity::Append( vsEntity *sprite )
{
	sprite->m_next = m_next;
	sprite->m_prev = this;

	m_next->m_prev = sprite;
	m_next = sprite;
}

void
vsEntity::Prepend( vsEntity *sprite )
{
	sprite->m_next = this;
	sprite->m_prev = m_prev;

	m_prev->m_next = sprite;
	m_prev = sprite;
}

void
vsEntity::AddChild( vsEntity *sprite )
{
	sprite->Extract();

	sprite->m_parent = this;
	sprite->m_next = m_child;
	sprite->m_prev = nullptr;

	if ( m_child )
		m_child->m_prev = sprite;

	m_child = sprite;
}

void
vsEntity::RemoveChild( vsEntity *sprite )
{
	vsAssert( sprite->m_parent == this, "Entity isn't a child of me?" );
	if ( m_child == sprite )
	{
		m_child = m_child->m_next;
	}
	sprite->m_parent = nullptr;
	sprite->Extract();
}

void
vsEntity::DoExtract()
{
	if ( m_parent )
	{
		m_parent->RemoveChild( this );
	}
	else
	{
		if ( m_prev )
			m_prev->m_next = m_next;
		if ( m_next )
			m_next->m_prev = m_prev;

		m_prev = m_next = this;
	}
	m_extractQueued = false;
	m_registeredScene = nullptr;
}

void
vsEntity::Extract()
{
	if ( m_processing )
		m_extractQueued = true;
	else
		DoExtract();
}

void
vsEntity::QueueExtract()
{
	m_extractQueued = true;
}

void
vsEntity::DrawChildren( vsRenderQueue *queue )
{
	vsEntity *child = m_child;

	while ( child )
	{
		if ( child->OnScreen(g_drawingCameraTransform) )
		{
			child->Draw( queue );
		}

		child = child->m_next;
	}
}

void
vsEntity::Draw( vsRenderQueue *queue )
{
	m_processing = true;

	if ( m_visible )
	{
		DynamicDraw(queue);
		DrawChildren(queue);
	}

	m_processing = false;
	if ( m_extractQueued )
		DoExtract();
}

void
vsEntity::Update( float timeStep )
{
	m_processing = true;

	vsEntity *child = m_child;

	while ( child )
	{
		child->Update( timeStep );
		child = child->m_next;
	}

	m_processing = false;
	if ( m_extractQueued )
		DoExtract();
}

void
vsEntity::RegisterOnScene(int sceneId)
{
	vsScene *scene = vsScreen::Instance()->GetScene(sceneId);
	RegisterOnScene(scene);
}

void
vsEntity::RegisterOnScene(vsScene *scene)
{
	scene->RegisterEntityOnTop(this);
	m_registeredScene = scene;
}

vsEntity *
vsEntity::Find( const vsString &name ) const
{
	if ( m_name == name )
		return const_cast<vsEntity*>(this);

	vsEntity *result = nullptr;

	for( vsEntity *child = m_child; child; child = child->m_next )
	{
		result = child->Find(name);

		if ( result )
		{
			break;
		}
	}

	return result;
}

vsEntity *
vsEntity::FindEntityAtPosition(const vsVector2D &pos) const
{
	if ( !m_clickable || !m_visible )
		return nullptr;

	vsEntity *result = nullptr;

	vsEntity *child = m_child;

	while ( !result && child )
	{
		vsEntity *next = child->m_next;

		result = child->FindEntityAtPosition(pos);

		child = next;
	}

	return result;
}

#if defined(_DEBUG)

void
vsEntity::RegisterOnDebugScene()
{
	vsScreen::Instance()->GetDebugScene()->RegisterEntityOnTop(this);
}

#endif // _DEBUG
