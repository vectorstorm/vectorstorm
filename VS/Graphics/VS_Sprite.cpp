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
	m_color(c_white),
	m_overlay(nullptr),
	m_material(nullptr),
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
vsSprite::GetWorldPosition() const
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
vsSprite::OnScreen( const vsTransform2D & cameraTrans ) const
{
	if ( !m_visible )
		return false;

// 	vsVector2D deltaToCamera = cameraTrans.GetTranslation() - m_transform.GetTranslation();
// 	float sqDistanceToCamera = deltaToCamera.SqLength();
//
// 	float ebrsq = (m_boundingRadius);
// 	ebrsq = ebrsq * ebrsq;
// 	float fovsq = cameraTrans.GetScale().x;
// 	vsTuneable float s_factor = (16.f/9.f);	// assume worst-case aspect ratio
// 	fovsq *= s_factor;
// 	fovsq = fovsq * fovsq;
// 	float sqVisionRadius = ebrsq + fovsq;
// //	sqVisionRadius *= sqVisionRadius;
//
// 	if ( sqDistanceToCamera > sqVisionRadius )
// 		return false;

	return true;
}

void
vsSprite::Draw( vsRenderQueue *queue )
{
	if ( GetVisible() )
	{
		vsTransform2D oldCameraTransform = g_drawingCameraTransform;

		queue->PushTransform2D( m_transform );

		if ( m_child )
		{
			g_drawingCameraTransform = m_transform.ApplyInverseTo(g_drawingCameraTransform);
		}

		if ( m_displayList )
		{
			// TODO:  Move the PushTransform2D/PopTransform functionality to here,
			// using the full local-to-world matrix we've already calculated.
			//
			// Annoyingly, I have some test cases that don't work this way, but
			// I haven't quite been able to work out why they don't.  Should
			// always work, as long as nobody overrides ::Draw() or ::DynamicDraw
			// and expects that there'll be an up-to-date transform stack in
			// the generic list.
			//
			vsDisplayList *list = queue->GetGenericList();
			if ( m_useColor )
				list->SetColor( m_color );
			if ( m_material )
				list->SetMaterial( m_material );
			list->SetMatrix4x4( queue->GetMatrix() );
			list->Append( *m_displayList );
			list->PopTransform();
		}
		else
		{
			DynamicDraw( queue );
		}

		if ( !m_fragment.IsEmpty() )
		{
			for( vsArrayStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
			{
				if ( iter->IsVisible() )
					queue->AddFragmentBatch( *iter );
			}
		}

		DrawChildren(queue);

		queue->PopMatrix();

		g_drawingCameraTransform = oldCameraTransform;
	}
}

void
vsSprite::SetDisplayList( vsDisplayList *list )
{
	m_displayList = list;
	CalculateBoundingRadius();
}

void
vsSprite::AddFragment( vsFragment *fragment )
{
	if ( fragment )
		m_fragment.AddItem( fragment );
}

void
vsSprite::DestroyFragment( vsFragment *fragment )
{
	if ( fragment )
		m_fragment.RemoveItem( fragment );
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
	for ( vsArrayStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		if ( fragment->IsSimple() )
		{
			vsRenderBuffer *b = fragment->GetSimpleVBO();
			for (int i = 0; i < b->GetPositionCount(); i++ )
				boundingBox.ExpandToInclude( b->GetPosition(i) );
		}
		else
		{
			if ( fragment->GetDisplayList() )
			{
				fragment->GetDisplayList()->GetBoundingBox(tl,br);
				boundingBox.ExpandToInclude(tl);
				boundingBox.ExpandToInclude(br);
			}
		}
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
	for ( vsArrayStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		if ( fragment->IsSimple() )
		{
			vsRenderBuffer *b = fragment->GetSimpleVBO();
			for (int i = 0; i < b->GetPositionCount(); i++ )
				m_boundingRadius = vsMax( m_boundingRadius, b->GetPosition(i).Length() );
		}
		else if ( fragment->GetDisplayList() )
		{
			m_boundingRadius = vsMax( m_boundingRadius, fragment->GetDisplayList()->GetBoundingRadius() );
		}
	}
	BuildBoundingBox();
}

bool
vsSprite::ContainsLocalPoint(const vsVector2D &pos) const
{
	return m_boundingBox.ContainsPoint(pos);
}

vsEntity *
vsSprite::FindEntityAtPosition(const vsVector2D &pos) const
{
	if ( !m_clickable || !m_visible )
		return nullptr;

	vsVector2D localPos = m_transform.ApplyInverseTo(pos);

	vsEntity *result = nullptr;
	bool isInside = ContainsLocalPoint(localPos);

	if ( isInside )
	{
		result = const_cast<vsSprite*>(this);

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
