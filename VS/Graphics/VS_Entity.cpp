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
	m_parent(NULL),
	m_child(NULL),
	m_visible(true)
{
	m_next = m_prev = this;
	m_clickable = true;
}

vsEntity::~vsEntity()
{
	if ( m_parent )
		m_parent->RemoveChild(this);
	else
		Extract();

	// now, delete our children.  Remember that they'll be extracting themselves from our child list as they're deleted,
	// so we need to check what the next child is before we delete the previous child!

	vsEntity *child = m_child;

	while(child)
	{
		vsEntity *next = child->m_next;

		if ( next == child )
			next = NULL;

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
	sprite->m_parent = this;
	sprite->m_next = m_child;
	sprite->m_prev = NULL;

	if ( m_child )
		m_child->m_prev = sprite;

	m_child = sprite;
}

void
vsEntity::RemoveChild( vsEntity *sprite )
{
	if ( m_child == sprite )
	{
		m_child = m_child->m_next;
	}
	sprite->m_parent = NULL;
	sprite->Extract();
}

void
vsEntity::Extract()
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
	if ( m_visible )
	{
		DynamicDraw(queue);
		DrawChildren(queue);
	}
}

void
vsEntity::Update( float timeStep )
{
	vsEntity *child = m_child;

	while ( child )
	{
		vsEntity *next = child->m_next;

		child->Update( timeStep );

		child = next;
	}
}

void
vsEntity::RegisterOnScene(int scene)
{
	vsScreen::Instance()->GetScene(scene)->RegisterEntityOnTop(this);
}

vsEntity *
vsEntity::Find( const vsString &name )
{
	if ( m_name == name )
		return this;

	vsEntity *result = NULL;

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
vsEntity::FindEntityAtPosition(const vsVector2D &pos)
{
	if ( !m_clickable || !m_visible )
		return NULL;

	vsEntity *result = NULL;

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
