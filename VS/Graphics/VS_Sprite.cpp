/*
 *  VS_Sprite.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Sprite.h"

#include "VS_DisplayList.h"
#include "VS_Overlay.h"
#include "VS_RenderQueue.h"

#include "VS_File.h"
#include "VS_Record.h"

extern vsTransform2D g_drawingCameraTransform;

vsSprite *
vsSprite::Load( const vsString &filename )
{
	vsSprite *result = new vsSprite;

	vsFile file(filename);
	vsRecord r;

	while( file.Record(&r) )
	{
		if ( r.GetLabel().AsString() == "Sprite" )
		{
			result->LoadFrom(&r);
			return result;
		}
	}

	return result;
}

void
vsSprite::LoadFrom( vsRecord *record )
{
	for ( int i = 0; i < record->GetChildCount(); i++ )
	{
		vsRecord *sr = record->GetChild(i);

		vsString srLabel = sr->GetLabel().AsString();

		if ( srLabel == "Name" )
		{
			m_name = sr->GetToken(0).AsString();
			// don't use the name right now.
		}
		else if ( srLabel == "Translation" )
		{
			SetPosition( sr->Vector3D() );
		}
		else if ( srLabel == "Sprite" )
		{
			vsSprite *child = new vsSprite;
			child->LoadFrom(sr);

			AddChild( child );
		}
		else if ( srLabel == "DisplayList" )
		{
			vsDisplayList *list = vsDisplayList::Load( sr );
			SetDisplayList( list );
		}
		else if ( srLabel == "Fragment" )
		{
			vsFragment *fragment = vsFragment::Load( sr );
			AddFragment( fragment );
		}
	}
}


vsSprite::vsSprite( vsDisplayList *list ) :
	vsEntity(),
	m_displayList(list),
	m_boundingRadius(0.f),
	m_color(vsColor::White),
	m_overlay(NULL),
	m_material(NULL),
	m_useColor(false),
	m_boundingBoxLocked(false),
	m_transform()
{
	m_displayList = list;

	CalculateBoundingRadius();
}

vsSprite::~vsSprite()
{
	if ( m_displayList )
		vsDelete(m_displayList);
	vsDelete( m_material );
}

vsVector2D
vsSprite::GetWorldPosition()
{
	vsVector2D position = GetPosition();

	vsEntity *parent = m_parent;

	while(parent)
	{
		vsSprite *p = dynamic_cast<vsSprite *>(parent);
		if ( p )
		{
			position = p->m_transform.ApplyTo(position);
		}

		parent = parent->GetParent();
	}

	return position;
}

bool
vsSprite::OnScreen( const vsTransform2D & cameraTrans )
{
	if ( !m_visible )
		return false;

	vsVector2D deltaToCamera = cameraTrans.GetTranslation() - m_transform.GetTranslation();
	float sqDistanceToCamera = deltaToCamera.SqLength();

	float ebrsq = (m_boundingRadius);
	ebrsq = ebrsq * ebrsq;
	float fovsq = cameraTrans.GetScale().x;
	vsTuneable float s_factor = (16.f/9.f);	// assume worst-case aspect ratio
	fovsq *= s_factor;
	fovsq = fovsq * fovsq;
	float sqVisionRadius = ebrsq + fovsq;
//	sqVisionRadius *= sqVisionRadius;

	if ( sqDistanceToCamera > sqVisionRadius )
		return false;

	return true;
}

void
vsSprite::Draw( vsRenderQueue *queue )
{
	if ( GetVisible() )
	{
		vsDisplayList *list = queue->GetGenericList();
		vsTransform2D oldCameraTransform = g_drawingCameraTransform;

		if ( m_useColor )
			list->SetColor( m_color );

		if ( m_material )
		{
			list->SetMaterial( m_material );
		}

		list->PushTransform( m_transform );
		queue->PushTransform2D( m_transform );

		if ( m_child )
		{
			g_drawingCameraTransform = m_transform.ApplyInverseTo(g_drawingCameraTransform);
		}

		if ( m_displayList )
		{
			list->Append( *m_displayList );
		}
		else
		{
			DynamicDraw( queue );
			list->Mark();
		}

		if ( !m_fragment.IsEmpty() )
		{
			for( vsListStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
			{
				vsFragment *fragment = *iter;
				queue->AddBatch( fragment->GetMaterial(), queue->GetMatrix(), fragment->GetDisplayList() );
			}
		}

//		Parent::Draw(list);
		DrawChildren(queue);

		list->PopTransform();
		queue->PopMatrix();

		g_drawingCameraTransform = oldCameraTransform;
	}
}

void
vsSprite::SetDisplayList( vsDisplayList *list )
{
	m_displayList = list;
	if ( m_displayList )
	{
		m_displayList->Mark();
	}
	CalculateBoundingRadius();
}

void
vsSprite::AddFragment( vsFragment *fragment )
{
	m_fragment.AddItem( fragment );
}

void
vsSprite::ClearFragments()
{
	m_fragment.Clear();
}


void
vsSprite::BuildBoundingBox()
{
	if ( m_boundingBoxLocked )
		return;
	
	vsBox2D boundingBox;
	vsVector2D tl, br;
	if ( m_displayList )
	{
		m_displayList->GetBoundingBox(tl,br);
		boundingBox.Set( tl, br );
	}
	for ( vsLinkedListStore<vsFragment>::Iterator iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		fragment->GetDisplayList()->GetBoundingBox(tl,br);
		boundingBox.ExpandToInclude(tl);
		boundingBox.ExpandToInclude(br);
	}

	vsEntity *c = m_child;

	while ( c )
	{
		vsSprite *childSprite = dynamic_cast<vsSprite*>(c);
		if ( childSprite )
		{
			childSprite->BuildBoundingBox();
			vsBox2D childBox = childSprite->GetBoundingBox();

			boundingBox.ExpandToInclude( childBox + childSprite->GetPosition() );
		}
		c = c->GetNext();
	}

	SetBoundingBox( boundingBox );
}

void
vsSprite::CalculateBoundingRadius()
{
	m_boundingRadius = 0.f;
	if ( m_displayList )
	{
		m_boundingRadius = m_displayList->GetBoundingRadius();

		/*vsVector2D tl, br;
		m_displayList->GetBoundingBox(tl,br);
		m_boundingBox.Set(tl,br);*/
	}
	for ( vsLinkedListStore<vsFragment>::Iterator iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		m_boundingRadius = vsMax( m_boundingRadius, fragment->GetDisplayList()->GetBoundingRadius() );
	}
	BuildBoundingBox();
}

bool
vsSprite::ContainsLocalPoint(const vsVector2D &pos)
{
	return m_boundingBox.ContainsPoint(pos);
}

vsEntity *
vsSprite::FindEntityAtPosition(const vsVector2D &pos)
{
	if ( !m_clickable || !m_visible )
		return NULL;

	vsVector2D localPos = m_transform.ApplyInverseTo(pos);

	vsEntity *result = NULL;
	bool isInside = ContainsLocalPoint(localPos);

	if ( isInside )
	{
		result = this;

		vsEntity *inChild = Parent::FindEntityAtPosition(localPos);

		if ( inChild )
			result = inChild;
	}

	return result;
}

void
vsSprite::Rotate(float angle)
{
    vsAngle a = m_transform.GetAngle();
    a.Rotate(angle);
    m_transform.SetAngle( a );
}
