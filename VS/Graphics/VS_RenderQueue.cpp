/*
 *  VS_RenderQueue.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 5/09/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#include "VS_RenderQueue.h"

#include "VS_Camera.h"
#include "VS_Scene.h"
#include "VS_System.h"

#include "VS_MaterialInternal.h"


vsRenderQueueLayer::MaterialBatch::MaterialBatch():
	material("White"),
	vertexBufferBatchList(NULL),
	next(NULL)
{
}

vsRenderQueueLayer::MaterialBatch::~MaterialBatch()
{
}

vsRenderQueueLayer::VertexBufferBatch::VertexBufferBatch():
	vertexBuffer(NULL),
	elementList(NULL),
	next(NULL)
{
}

vsRenderQueueLayer::VertexBufferBatch::~VertexBufferBatch()
{
}

vsRenderQueueLayer::vsRenderQueueLayer():
	m_batch(NULL),
	m_batchCount(0),
	m_materialBatchPool(NULL),
	m_vertexBufferBatchPool(NULL),
	m_batchElementPool(NULL)
{
}

vsRenderQueueLayer::~vsRenderQueueLayer()
{
	while( m_materialBatchPool )
	{
		MaterialBatch *b = m_materialBatchPool;
		m_materialBatchPool = m_materialBatchPool->next;
		vsDelete(b);
	}
	while( m_vertexBufferBatchPool )
	{
		VertexBufferBatch *b = m_vertexBufferBatchPool;
		m_vertexBufferBatchPool = m_vertexBufferBatchPool->next;
		vsDelete(b);
	}
	while( m_batchElementPool )
	{
		BatchElement *e = m_batchElementPool;
		m_batchElementPool = m_batchElementPool->next;
		vsDelete(e);
	}
}

vsRenderQueueLayer::VertexBufferBatch *
vsRenderQueueLayer::FindBatch( vsMaterial *material, vsRenderBuffer *vertexBuffer )
{
	VertexBufferBatch *vbatch = NULL;
	MaterialBatch *mbatch = NULL;

	// TODO:  Hash map would be faster.
	for (MaterialBatch *i = m_batch; i; i = i->next)
	{
		if ( i->material == material )
		{
			mbatch = i;
			break;
		}
	}
	// if we didn't find a material batch, make one.
	if ( !mbatch )
	{
		if ( !m_materialBatchPool )
		{
			m_materialBatchPool = new MaterialBatch;
		}
		MaterialBatch *batch = m_materialBatchPool;
		m_materialBatchPool = batch->next;
		batch->next = NULL;
		batch->material = *material;
		mbatch = batch;
		// insert this batch into our batch list, SORTED.
		bool inserted = false;

		if ( m_batch == NULL )
		{
			m_batch = batch;
		}
		else
		{
			MaterialBatch *lb;
			for(lb = m_batch; lb->next; lb = lb->next)
			{
				if ( lb->next->material.GetResource()->m_layer > material->GetResource()->m_layer )
				{
					batch->next = lb->next;
					lb->next = batch;
					inserted = true;
					break;
				}
			}

			if ( !inserted )
			{
				lb->next = batch;
			}
		}
		m_batchCount++;
	}

	// now I Have a MaterialBatch.  I need to find a VertexBufferBatch.
	for (VertexBufferBatch *i = mbatch->vertexBufferBatchList; i; i = i->next)
	{
		if ( i->vertexBuffer == vertexBuffer )
		{
			vbatch = i;
			break;
		}
	}

	if ( !vbatch )
	{
		if ( !m_vertexBufferBatchPool )
		{
			m_vertexBufferBatchPool = new VertexBufferBatch;
		}
		vbatch = m_vertexBufferBatchPool;
		m_vertexBufferBatchPool = vbatch->next;
		vbatch->next = NULL;
		vbatch->vertexBuffer = vertexBuffer;

		// insert this vbatch into our material batch's list.
		vbatch->next = mbatch->vertexBufferBatchList;
		mbatch->vertexBufferBatchList = vbatch;
	}

	return vbatch;
}


void
vsRenderQueueLayer::AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer *vertexBuffer, vsDisplayList *batchList )
{
	VertexBufferBatch *batch = FindBatch(material, vertexBuffer);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
		m_batchElementPool->next = NULL;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;

	element->matrix = matrix;
	element->list = batchList;

	element->next = batch->elementList;
	batch->elementList = element;
}

vsDisplayList *
vsRenderQueueLayer::MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size )
{
	VertexBufferBatch *batch = FindBatch(material, NULL);

	if ( m_batchElementPool == NULL )
	{
		m_batchElementPool = new BatchElement;
		m_batchElementPool->next = NULL;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = m_batchElementPool->next;
	element->next = NULL;

	element->matrix = matrix;
	element->list = new vsDisplayList(size);

	element->next = batch->elementList;
	batch->elementList = element;
	m_temporaryLists.AddItem(element->list);

	return element->list;
}

void
vsRenderQueueLayer::StartRender()
{
	m_batchCount = 0;
	vsAssert( m_batch == NULL, "Batches not cleared?" );
	//	m_batch = NULL;


}

void
vsRenderQueueLayer::Draw( vsDisplayList *list )
{
	for (MaterialBatch *b = m_batch; b; b = b->next)
	{
		list->SetMaterial( &b->material );

		for (VertexBufferBatch *vb = b->vertexBufferBatchList; vb; vb = vb->next)
		{
			if ( vb->vertexBuffer )
			{
				list->BindBuffer( vb->vertexBuffer );
			}
			for (BatchElement *e = vb->elementList; e; e = e->next)
			{
				bool hasTransform = true;//e->matrix != vsMatrix4x4::Identity;

				if ( hasTransform )
				{
					list->SetMatrix4x4( e->matrix );
				}

				list->Append( *e->list );

				if ( hasTransform )
				{
					list->PopTransform();
				}
			}
			if ( vb->vertexBuffer )
			{
				list->ClearArrays();
			}
		}
	}
}

void
vsRenderQueueLayer::EndRender()
{
	// strategy:  Find the last element of each list, and tack the pools onto
	// the end.  TODO:  Consider whether this would be faster (and simpler!) if
	// we found the end of each pool and tacked the lists on there.
	//
	// Answer:  No.  We have to iterate across all elements in our list anyway,
	// freeing their contents.  So iterating across the pools wouldn't be an
	// improvement.

	m_batchCount = 0;
	MaterialBatch *last = NULL;
	VertexBufferBatch *vlast = NULL;
	for (MaterialBatch *b = m_batch; b; b = b->next)
	{
		last = b;

		for (VertexBufferBatch *vb = b->vertexBufferBatchList; vb; vb = vb->next)
		{
			vlast = vb;

			BatchElement *lastElement = vb->elementList;
			while ( lastElement && lastElement->next )
			{
				lastElement = lastElement->next;
			}
			lastElement->next = m_batchElementPool;
			m_batchElementPool = vb->elementList;
			vb->elementList = NULL;
			vb->vertexBuffer = NULL;
		}
		if ( vlast )
		{
			vlast->next = m_vertexBufferBatchPool;
			m_vertexBufferBatchPool = b->vertexBufferBatchList;
			b->vertexBufferBatchList = NULL;
		}
	}
	if ( last )
	{
		last->next = m_materialBatchPool;
		m_materialBatchPool = m_batch;
	}
	m_batch = NULL;

	m_temporaryLists.Clear();
}

vsRenderQueue::vsRenderQueue( int layerCount, int genericListSize):
	m_parent(NULL)
{
	m_transformStackLevel = 0;

	m_layerCount = layerCount;
	m_layer = new vsRenderQueueLayer[layerCount];

	m_genericList = new vsDisplayList(genericListSize);
}

vsRenderQueue::~vsRenderQueue()
{
	vsDeleteArray( m_layer );
	vsDelete( m_genericList );
}

void
vsRenderQueue::StartRender(vsScene *parent)
{
	m_parent = parent;
	InitialiseTransformStack();

	for ( int i = 0; i < m_layerCount; i++ )
	{
		m_layer[i].StartRender();
	}
	m_genericList->Clear();

}

void
vsRenderQueue::Draw( vsDisplayList *list )
{
	for ( int i = 0; i < m_layerCount; i++ )
	{
		m_layer[i].Draw(list);
	}
	list->Append(*m_genericList);

	DeinitialiseTransformStack();
	vsAssert( m_transformStackLevel == 0, "Unbalanced push/pop of transforms?");
}

void
vsRenderQueue::EndRender()
{
	for ( int i = 0; i < m_layerCount; i++ )
	{
		m_layer[i].EndRender();
	}
	m_genericList->Clear();
}

void
vsRenderQueue::InitialiseTransformStack()
{
	if ( m_parent->Is3D() )
	{
		vsTransform3D startingTransform;
		switch( vsSystem::Instance()->GetOrientation() )
		{
			case Orientation_Normal:
				break;
			case Orientation_Six:
				startingTransform.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(180.f) ) );
				break;
			case Orientation_Three:
				startingTransform.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(270.f) ) );
				break;
			case Orientation_Nine:
				startingTransform.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(90.f) ) );
				break;
		}
		//
//		startingTransform.SetTranslation( vsVector3D(0.f, 0.0f, 0.f) );
		vsMatrix4x4 startingMatrix = startingTransform.GetMatrix();

		vsMatrix4x4 requestedMatrix = vsMatrix4x4::Identity;

		requestedMatrix.w -= m_parent->GetCamera3D()->GetPosition();

		vsMatrix4x4 myIdentity;
		myIdentity.x *= -1.f;

		vsMatrix4x4 cameraMatrix = m_parent->GetCamera3D()->GetTransform().GetMatrix();

		vsVector3D forward = cameraMatrix.z;
		vsVector3D up = cameraMatrix.y;
		vsVector3D side = forward.Cross(up);

		cameraMatrix.x = side;
		cameraMatrix.y = up;
		cameraMatrix.z = -forward;
		cameraMatrix.w.Set(0.f,0.f,0.f,1.f);
		cameraMatrix.Invert();

		cameraMatrix = startingMatrix * myIdentity * cameraMatrix;

		m_transformStack[0] = cameraMatrix * requestedMatrix;
		m_transformStackLevel = 1;
	}
	else
	{
		vsTransform3D startingTransform;
		switch( vsSystem::Instance()->GetOrientation() )
		{
			case Orientation_Normal:
				break;
			case Orientation_Six:
				startingTransform.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(180.f) ) );
				break;
			case Orientation_Three:
				startingTransform.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(90.f) ) );
				break;
			case Orientation_Nine:
				startingTransform.SetRotation ( vsQuaternion( vsVector3D::ZAxis, DEGREES(270.f) ) );
				break;
		}
		//
		//		startingTransform.SetTranslation( vsVector3D(0.f, 0.0f, 0.f) );
		vsMatrix4x4 startingMatrix = startingTransform.GetMatrix();

		m_transformStack[0] = startingMatrix;
		m_transformStackLevel = 1;
	}
}

void
vsRenderQueue::DeinitialiseTransformStack()
{
	if ( m_parent->Is3D() )
	{
		m_transformStackLevel--;
	}
	else
	{
		m_transformStackLevel--;
	}
}

void
vsRenderQueue::AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer *vertexBuffer, vsDisplayList *batch )
{
	int layerId = PickLayerForMaterial( material );

	m_layer[layerId].AddBatch( material, matrix, vertexBuffer, batch );
}

vsDisplayList *
vsRenderQueue::MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size )
{
	int layerId = PickLayerForMaterial( material );

	return m_layer[layerId].MakeTemporaryBatchList( material, matrix, size );
}

vsDisplayList *
vsRenderQueue::MakeTemporaryBatchList( vsMaterial *material, int size )
{
	int layerId = PickLayerForMaterial( material );

	return m_layer[layerId].MakeTemporaryBatchList( material, vsMatrix4x4::Identity, size );
}

vsRenderQueueLayer *
vsRenderQueue::GetLayer( int i )
{
	vsAssert( i >= 0 && i < m_layerCount, "Requested nonexistant render layer!" );

	if ( i >= 0 && i < m_layerCount )
	{
		return &m_layer[i];
	}

	return NULL;
}


vsMatrix4x4
vsRenderQueue::PushMatrix( const vsMatrix4x4 &matrix )
{
	vsAssert( m_transformStackLevel < MAX_STACK_DEPTH, "Transform stack overflow!" )
	vsAssert( m_transformStackLevel > 0, "Uninitialised transform stack??" )

	if ( m_transformStackLevel < MAX_SCENE_STACK )
	{
		m_transformStack[m_transformStackLevel] = m_transformStack[m_transformStackLevel-1] * matrix;
		m_transformStackLevel++;
	}

	return m_transformStack[ m_transformStackLevel-1 ];
}

vsMatrix4x4
vsRenderQueue::PushTransform2D( const vsTransform2D &transform )
{
	vsAssert( m_transformStackLevel < MAX_STACK_DEPTH, "Transform stack overflow!" )
	vsAssert( m_transformStackLevel > 0, "Uninitialised transform stack??" )

	if ( m_transformStackLevel < MAX_SCENE_STACK )
	{
        vsMatrix4x4 matrix = transform.GetMatrix();
        matrix.SetTranslation( -1.f * matrix.w );
		m_transformStack[m_transformStackLevel] = m_transformStack[m_transformStackLevel-1] * matrix;
		m_transformStackLevel++;
	}

	return m_transformStack[ m_transformStackLevel-1 ];
}

vsMatrix4x4
vsRenderQueue::PushTranslation( const vsVector3D &t )
{
	vsMatrix4x4 mat;
	mat.SetTranslation(t);

	return PushMatrix( mat );
}

void
vsRenderQueue::PopMatrix()
{
	vsAssert( m_transformStackLevel > 0, "Transform stack underflow!" );
	m_transformStackLevel--;
}

vsMatrix4x4
vsRenderQueue::GetMatrix()
{
	vsAssert( m_transformStackLevel > 0, "Nothing in the transform stack!" );

		return m_transformStack[m_transformStackLevel-1];
}

int
vsRenderQueue::PickLayerForMaterial( vsMaterial *material )
{
	if ( material->GetResource()->m_glow )
	{
		return 1;
	}
	return 0;
}



