/*
 *  VS_ModelInstanceGroup.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 21/03/2016
 *  Copyright 2016 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_ModelInstanceGroup.h"
#include "VS_ModelInstance.h"
#include "VS_Model.h"

vsModelInstance *
vsModelInstanceGroup::MakeInstance()
{
	vsModelInstance *inst = new vsModelInstance;
	AddInstance(inst);
	return inst;
}

void
vsModelInstanceGroup::TakeInstancesFromGroup( vsModelInstanceGroup *otherGroup )
{
	while( !otherGroup->m_instance.IsEmpty() )
	{
		vsModelInstance *instance = otherGroup->m_instance[0];
		bool visible = instance->visible;
		otherGroup->RemoveInstance(instance);
		AddInstance(instance);
		instance->SetVisible(visible);
		// UpdateInstance(instance, visible);
	}
}

void
vsModelInstanceGroup::UpdateInstance( vsModelInstance *inst, bool show )
{
	vsAssert(inst->group == this, "Wrong group??");

	if ( show )
	{
		if ( inst->matrixIndex < 0 ) // we've come into view!
		{
			inst->matrixIndex = m_matrix.ItemCount();
			m_matrix.AddItem( inst->matrix );
			m_color.AddItem( inst->color );
			m_matrixInstanceId.AddItem( inst->index );
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
			m_bufferIsDirty = true;
#endif
		}
		else // we were already in view;  just update our matrix
		{
			m_matrix[inst->matrixIndex] = inst->matrix;
			m_color[inst->matrixIndex] = inst->color;
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
			m_bufferIsDirty = true;
#endif
		}
	}
	else if ( !show && inst->matrixIndex >= 0 ) // we've gone out of view!
	{
		int swapFrom = m_matrix.ItemCount() - 1;
		int swapTo = inst->matrixIndex;

		// We don't need to swap if we were already the last thing in the list.
		if ( swapFrom != swapTo )
		{
			int swapperInstanceId = m_matrixInstanceId[swapFrom];
			vsModelInstance *swapper = m_instance[swapperInstanceId];

			m_matrix[swapTo] = m_matrix[swapFrom];
			m_color[swapTo] = m_color[swapFrom];
			m_matrixInstanceId[swapTo] = m_matrixInstanceId[swapFrom];
			swapper->matrixIndex = swapTo;
		}
		m_matrix.PopBack();
		m_color.PopBack();
		m_matrixInstanceId.PopBack();
		inst->matrixIndex = -1;
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
		m_bufferIsDirty = true;
#endif
	}
}

void
vsModelInstanceGroup::AddInstance( vsModelInstance *inst )
{
	bool wasVisible = inst->visible;
	inst->group = this;
	inst->visible = false;
	inst->index = m_instance.ItemCount();
	inst->matrixIndex = -1;
	m_instance.AddItem(inst);

	if ( wasVisible )
	{
		inst->SetVisible(true);
	}
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
	m_bufferIsDirty = true;
#endif
}

bool
vsModelInstanceGroup::ContainsInstance( vsModelInstance *instance )
{
	return m_instance.Contains(instance);
}


void
vsModelInstanceGroup::RemoveInstance( vsModelInstance *inst )
{
	// FIRST, update this instance to not be visible.  That gets it out of our
	// matrix and matrixInstanceId arrays, if it had been visible.
	UpdateInstance( inst, false );

	// NOW, I want to swap this instance into last position in the instance
	// array.

	// Check whether I'm already in last position.
	int lastIndex = m_instance.ItemCount()-1;
	if ( inst->index != lastIndex )
	{
		// Not in last position, so swap that last item into my current spot.
		vsModelInstance *last = m_instance[ lastIndex ];

		// To do this swap, I potentially need to update two references to the
		// 'last' item being swapped:  the instance array, and (if the 'last'
		// instance is visible), the modelInstanceId array which tells where
		// it can be found in the instance array.
		m_instance[ inst->index ] = last;
		last->index = inst->index;

		if ( last->matrixIndex >= 0 )
		{
			m_matrixInstanceId[ last->matrixIndex ] = inst->index;
		}
	}

	// Finally, drop that last instance.
	m_instance.PopBack();
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
	m_bufferIsDirty = true;
#endif
}

void
vsModelInstanceGroup::Draw( vsRenderQueue *queue )
{
	if ( m_matrix.IsEmpty() )
		return;

#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
	if ( m_bufferIsDirty )
	{
		vsAssert(m_matrix.ItemCount() == m_color.ItemCount(), "Non-equal instance buffers??");
		m_matrixBuffer.SetArray(&m_matrix[0], m_matrix.ItemCount() );
		m_colorBuffer.SetArray(&m_color[0], m_color.ItemCount() );
		m_bufferIsDirty = false;
	}
	m_model->DrawInstanced( queue, &m_matrixBuffer, &m_colorBuffer, m_values );
#else
	m_model->DrawInstanced( queue, &m_matrix[0], &m_color[0], m_matrix.ItemCount(), m_values );
#endif // INSTANCED_MODEL_USES_LOCAL_BUFFER

}

void
vsModelInstanceGroup::CalculateMatrixBounds( vsBox3D& out )
{
	out.Clear();
	for ( int i = 0; i < m_matrix.ItemCount(); i++ )
		out.ExpandToInclude( m_matrix[i].w );
}

