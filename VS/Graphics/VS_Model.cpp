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
#include "VS_EulerAngles.h"

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
		else if ( srLabel == "rotation" )
		{
			SetOrientation( sr->Quaternion() );
		}
		else if ( srLabel == "rotationXYZDegrees" )
		{
			// for convenience, allow the rotation to be set as euler angles,
			// using degrees.
			vsVector3D xyz = sr->Vector3D();
			SetOrientation( vsQuaternion( vsEulerAngles(
							DEGREES(xyz.x),
							DEGREES(xyz.y),
							DEGREES(xyz.z)
							) ) );
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
	m_displayList(list),
	m_instanceData(NULL)
{
}

vsModel::~vsModel()
{
	m_fragment.Clear();

	if ( m_displayList )
		vsDelete(m_displayList);
	vsDelete( m_material );
	vsDelete( m_instanceData );
}

void
vsModel::SetAsInstanceModel()
{
	if ( !m_instanceData )
	{
		m_instanceData = new InstanceData;
	}
}

vsModelInstance *
vsModel::MakeInstance()
{
	SetAsInstanceModel();
	vsModelInstance *inst = new vsModelInstance;
	inst->model = this;
	inst->visible = false;
	inst->index = m_instanceData->instance.ItemCount();
	inst->matrixIndex = -1;
	m_instanceData->instance.AddItem(inst);
	return inst;
}

void
vsModel::UpdateInstance( vsModelInstance *inst, bool show )
{
	if ( show )
	{
		if ( inst->matrixIndex < 0 ) // we've come into view!
		{
			inst->matrixIndex = m_instanceData->matrix.ItemCount();
			m_instanceData->matrix.AddItem( inst->matrix );
			m_instanceData->matrixInstanceId.AddItem( inst->index );
		}
		else // we were already in view;  just update our matrix
		{
			m_instanceData->matrix[inst->matrixIndex] = inst->matrix;
		}
	}
	else if ( !show && inst->matrixIndex >= 0 ) // we've gone out of view!
	{
		int swapFrom = m_instanceData->matrix.ItemCount() - 1;
		int swapTo = inst->matrixIndex;

		int swapperInstanceId = m_instanceData->matrixInstanceId[swapFrom];
		vsModelInstance *swapper = m_instanceData->instance[swapperInstanceId];

		m_instanceData->matrix[swapTo] = m_instanceData->matrix[swapFrom];
		m_instanceData->matrixInstanceId[swapTo] = m_instanceData->matrixInstanceId[swapFrom];
		m_instanceData->matrix.PopBack();
		m_instanceData->matrixInstanceId.PopBack();
		swapper->matrixIndex = swapTo;
		inst->matrixIndex = -1;
	}

	if ( !m_instanceData->matrix.IsEmpty() )
		m_instanceData->bufferIsDirty = true;
}

void
vsModel::RemoveInstance( vsModelInstance *inst )
{
	// FIRST, update this instance to not be visible.  That gets it out of our
	// matrix and matrixInstanceId arrays, if it had been visible.
	UpdateInstance( inst, false );

	// NOW, I want to swap this instance into last position in the instance
	// array.

	// Check whether I'm already in last position.
	int lastIndex = m_instanceData->instance.ItemCount()-1;
	if ( inst->index != lastIndex )
	{
		// Not in last position, so swap that last item into my current spot.
		vsModelInstance *last = m_instanceData->instance[ lastIndex ];

		// To do this swap, I potentially need to update two references to the
		// 'last' item being swapped:  the instance array, and (if the 'last'
		// instance is visible), the modelInstanceId array which tells where
		// it can be found in the instance array.
		m_instanceData->instance[ inst->index ] = last;
		last->index = inst->index;

		if ( last->matrixIndex >= 0 )
		{
			m_instanceData->matrixInstanceId[ last->matrixIndex ] = inst->index;
		}
	}

	// Finally, drop that last position.
	m_instanceData->instance.PopBack();
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
		if ( m_instanceData )
		{
			if ( m_instanceData->matrix.IsEmpty() )
				return;

			if ( m_instanceData->bufferIsDirty )
			{
				m_instanceData->matrixBuffer.SetArray( &m_instanceData->matrix[0], m_instanceData->matrix.ItemCount() );
				m_instanceData->bufferIsDirty = false;
			}
			for( vsListStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
			{
				queue->AddFragmentInstanceBatch( *iter, &m_instanceData->matrix[0], m_instanceData->matrix.ItemCount() );
			}
		}
		else
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
}

vsModelInstance::~vsModelInstance()
{
	model->RemoveInstance(this);
}

void
vsModelInstance::SetVisible( bool v )
{
	if ( visible != v )
	{
		visible = v;
		model->UpdateInstance( this, visible );
	}
}

void
vsModelInstance::SetMatrix( const vsMatrix4x4& mat )
{
	if ( matrix != mat )
	{
		matrix = mat;
		if ( visible )
		{
			model->UpdateInstance( this, visible );
		}
	}
}

