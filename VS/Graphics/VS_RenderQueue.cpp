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
#include "VS_Fragment.h"
#include "VS_Scene.h"
#include "VS_System.h"

#include "VS_DynamicBatch.h"
#include "VS_DynamicBatchManager.h"

#include "VS_MaterialInternal.h"

#include "VS/VS_DisableDebugNew.h"
#include <map>
#include "VS/VS_EnableDebugNew.h"

class vsRenderQueueStage
{
public:
	struct BatchMap;
	struct BatchElement;
	struct Batch;
private:

	BatchMap*			m_batchMap;
	Batch*				m_batch;
	int					m_batchCount;

	Batch *				m_batchPool;
	BatchElement * 		m_batchElementPool;

	vsLinkedListStore<vsDisplayList>	m_temporaryLists;

	Batch *			FindBatch( vsMaterial *material );


public:

	vsRenderQueueStage();
	~vsRenderQueueStage();

	void			StartRender();
	void			Draw( vsDisplayList *list );	// write our batches into here.
	void			EndRender();

	// Add a batch to this stage
	void			AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch );
	void			AddSimpleBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsDisplayList *batch, vsShaderValues *values = NULL );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batch, vsShaderValues *values = NULL );

	void			AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type, vsShaderValues *values = NULL );
	void			AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type );
	void			AddSimpleInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type, vsShaderValues *values = NULL );

	// For stuff which really doesn't want to keep its display list around, call this to get a temporary display list.
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size );
};


struct vsRenderQueueStage::BatchElement
{
	vsMatrix4x4		matrix;
	vsMaterial *	material;
	vsShaderValues *shaderValues;
	const vsMatrix4x4 *	instanceMatrix;
	const vsColor *	instanceColor;
	int				instanceMatrixCount;
	vsRenderBuffer *instanceMatrixBuffer;
	vsRenderBuffer *instanceColorBuffer;
	vsDisplayList *	list;
	vsRenderBuffer *vbo;
	vsRenderBuffer *ibo;
	vsFragment::SimpleType simpleType;
	BatchElement *	next;

	vsDynamicBatch * batch;

	BatchElement():
		material(NULL),
		shaderValues(NULL),
		instanceMatrix(NULL),
		instanceColor(NULL),
		instanceMatrixCount(0),
		instanceMatrixBuffer(NULL),
		instanceColorBuffer(NULL),
		list(NULL),
		vbo(NULL),
		ibo(NULL),
		next(NULL),
		batch(NULL)
	{
	}

	void Clear()
	{
		material = NULL;
		shaderValues = NULL;
		instanceMatrix = NULL;
		instanceMatrixCount = 0;
		instanceMatrixBuffer = NULL;
		instanceColorBuffer = NULL;
		instanceColor = NULL;
		list = NULL;
		vbo = NULL;
		ibo = NULL;
		batch = NULL;
	}

};

struct vsRenderQueueStage::Batch
{
	vsMaterialInternal*	material;
	BatchElement*		elementList;
	Batch*				next;

	Batch();
	~Batch();
};

struct vsRenderQueueStage::BatchMap
{
	std::map<vsMaterialInternal*,vsRenderQueueStage::Batch*> map;
};

vsRenderQueueStage::Batch::Batch():
	material(NULL),
	elementList(NULL),
	next(NULL)
{
}

vsRenderQueueStage::Batch::~Batch()
{
}

vsRenderQueueStage::vsRenderQueueStage():
	m_batchMap(new BatchMap),
	m_batch(NULL),
	m_batchCount(0),
	m_batchPool(NULL),
	m_batchElementPool(NULL)
{
}

vsRenderQueueStage::~vsRenderQueueStage()
{
	vsDelete( m_batchMap );
	while( m_batchPool )
	{
		Batch *b = m_batchPool;
		m_batchPool = m_batchPool->next;
		vsDelete(b);
	}
	while( m_batchElementPool )
	{
		BatchElement *e = m_batchElementPool;
		m_batchElementPool = m_batchElementPool->next;
		vsDelete(e);
	}
}

