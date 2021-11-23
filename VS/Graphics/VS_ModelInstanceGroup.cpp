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

vsModelInstanceLodGroup::vsModelInstanceLodGroup( vsModelInstanceGroup *group, vsModel *model, size_t lodLevel ):
	m_group(group),
	m_model(model),
	m_lodLevel(lodLevel),
	m_values(nullptr),
	m_options(nullptr)
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
	,
	m_matrixBuffer(vsRenderBuffer::Type_Dynamic),
	m_colorBuffer(vsRenderBuffer::Type_Dynamic),
	m_bufferIsDirty(false)
#endif // INSTANCED_MODEL_USES_LOCAL_BUFFER
{
}

vsModelInstanceLodGroup::~vsModelInstanceLodGroup()
{
	for ( int i = 0; i < m_instance.ItemCount(); i++ )
		RemoveInstance(m_instance[i]);
}

vsModelInstance *
vsModelInstanceLodGroup::MakeInstance()
{
	vsModelInstance *inst = new vsModelInstance;
	AddInstance(inst);
	return inst;
}

void
vsModelInstanceLodGroup::TakeInstancesFromGroup( vsModelInstanceLodGroup *otherGroup )
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
vsModelInstanceLodGroup::UpdateInstance( vsModelInstance *inst, bool show )
{
	vsScopedLock lock(m_mutex);

	vsAssert(inst->lodGroup == this, "Wrong group??");

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
vsModelInstanceLodGroup::AddInstance( vsModelInstance *inst )
{
	vsScopedLock lock(m_mutex);

	bool wasVisible = inst->visible;
	inst->group = m_group;
	inst->lodGroup = this;
	inst->visible = false;
	inst->lodLevel = m_lodLevel;
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
vsModelInstanceLodGroup::ContainsInstance( vsModelInstance *instance )
{
	vsScopedLock lock(m_mutex);
	return m_instance.Contains(instance);
}

bool
vsModelInstanceLodGroup::IsEmpty()
{
	vsScopedLock lock(m_mutex);
	return m_instance.IsEmpty();
}

void
vsModelInstanceLodGroup::RemoveInstance( vsModelInstance *inst )
{
	vsScopedLock lock(m_mutex);
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

	inst->group = nullptr;
	inst->lodGroup = nullptr;
}

void
vsModelInstanceLodGroup::Draw( vsRenderQueue *queue )
{
	vsScopedLock lock(m_mutex);

	if ( m_matrix.IsEmpty() )
		return;

	// int preLodLevel = m_model->GetLodLevel();
	// m_model->SetLodLevel( m_lodLevel );
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
	if ( m_bufferIsDirty )
	{
		vsAssert(m_matrix.ItemCount() == m_color.ItemCount(), "Non-equal instance buffers??");
		m_matrixBuffer.SetArray(&m_matrix[0], m_matrix.ItemCount() );
		m_colorBuffer.SetArray(&m_color[0], m_color.ItemCount() );
		m_bufferIsDirty = false;
	}
	m_model->DrawInstanced( queue, &m_matrixBuffer, &m_colorBuffer, m_values, m_options, m_lodLevel );
#else
	m_model->DrawInstanced( queue, &m_matrix[0], &m_color[0], m_matrix.ItemCount(), m_values, m_options, m_lodLevel );
#endif // INSTANCED_MODEL_USES_LOCAL_BUFFER

	// m_model->SetLodLevel( preLodLevel );
}

void
vsModelInstanceLodGroup::CalculateMatrixBounds( vsBox3D& out )
{
	out.Clear();
	for ( int i = 0; i < m_matrix.ItemCount(); i++ )
		out.ExpandToInclude( m_matrix[i].w );
}

void
vsModelInstanceLodGroup::CalculateBounds( vsBox3D& out )
{
	vsBox3D box = GetModel()->GetBoundingBox();
	out.Clear();
	for ( int i = 0; i < m_instance.ItemCount(); i++ )
	{
		for ( int c = 0; c < 8; c++ )
		{
			vsVector3D pos = m_instance[i]->matrix.ApplyTo( box.Corner(c) );
			out.ExpandToInclude( pos );
		}
	}
	// for ( int i = 0; i < m_matrix.ItemCount(); i++ )
	// {
	// 	for ( int c = 0; c < 8; c++ )
	// 	{
	// 		vsVector3D pos = m_matrix[i].ApplyTo( box.Corner(c) );
	// 		out.ExpandToInclude( pos );
	// 	}
	// }
}

vsModelInstanceGroup::vsModelInstanceGroup( vsModel *model ):
	m_model( model ),
	m_lod( model->GetLodCount() )
{
	for ( int i = 0; i < model->GetLodCount(); i++ )
	{
		m_lod.AddItem( new vsModelInstanceLodGroup(this, model,i) );
	}
}

void
vsModelInstanceGroup::TakeInstancesFromGroup( vsModelInstanceGroup *otherGroup )
{
	for ( int src = 0; src < otherGroup->m_lod.ItemCount(); src++ )
	{
		// try to move into the same lod number, if that's possible.  If not,
		// then move into the highest-numbered lod available.
		int dest = src;
		if ( dest >= m_lod.ItemCount() )
		{
			dest = m_lod.ItemCount() - 1;
		}
		m_lod[dest]->TakeInstancesFromGroup( otherGroup->m_lod[src] );
	}
}

void
vsModelInstanceGroup::SetShaderValues( vsShaderValues *values )
{
	for ( int i = 0; i < m_lod.ItemCount(); i++ )
	{
		m_lod[i]->SetShaderValues(values);
	}
}

void
vsModelInstanceGroup::SetShaderOptions( vsShaderOptions *options )
{
	for ( int i = 0; i < m_lod.ItemCount(); i++ )
	{
		m_lod[i]->SetShaderOptions(options);
	}
}

vsModelInstance *
vsModelInstanceGroup::MakeInstance( int lod )
{
	vsAssert( lod < m_lod.ItemCount(), "Requested invalid lod level?" );
	return m_lod[lod]->MakeInstance();
}

void
vsModelInstanceGroup::AddInstance( vsModelInstance *instance )
{
	int dest = instance->lodLevel;
	if ( dest >= m_lod.ItemCount() )
	{
		dest = m_lod.ItemCount() - 1;
	}
	m_lod[dest]->AddInstance(instance);
	instance->UpdateGroup();
}

void
vsModelInstanceGroup::RemoveInstance( vsModelInstance *instance )
{
	if ( instance->group == this )
		instance->lodGroup->RemoveInstance(instance);
}

bool
vsModelInstanceGroup::ContainsInstance( vsModelInstance *instance )
{
	if ( instance->group == this )
		return true;
	return false;
}

bool
vsModelInstanceGroup::IsEmpty()
{
	bool isEmpty = true;
	for ( int i = 0; i < m_lod.ItemCount(); i++ )
	{
		isEmpty &= m_lod[i]->IsEmpty();
	}
	return isEmpty;
}

void
vsModelInstanceGroup::UpdateInstance( vsModelInstance *instance, bool show )
{
	if ( m_lod[instance->lodLevel] == instance->lodGroup ) // same lod!
		m_lod[instance->lodLevel]->UpdateInstance(instance, show);
	else
	{
		// instance has changed LOD level, so swap it into a new lod instance group.
		instance->lodGroup->RemoveInstance(instance);
		m_lod[instance->lodLevel]->AddInstance(instance);
	}
}

vsModel *
vsModelInstanceGroup::GetModel()
{
	return m_model;
}

// find the bounds of our matrix translations.
void
vsModelInstanceGroup::CalculateMatrixBounds( vsBox3D& out )
{
	vsBox3D a;
	out.Clear();
	for ( int i = 0; i < m_lod.ItemCount(); i++ )
	{
		m_lod[i]->CalculateMatrixBounds(a);
		out.ExpandToInclude(a);
	}
}

void
vsModelInstanceGroup::CalculateBounds( vsBox3D& out )
{
	out.Clear();
	vsBox3D o;

	for ( int i = 0; i < m_lod.ItemCount(); i++ )
	{
		m_lod[i]->CalculateBounds(o);
		out.ExpandToInclude(o);
	}
}

void
vsModelInstanceGroup::Draw( vsRenderQueue *queue )
{
	for ( int i = 0; i < m_lod.ItemCount(); i++ )
	{
		m_lod[i]->Draw(queue);
	}
}

