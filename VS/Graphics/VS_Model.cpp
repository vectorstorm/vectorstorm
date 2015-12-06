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
#include "VS_Serialiser.h"
#include "VS_Store.h"


vsModel *
vsModel::Load( const vsString &filename_in )
{
	vsString filename = filename_in;
	// Check extension.
	size_t dot = filename.rfind('.');
	if ( dot != vsString::npos && dot == filename.size() - 4 )
	{
		// three-character extension
		vsString extension = filename.substr(dot+1,-1);
		if ( extension == "vmd" || extension == "vmb" )
			filename.erase(dot,-1);
	}

	vsString binaryFilename = filename + ".vmb";
	vsString textFilename = filename + ".vmd";
	if ( vsFile::Exists(binaryFilename) )
		return LoadBinary( binaryFilename );
	else
		return LoadText( textFilename );
}

vsFragment*
vsModel::LoadFragment_Internal( vsSerialiserRead& r )
{
	vsFragment *result = NULL;

	vsString tag;
	r.String(tag);
	if ( tag == "Fragment" )
	{
		result = new vsFragment;
		vsString materialName, format;
		r.String(materialName);
		r.String(format);
		result->SetMaterial(materialName);
		int32_t vertexCount;
		r.Int32(vertexCount);
		vsRenderBuffer *vbo = new vsRenderBuffer(vsRenderBuffer::Type_Static);
		vsRenderBuffer *ibo = new vsRenderBuffer(vsRenderBuffer::Type_Static);

		if ( format == "PCNT" )
		{
			vsRenderBuffer::PCNT *buffer = new vsRenderBuffer::PCNT[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				r.Color(buffer[i].color);
				r.Vector3D(buffer[i].normal);
				r.Vector2D(buffer[i].texel);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PCN" )
		{
			vsRenderBuffer::PCN *buffer = new vsRenderBuffer::PCN[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				r.Color(buffer[i].color);
				r.Vector3D(buffer[i].normal);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PCT" )
		{
			vsRenderBuffer::PCT *buffer = new vsRenderBuffer::PCT[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				r.Color(buffer[i].color);
				r.Vector2D(buffer[i].texel);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PC" )
		{
			vsRenderBuffer::PC *buffer = new vsRenderBuffer::PC[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				r.Color(buffer[i].color);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PN" )
		{
			vsRenderBuffer::PN *buffer = new vsRenderBuffer::PN[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				r.Vector3D(buffer[i].normal);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "PT" )
		{
			vsRenderBuffer::PT *buffer = new vsRenderBuffer::PT[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
				r.Vector2D(buffer[i].texel);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else if ( format == "P" )
		{
			vsRenderBuffer::P *buffer = new vsRenderBuffer::P[ vertexCount ];
			for ( int32_t i = 0; i < vertexCount; i++ )
			{
				r.Vector3D(buffer[i].position);
			}
			vbo->SetArray(buffer, vertexCount);
			vsDeleteArray(buffer);
		}
		else
		{
			vsAssert(0, vsFormatString("Unsupported vertex format: %s", format.c_str()) );
		}

		r.String(tag);

		vsAssert( tag == "IndexBuffer", "Not matching up??" );
		int32_t indexCount;
		r.Int32(indexCount);
		uint16_t *indices = new uint16_t[ indexCount ];
		for ( int i = 0; i < indexCount; i++ )
		{
			int32_t ind;
			r.Int32(ind);
			indices[i] = ind;
		}
		ibo->SetArray(indices, indexCount);
		vsDeleteArray(indices);

		result->AddBuffer(vbo);
		result->AddBuffer(ibo);

		vsDisplayList *list = new vsDisplayList(128);
		list->BindBuffer(vbo);
		list->TriangleListBuffer(ibo);
		list->ClearArrays();

		result->SetDisplayList(list);
	}

	return result;
}

vsModel*
vsModel::LoadModel_Internal( vsSerialiserRead& r )
{
	vsModel *result = NULL;
	vsString tag;
	r.String(tag);
	if ( tag == "ModelV1" )
	{
		result = new vsModel;
		r.String(result->m_name);

		vsVector3D trans, scale;
		vsVector4D rot;
		r.Vector3D(trans);
		r.Vector4D(rot);
		r.Vector3D(scale);

		result->SetPosition(trans);
		result->SetScale(scale);
		result->SetOrientation( vsQuaternion( rot.x, rot.y, rot.z, rot.w ) );

		int32_t meshCount;
		r.Int32(meshCount);

		for ( int i = 0; i < meshCount; i++ )
		{
			vsFragment *f = LoadFragment_Internal(r);
			result->AddFragment(f);
		}

		int32_t childCount;
		r.Int32(childCount);

		for ( int32_t i = 0; i < childCount; i++ )
		{
			vsModel *child = LoadModel_Internal(r);
			result->AddChild(child);
		}
	}

	return result;
}

vsModel *
vsModel::LoadBinary( const vsString &filename )
{
	vsModel *result = NULL;

	vsFile file(filename);
	vsStore store(file.GetLength());
	file.Store(&store);
	vsSerialiserRead r(&store);

	result = LoadModel_Internal(r);


	return result;
}

vsModel *
vsModel::LoadText( const vsString &filename )
{
	vsFile file(filename);
	vsRecord r;

	while( file.Record(&r) )
	{
		if ( r.GetLabel().AsString() == "Model" )
		{
			vsModel *result = new vsModel;
			result->LoadFrom(&r);
			return result;
		}
	}

	return NULL;
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
		else if ( srLabel == "matrix" )
		{
			vsAssert( sr->GetChildCount() == 4, "Wrong number of matrix lines" );
			vsMatrix4x4 mat;
			for ( int i = 0; i < 4; i++ )
			{
				mat[i] = sr->GetChild(i)->Vector4D();
			}
			SetPosition(mat.w);
			vsQuaternion q(mat.z, mat.y);
			SetOrientation(q);
			// SetScale(mat.GetScale());
		}
		else if ( srLabel == "translation" )
		{
			SetPosition( sr->Vector3D() );
		}
		else if ( srLabel == "rotation" )
		{
			SetOrientation( sr->Quaternion() );
		}
		else if ( srLabel == "scale" )
		{
			SetScale( sr->Vector3D() );
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
	m_instanceGroup(NULL)
{
}

vsModel::~vsModel()
{
	m_fragment.Clear();

	if ( m_displayList )
		vsDelete(m_displayList);
	vsDelete( m_material );
	vsDelete( m_instanceGroup );
}

void
vsModel::SetAsInstanceModel()
{
	if ( !m_instanceGroup )
	{
		m_instanceGroup = new vsModelInstanceGroup(this);
	}
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
	if ( fragment )
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
	for ( vsArrayStore<vsFragment>::Iterator iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
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
vsModel::DrawInstanced( vsRenderQueue *queue, const vsMatrix4x4* matrices, const vsColor* colors, int instanceCount, vsShaderValues *shaderValues )
{
	for( vsArrayStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
	{
		queue->AddFragmentInstanceBatch( *iter, matrices, colors, instanceCount, shaderValues );
	}
}

void
vsModel::DrawInstanced( vsRenderQueue *queue, vsRenderBuffer* matrixBuffer, vsRenderBuffer* colorBuffer, vsShaderValues *shaderValues )
{
	for( vsArrayStoreIterator<vsFragment> iter = m_fragment.Begin(); iter != m_fragment.End(); iter++ )
	{
		queue->AddFragmentInstanceBatch( *iter, matrixBuffer, colorBuffer, shaderValues );
	}
}

void
vsModel::Draw( vsRenderQueue *queue )
{
	if ( GetVisible() )
	{
		if ( m_instanceGroup )
		{
			m_instanceGroup->Draw( queue );
		}
		else
		{
			bool hasTransform = (m_transform != vsTransform3D::Identity);

			if ( hasTransform )
			{
				queue->PushMatrix( m_transform.GetMatrix() );
			}

			if ( m_displayList )
			{
				// old rendering support
				vsDisplayList *list = queue->GetGenericList();
				if ( m_material )
				{
					list->SetMaterial( m_material );
				}
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

			if ( hasTransform )
			{
				queue->PopMatrix();
			}
		}
	}
}

vsModelInstance::~vsModelInstance()
{
	group->RemoveInstance(this);
}

void
vsModelInstance::SetVisible( bool v )
{
	if ( visible != v )
	{
		visible = v;
		group->UpdateInstance( this, visible );
	}
}

void
vsModelInstance::SetMatrix( const vsMatrix4x4& mat )
{
	SetMatrix( mat, c_white );
}

void
vsModelInstance::SetMatrix( const vsMatrix4x4& mat, const vsColor &c )
{
	if ( matrix != mat || color != c)
	{
		matrix = mat;
		color = c;
		if ( visible )
		{
			group->UpdateInstance( this, visible );
		}
	}
}

vsModel *
vsModelInstance::GetModel()
{
	return GetInstanceGroup()->GetModel();
}

vsModelInstance *
vsModel::MakeInstance()
{
	SetAsInstanceModel();
	return m_instanceGroup->MakeInstance();
}

void
vsModel::UpdateInstance( vsModelInstance *inst, bool show )
{
	m_instanceGroup->UpdateInstance( inst, show );
}

void
vsModel::RemoveInstance( vsModelInstance *inst )
{
	m_instanceGroup->RemoveInstance( inst );
}

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
	inst->group = this;
	inst->visible = false;
	inst->index = m_instance.ItemCount();
	inst->matrixIndex = -1;
	m_instance.AddItem(inst);
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