vsRenderQueueStage::Batch *
vsRenderQueueStage::FindBatch( vsMaterial *material )
{
	std::map<vsMaterialInternal*, vsRenderQueueStage::Batch*>::iterator it = m_batchMap->map.find(material->GetResource());
	if ( it != m_batchMap->map.end() )
	{
		return it->second;
	}

	if ( !m_batchPool )
	{
		m_batchPool = new Batch;
	}
	Batch *batch = m_batchPool;
	m_batchPool = batch->next;
	batch->next = NULL;
	batch->material = material->GetResource();

	// insert this batch into our batch list, SORTED.
	bool inserted = false;

	if ( m_batch == NULL )
	{
		m_batch = batch;
	}
	else
	{
		if ( m_batch->material->m_layer > material->GetResource()->m_layer )
		{
			// we should sort before the first thing in the batch list.
			batch->next = m_batch;
			m_batch = batch;
			inserted = true;
		}
		else
		{
			Batch *lb;
			for(lb = m_batch; lb->next; lb = lb->next)
			{
				if ( lb->next->material->m_layer > material->GetResource()->m_layer )
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
	}
	m_batchCount++;
	m_batchMap->map[material->GetResource()] = batch;

	return batch;
}


void
vsRenderQueueStage::AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batchList )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->material = material;
	element->matrix = matrix;
	element->list = batchList;
	element->instanceMatrix = NULL;
	element->instanceMatrixBuffer = NULL;
	element->instanceColorBuffer = NULL;

	element->next = batch->elementList;
	batch->elementList = element;
}

void
vsRenderQueueStage::AddSimpleBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType simpleType)
{
	Batch *batch = FindBatch(material);

	if ( vsSystem::Instance()->GetPreferences()->GetDynamicBatching() )
	{
		// Check for compatible simple BatchElements
		// in this batch.  If I find one, we'll merge together.
		//
		// "Compatible" means:  VBO is the same format and simpleType is the same.
		// And no instance data;  batching doesn't work with instanced draws!
		//
		// Actually..  I can ignore 'simpleType' and just convert everything into
		// triangle lists.  That'd get around the issue of primitive restarts in
		// the case of fans and strips.
		//
		// Also, I probably want to have a rule like "if we add the size of their
		// VBO array to the size of our VBO array, the total size should be under
		// <X>".  (Unity has a limit of 900 vertex attributes and 300 vertices..  so..
		// with a three-attribute vertex format like PCT, you can do 300 vertices.
		// But with PCNT, you only get 225.)  I could do something like that, I guess?

		BatchElement *mergeCandidate = NULL;
		if ( vsDynamicBatch::Supports( vbo->GetContentType() ) &&
				vbo->GetPositionCount() < 200 ) // don't even try to merge things that are too big.
		{
			mergeCandidate = batch->elementList;
			while(mergeCandidate)
			{
				// [TODO] I should also be checking whether there's space in the
				// mergeCandidate's buffer to merge with it.
				//
				// Also, we really don't want to merge into a renderbuffer *every*
				// time, because each time it would initiate a transfer to the GPU.
				// Instead, we want to be doing these merges into CPU-side memory
				// and only push into a GPU buffer once we're *done* merging!
				if ( mergeCandidate->instanceMatrix == NULL &&
						mergeCandidate->vbo &&
						mergeCandidate->vbo->GetContentType() == vbo->GetContentType() &&
						mergeCandidate->material->MatchesForBatching( material ) )
				{
					// break;
					if ( mergeCandidate->batch == NULL )
					{
						if ( mergeCandidate->vbo->GetPositionCount() + vbo->GetPositionCount() < 300 )
							break;
					}
					else
					{
						if ( mergeCandidate->batch->CanFitVertices( vbo->GetPositionCount() ) )
							break;
					}
				}
				mergeCandidate = mergeCandidate->next;
			}
		}

		if (mergeCandidate)
		{
			// vsLog("Found merge candidate!");
			// Okay.  So what we're going to do is this:
			//
			// First, we need to understand whether this batch is already a "merge"
			// batch, because if so we can safely add ourself to it.  If NOT, we
			// must create a "merge" batch and add BOTH the merge candidate AND
			// this batch to it, then remove the mergeCandidate.
			//
			// This implies that we need to have some set of "merge" batches around
			// and ready for use.  And we need a way to mark which BatchElements
			// represent these "merge" batches

			if ( mergeCandidate->batch == NULL )
			{
				mergeCandidate->batch = vsDynamicBatchManager::Instance()->GetNewBatch();
				mergeCandidate->batch->StartBatch( mergeCandidate->vbo,
						mergeCandidate->ibo,
						mergeCandidate->matrix,
						mergeCandidate->simpleType );
			}
			mergeCandidate->batch->AddToBatch( vbo, ibo, matrix, simpleType );
			return;
		}
	}

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}

	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->material = material;
	element->matrix = matrix;
	element->list = NULL;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;
	element->instanceMatrix = NULL;
	element->instanceMatrixBuffer = NULL;
	element->instanceColorBuffer = NULL;

	element->next = batch->elementList;
	batch->elementList = element;
}

