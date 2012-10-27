/*
 *  VS_Model.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 9/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Model.h"

#include "VS_DisplayList.h"
#include "VS_RenderQueue.h"

#include "VS_File.h"
#include "VS_Record.h"


vsModel *
vsModel::Load( const vsString &filename )
{
	vsModel *result = new vsModel;

	vsFile file(filename);
	vsRecord r;

	while( file.Record(&r) )
	{
		if ( r.GetLabel().AsString() == "Model" )
		{
			result->LoadFrom(&r);
			return result;
		}
	}

	return result;
}

void
vsModel::LoadFrom( vsRecord *record )
{
	for ( int i = 0; i < record->GetChildCount(); i++ )
	{
		vsRecord *sr = record->GetChild(i);

		vsString srLabel = sr->GetLabel().AsString();

		if ( srLabel == "name" )
		{
			m_name = sr->GetToken(0).AsString();
			// don't use the name right now.
		}
		else if ( srLabel == "translation" )
		{
			SetPosition( sr->Vector3D() );
		}
		else if ( srLabel == "Model" )
		{
			vsModel *child = new vsModel;
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


vsModel::vsModel( vsDisplayList *list ):
	m_material(NULL),
	m_displayList(list)
{
}

vsModel::~vsModel()
{
	m_fragment.Clear();

	if ( m_displayList )
		vsDelete(m_displayList);
	vsDelete( m_material );
}

void
vsModel::SetDisplayList( vsDisplayList *list )
{
	if ( m_displayList )
		vsDelete( m_displayList );
	m_displayList = list;

	BuildBoundingBox();
}

void
vsModel::AddFragment( vsFragment *fragment )
{
	m_fragment.AddItem( fragment );
}

void
vsModel::ClearFragments()
{
	m_fragment.Clear();
}

void
vsModel::BuildBoundingBox()
{
	vsBox3D boundingBox;
	if ( m_displayList )
	{
		m_displayList->GetBoundingBox(boundingBox);
	}
	for ( vsLinkedListStore<vsFragment>::Iterator iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
	{
		vsFragment *fragment = *iter;
		vsBox3D fragmentBox;
		fragment->GetDisplayList()->GetBoundingBox( fragmentBox );
		boundingBox.ExpandToInclude( fragmentBox );
	}

	vsEntity *c = m_child;

	while ( c )
	{
		vsModel *childSprite = dynamic_cast<vsModel*>(c);
		if ( childSprite )
		{
			childSprite->BuildBoundingBox();
			vsBox3D childBox = childSprite->GetBoundingBox();

			boundingBox.ExpandToInclude( childBox + childSprite->GetPosition() );
		}
		c = c->GetNext();
	}

	SetBoundingBox( boundingBox );
}



void
vsModel::Draw( vsRenderQueue *queue )
{
	if ( GetVisible() )
	{
		vsDisplayList *list = queue->GetGenericList();
		if ( m_material )
		{
			list->SetMaterial( m_material );
		}

		bool hasTransform = (m_transform != vsTransform3D::Identity);

		if ( hasTransform )
		{
			queue->PushMatrix( m_transform.GetMatrix() );
		}
		list->SetMatrix4x4( queue->GetMatrix() );

		if ( m_displayList )
		{
			list->Append( *m_displayList );
		}
		else
		{
			DynamicDraw( queue );
		}

		if ( !m_fragment.IsEmpty() )
		{
			for( vsListStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
			{
				queue->AddFragmentBatch( *iter );
			}
		}

		//		Parent::Draw(list);
		DrawChildren(queue);

		if ( hasTransform )
		{
			queue->PopMatrix();
		}
		list->PopTransform();
	}
}