void
vsRenderQueueStage::AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsDisplayList *batchList )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceMatrixBuffer = NULL;
	element->instanceColorBuffer = NULL;
	element->list = batchList;

	element->next = batch->elementList;
	batch->elementList = element;
}

void
vsRenderQueueStage::AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batchList, vsShaderValues *values )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->instanceMatrixBuffer = matrixBuffer;
	element->instanceColorBuffer = colorBuffer;
	element->list = batchList;

	element->next = batch->elementList;
	batch->elementList = element;
}

void
vsRenderQueueStage::AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsDisplayList *batchList, vsShaderValues *values )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceColor = color;
	element->list = batchList;

	element->next = batch->elementList;
	batch->elementList = element;
}
void
vsRenderQueueStage::AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceMatrixBuffer = NULL;
	element->instanceColorBuffer = NULL;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;

	element->next = batch->elementList;
	batch->elementList = element;
}

void
vsRenderQueueStage::AddSimpleInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->instanceMatrixBuffer = matrixBuffer;
	element->instanceColorBuffer = colorBuffer;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;

	element->next = batch->elementList;
	batch->elementList = element;
}

void
vsRenderQueueStage::AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = NULL;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceColor = color;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;

	element->next = batch->elementList;
	batch->elementList = element;
}

vsDisplayList *
vsRenderQueueStage::MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size )
{
	Batch *batch = FindBatch(material);

	if ( m_batchElementPool == NULL )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = m_batchElementPool->next;
	element->next = NULL;
	element->Clear();

	element->material = material;
	element->matrix = matrix;
	element->list = new vsDisplayList(size);

	element->next = batch->elementList;
	batch->elementList = element;
	m_temporaryLists.AddItem(element->list);

	return element->list;
}

void
vsRenderQueueStage::StartRender()
{
	m_batchCount = 0;
	vsAssert( m_batch == NULL, "Batches not cleared?" );
	//	m_batch = NULL;


}

void
vsRenderQueueStage::Draw( vsDisplayList *list )
{
	for (Batch *b = m_batch; b; b = b->next)
	{
		for (BatchElement *e = b->elementList; e; e = e->next)
		{
			list->SetMaterial( e->material );
			if ( e->batch )
			{
				list->SetMatrix4x4( vsMatrix4x4::Identity );
				e->batch->Draw(list);
				list->PopTransform();
			}
			else
			{
				if ( e->instanceMatrixBuffer )
					list->SetMatrices4x4Buffer( e->instanceMatrixBuffer );
				else if ( e->instanceMatrix )
					list->SetMatrices4x4( e->instanceMatrix, e->instanceMatrixCount );
				else
					list->SetMatrix4x4( e->matrix );
				list->SetShaderValues( e->shaderValues );
				if ( e->instanceColorBuffer )
					list->SetColorsBuffer( e->instanceColorBuffer );
				else if ( e->instanceColor )
					list->SetColors( e->instanceColor, e->instanceMatrixCount );

				if ( e->list )
					list->Append( *e->list );
				else if ( e->vbo && e->ibo )
				{
					list->BindBuffer( e->vbo );
					if ( e->simpleType == vsFragment::SimpleType_TriangleList )
						list->TriangleListBuffer( e->ibo );
					else if ( e->simpleType == vsFragment::SimpleType_TriangleFan )
						list->TriangleFanBuffer( e->ibo );
					else if ( e->simpleType == vsFragment::SimpleType_TriangleStrip )
						list->TriangleStripBuffer( e->ibo );
					list->ClearArrays();
				}
				list->PopTransform();
			}
		}
	}
}

void
vsRenderQueueStage::EndRender()
{
	m_batchCount = 0;
	Batch *last = NULL;
	for (Batch *b = m_batch; b; b = b->next)
	{
		last = b;
		BatchElement *lastElement = b->elementList;
		while ( lastElement->next )
		{
			lastElement->instanceMatrix = NULL;
			lastElement->instanceMatrixCount = 0;
			lastElement = lastElement->next;
		}
		lastElement->instanceMatrix = NULL;
		lastElement->instanceMatrixCount = 0;
		lastElement->next = m_batchElementPool;
		m_batchElementPool = b->elementList;
		b->elementList = NULL;
	}
	if ( last )
	{
		last->next = m_batchPool;
		m_batchPool = m_batch;
	}
	m_batch = NULL;

	m_temporaryLists.Clear();
	m_batchMap->map.clear();
}

vsRenderQueue::vsRenderQueue( int stageCount, int genericListSize):
	m_parent(NULL),
	m_genericList(new vsDisplayList(genericListSize)),
	m_stage(new vsRenderQueueStage[4]),
	m_stageCount(4),
	m_transformStack(),
	m_transformStackLevel(0)
	// m_orthographic(true)
{
}

vsRenderQueue::~vsRenderQueue()
{
	vsDeleteArray( m_stage );
	vsDelete( m_genericList );
}

void
vsRenderQueue::StartRender(vsScene *parent, int materialHideFlags)
{
	m_materialHideFlags = materialHideFlags;
	m_parent = parent;
	InitialiseTransformStack();

	for ( int i = 0; i < m_stageCount; i++ )
	{
		m_stage[i].StartRender();
	}
	m_genericList->Clear();
}

void
vsRenderQueue::StartRender( const vsMatrix4x4& projection, const vsMatrix4x4& worldToView, const vsMatrix4x4& iniMatrix, int materialHideFlags )
{
	m_materialHideFlags = materialHideFlags;
	m_parent = NULL;
	m_projection = projection;
	m_worldToView = worldToView;
	m_transformStack[0] = iniMatrix;
	m_transformStackLevel = 1;

	for ( int i = 0; i < m_stageCount; i++ )
	{
		m_stage[i].StartRender();
	}
	m_genericList->Clear();
}

void
vsRenderQueue::Draw( vsDisplayList *list )
{
	for ( int i = 0; i < 3; i++ )
	{
		m_stage[i].Draw(list);
	}
	list->Append(*m_genericList);
	m_stage[3].Draw(list);

	DeinitialiseTransformStack();
	vsAssert( m_transformStackLevel == 0, "Unbalanced push/pop of transforms?");
}

void
vsRenderQueue::EndRender()
{
	for ( int i = 0; i < m_stageCount; i++ )
	{
		m_stage[i].EndRender();
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
		vsMatrix4x4 startingMatrix = startingTransform.GetMatrix();

		vsMatrix4x4 requestedMatrix = vsMatrix4x4::Identity;

		requestedMatrix.w -= m_parent->GetCamera3D()->GetPosition();
        //
		vsMatrix4x4 myIdentity;
		myIdentity.x *= -1.f;
        //
		vsMatrix4x4 cameraMatrix = m_parent->GetCamera3D()->GetTransform().GetMatrix();

		vsVector3D forward = cameraMatrix.z;
		vsVector3D up = cameraMatrix.y;
		vsVector3D side = forward.Cross(up);

		cameraMatrix.x = side;
		cameraMatrix.y = up;
		cameraMatrix.z = forward;
		cameraMatrix.w.Set(0.f,0.f,0.f,1.f);
		cameraMatrix.Invert();

		// vsMatrix4x4 cameraMatrix = m_parent->GetCamera3D()->GetTransform().GetMatrix();
		// cameraMatrix.Invert();
		cameraMatrix = startingMatrix * myIdentity * cameraMatrix;

		m_worldToView = cameraMatrix * requestedMatrix;
		m_transformStack[0] = vsMatrix4x4::Identity;
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
		vsTransform2D cameraTransform = m_parent->GetCamera()->GetCameraTransform();
		// cameraMatrix will have a scale on its members from the camera. (Since
		// that's where it stores the FOV).
		// We remove that, since that eventually becomes part of the PROJECTION
		// transform, not the MODELVIEW transform, which is all we care about here..
		cameraTransform.SetScale(vsVector2D(1.f,1.f));
		vsMatrix4x4 cameraMatrix = cameraTransform.GetMatrix();
		cameraMatrix.Invert();

		m_worldToView = cameraMatrix * startingMatrix;
		m_transformStack[0] = vsMatrix4x4::Identity;
		m_transformStackLevel = 1;
	}
}

bool
vsRenderQueue::IsOrthographic()
{
	if ( m_parent->Is3D() )
	{
		if ( m_parent->GetCamera3D()->GetProjectionType() == vsCamera3D::PT_Perspective )
			return false;
	}
	return true;
}

void
vsRenderQueue::DeinitialiseTransformStack()
{
	m_transformStackLevel--;
}

void
vsRenderQueue::AddSimpleBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );

	m_stage[stageId].AddSimpleBatch( material, matrix, vbo, ibo, simpleType );
}

void
vsRenderQueue::AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );

	m_stage[stageId].AddBatch( material, matrix, batch );
}

void
vsRenderQueue::AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batch, vsShaderValues *values )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddInstanceBatch( material, matrixBuffer, colorBuffer, batch, values );
}

void
vsRenderQueue::AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsDisplayList *batch, vsShaderValues *values)
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddInstanceBatch( material, matrix, color, instanceCount, batch, values );
}

void
vsRenderQueue::AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int instanceCount, vsDisplayList *batch )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddInstanceBatch( material, matrix, instanceCount, batch );
}

void
vsRenderQueue::AddSimpleInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddSimpleInstanceBatch( material, matrixBuffer, colorBuffer, vbo, ibo, simpleType, values );
}

void
vsRenderQueue::AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values)
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddSimpleInstanceBatch( material, matrix, color, instanceCount, vbo, ibo, simpleType, values );
}

void
vsRenderQueue::AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int instanceCount, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddSimpleInstanceBatch( material, matrix, instanceCount, vbo, ibo, simpleType );
}

void
vsRenderQueue::AddFragmentBatch( vsFragment *fragment )
{
	if ( fragment->IsSimple() )
		AddSimpleBatch( fragment->GetMaterial(), GetMatrix(), fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType() );
	else
		AddBatch( fragment->GetMaterial(), GetMatrix(), fragment->GetDisplayList() );
}

void
vsRenderQueue::AddFragmentInstanceBatch( vsFragment *fragment, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsShaderValues *values )
{
	if ( fragment->IsSimple() )
		AddSimpleInstanceBatch( fragment->GetMaterial(), matrix, color, instanceCount, fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType(), values );
	else
		AddInstanceBatch( fragment->GetMaterial(), matrix, color, instanceCount, fragment->GetDisplayList(), values );
}

void
vsRenderQueue::AddFragmentInstanceBatch( vsFragment *fragment, const vsMatrix4x4 *matrix, int instanceCount )
{
	if ( fragment->IsSimple() )
		AddSimpleInstanceBatch( fragment->GetMaterial(), matrix, instanceCount, fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType() );
	else
		AddInstanceBatch( fragment->GetMaterial(), matrix, instanceCount, fragment->GetDisplayList() );
}

void
vsRenderQueue::AddFragmentInstanceBatch( vsFragment *fragment, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsShaderValues *values )
{
	if ( fragment->IsSimple() )
		AddSimpleInstanceBatch( fragment->GetMaterial(), matrixBuffer, colorBuffer, fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType(), values );
	else
		AddInstanceBatch( fragment->GetMaterial(), matrixBuffer, colorBuffer, fragment->GetDisplayList(), values );
}

vsDisplayList *
vsRenderQueue::MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size )
{
	int stageId = PickStageForMaterial( material );

	return m_stage[stageId].MakeTemporaryBatchList( material, matrix, size );
}

vsDisplayList *
vsRenderQueue::MakeTemporaryBatchList( vsMaterial *material, int size )
{
	int stageId = PickStageForMaterial( material );

	return m_stage[stageId].MakeTemporaryBatchList( material, m_transformStack[0], size );
}

vsRenderQueueStage *
vsRenderQueue::GetStage( int i )
{
	vsAssert( i >= 0 && i < m_stageCount, "Requested nonexistant render stage!" );

	if ( i >= 0 && i < m_stageCount )
	{
		return &m_stage[i];
	}

	return NULL;
}


const vsMatrix4x4&
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

const vsMatrix4x4&
vsRenderQueue::SetMatrix( const vsMatrix4x4 &matrix )
{
	vsAssert( m_transformStackLevel < MAX_STACK_DEPTH, "Transform stack overflow!" )
	vsAssert( m_transformStackLevel > 0, "Uninitialised transform stack??" )

	if ( m_transformStackLevel < MAX_SCENE_STACK )
	{
		m_transformStack[m_transformStackLevel] = matrix;
		m_transformStackLevel++;
	}

	return m_transformStack[ m_transformStackLevel-1 ];
}

const vsMatrix4x4&
vsRenderQueue::PushTransform2D( const vsTransform2D &transform )
{
	vsAssert( m_transformStackLevel < MAX_STACK_DEPTH, "Transform stack overflow!" )
	vsAssert( m_transformStackLevel > 0, "Uninitialised transform stack??" )

	if ( m_transformStackLevel < MAX_SCENE_STACK )
	{
		vsMatrix4x4 matrix = transform.GetMatrix();
		// matrix.SetTranslation( -1.f * matrix.w );
		m_transformStack[m_transformStackLevel] = m_transformStack[m_transformStackLevel-1] * matrix;
		m_transformStackLevel++;
	}

	return m_transformStack[ m_transformStackLevel-1 ];
}

const vsMatrix4x4&
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

const vsMatrix4x4&
vsRenderQueue::GetMatrix()
{
	vsAssert( m_transformStackLevel > 0, "Nothing in the transform stack!" );

	return m_transformStack[m_transformStackLevel-1];
}

const vsMatrix4x4&
vsRenderQueue::GetWorldToViewMatrix()
{
	return m_worldToView;
}

int
vsRenderQueue::PickStageForMaterial( vsMaterial *material )
{
	if ( material->GetResource()->m_postGeneric )
	{
		return 3;
	}
	if ( material->GetResource()->m_postGlow )
	{
		return 2;
	}
	else if ( material->GetResource()->m_glow && !material->GetResource()->m_preGlow )
	{
		return 1;
	}
	return 0;
}



