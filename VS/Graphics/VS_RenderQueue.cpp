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

#include "VS_Profile.h"

class vsRenderQueueStage
{
public:
	struct BatchMap;
	struct BatchElement;
	struct Batch;
private:
	vsMatrix4x4			m_worldToView;

	BatchMap*			m_batchMap;
	Batch*				m_batch;
	int					m_batchCount;

	Batch *				m_batchPool;
	BatchElement * 		m_batchElementPool;

	vsLinkedListStore<vsDisplayList>	m_temporaryLists;

	Batch *			FindBatch( vsMaterial *material );

	inline void _AddElementToBatch( vsRenderQueueStage::BatchElement *element, vsRenderQueueStage::Batch *batch );

public:

	vsRenderQueueStage();
	~vsRenderQueueStage();

	void			SetWorldToView( const vsMatrix4x4& wtv ) { m_worldToView = wtv; }

	void			StartRender();
	void			Draw( vsDisplayList *list );	// write our batches into here.
	void			EndRender();

	// Add a batch to this stage
	void			AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch );
	void			AddBatch( vsMaterial *material, vsVertexArrayObject *vao, const vsMatrix4x4 &matrix, vsDisplayList *batch );
	void			AddSimpleBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type );
	void			AddSimpleBatch( vsMaterial *material, vsVertexArrayObject *vao, const vsMatrix4x4 &matrix, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsDisplayList *batch, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batch, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );
	void			AddInstanceBatch( vsMaterial *material, vsVertexArrayObject *vao, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batch, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );

	void			AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );
	void			AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type );
	void			AddSimpleInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );
	void			AddSimpleInstanceBatch( vsMaterial *material, vsVertexArrayObject *vao, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType type, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );

	// For stuff which really doesn't want to keep its display list around, call this to get a temporary display list.
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size );
};


struct vsRenderQueueStage::BatchElement
{
	vsMatrix4x4		matrix;
	vsMaterial *	material;
	vsShaderValues *shaderValues;
	vsShaderOptions *shaderOptions;
	const vsMatrix4x4 *	instanceMatrix;
	const vsColor *	instanceColor;
	int				instanceMatrixCount;
	vsRenderBuffer *instanceMatrixBuffer;
	vsRenderBuffer *instanceColorBuffer;
	vsDisplayList *	list;
	vsVertexArrayObject *vao;
	vsRenderBuffer *vbo;
	vsRenderBuffer *ibo;
	vsFragment::SimpleType simpleType;
	BatchElement *	next;

	float           sortValue;

	vsDynamicBatch * batch;

	BatchElement():
		material(nullptr),
		shaderValues(nullptr),
		shaderOptions(nullptr),
		instanceMatrix(nullptr),
		instanceColor(nullptr),
		instanceMatrixCount(0),
		instanceMatrixBuffer(nullptr),
		instanceColorBuffer(nullptr),
		list(nullptr),
		vao(nullptr),
		vbo(nullptr),
		ibo(nullptr),
		next(nullptr),
		sortValue(0.f),
		batch(nullptr)
	{
	}

	void Clear()
	{
		material = nullptr;
		shaderValues = nullptr;
		shaderOptions = nullptr;
		instanceMatrix = nullptr;
		instanceMatrixCount = 0;
		instanceMatrixBuffer = nullptr;
		instanceColorBuffer = nullptr;
		instanceColor = nullptr;
		list = nullptr;
		vao = nullptr;
		vbo = nullptr;
		ibo = nullptr;
		sortValue = 0.f;
		batch = nullptr;
	}

};

struct vsRenderQueueStage::Batch
{
	vsMaterialInternal*	material;
	BatchElement*		elementList;
	vsVertexArrayObject *vao;
	Batch*				next;

	Batch();
	~Batch();
};

struct vsRenderQueueStage::BatchMap
{
	std::map<vsMaterialInternal*,vsRenderQueueStage::Batch*> map;
};


vsRenderQueueStage::Batch::Batch():
	material(nullptr),
	elementList(nullptr),
	next(nullptr)
{
}

vsRenderQueueStage::Batch::~Batch()
{
}

vsRenderQueueStage::vsRenderQueueStage():
	m_batchMap(new BatchMap),
	m_batch(nullptr),
	m_batchCount(0),
	m_batchPool(nullptr),
	m_batchElementPool(nullptr)
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

void
vsRenderQueueStage::_AddElementToBatch( vsRenderQueueStage::BatchElement *element, vsRenderQueueStage::Batch *batch )
{
	// check if sorting is enabled.

	if ( batch->material->m_zSort )
	{
		// okay, we need to insert this batch somewhere in our list according to our
		// zSort.

		// how can I figure out my sort value?
		vsVector4D viewPos = m_worldToView.ApplyTo( element->matrix.w );
		float sortVal = viewPos.z;

		element->sortValue = sortVal;
		// now, we insert into this batch's element list.

		if ( !batch->elementList || batch->elementList->sortValue < element->sortValue )
		{
			element->next = batch->elementList;
			batch->elementList = element;
			return;
		}
		else
		{
			BatchElement *next = batch->elementList;
			// if ( !next->next && next->sortValue < element->sortValue )
			// {
			// 	// we go BEFORE the first thing in the list now!
            //
			// 	element->next = batch->elementList;
			// 	batch->elementList = element;
			// 	return;
			// }

			while ( next )
			{
				if ( !next->next || next->next->sortValue < element->sortValue )
				{
					element->next = next->next;
					next->next = element;
					return;
				}
				else
				{
					next = next->next;
				}
			}
		}
	}
	else
	{
		element->next = batch->elementList;
		batch->elementList = element;
	}
}


vsRenderQueueStage::Batch *
vsRenderQueueStage::FindBatch( vsMaterial *material )
{
	PROFILE("RenderQueueStage::FindBatch");

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
	batch->next = nullptr;
	batch->material = material->GetResource();

	// insert this batch into our batch list, SORTED.
	bool inserted = false;

	if ( m_batch == nullptr )
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
	AddBatch( material, nullptr, matrix, batchList );
}

void
vsRenderQueueStage::AddBatch( vsMaterial *material, vsVertexArrayObject *vao, const vsMatrix4x4 &matrix, vsDisplayList *batchList )
{
	PROFILE("AddBatch");
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->matrix = matrix;
	element->list = batchList;
	element->vao = vao;
	element->instanceMatrix = nullptr;
	element->instanceMatrixBuffer = nullptr;
	element->instanceColorBuffer = nullptr;

	_AddElementToBatch( element, batch );
}

void
vsRenderQueueStage::AddSimpleBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType simpleType)
{
	AddSimpleBatch( material, nullptr, matrix, vbo, ibo, simpleType );
}

void
vsRenderQueueStage::AddSimpleBatch( vsMaterial *material, vsVertexArrayObject *vao, const vsMatrix4x4 &matrix, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType simpleType)
{
	PROFILE("AddSimpleBatch");
	Batch *batch = FindBatch(material);

	if ( vsSystem::Instance()->GetPreferences()->GetDynamicBatching() )
	{
		// Check for compatible simple BatchElements in this batch.  If I find
		// one, we'll merge together.
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

		BatchElement *mergeCandidate = nullptr;
		// don't even try to merge things that are too big.
		if ( vbo->GetPositionCount() < 100 &&
				vsDynamicBatch::Supports( vbo->GetContentType() ) )
		{
			PROFILE("Finding merge candidate");
			mergeCandidate = batch->elementList;
			while(mergeCandidate)
			{
				// [TODO] I should also be checking whether there's space in the
				// mergeCandidate's buffer to merge with it.
				if ( mergeCandidate->instanceMatrix == nullptr &&
						mergeCandidate->vbo &&
						mergeCandidate->vbo->GetContentType() == vbo->GetContentType() &&
						mergeCandidate->material->MatchesForBatching( material ) )
				{
					if ( mergeCandidate->batch == nullptr )
					{
						if ( mergeCandidate->vbo->GetPositionCount() + vbo->GetPositionCount() < 100 )
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
			PROFILE("Doing merge");
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

			if ( mergeCandidate->batch == nullptr )
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
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->matrix = matrix;
	element->list = nullptr;
	element->vao = vao;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;
	element->instanceMatrix = nullptr;
	element->instanceMatrixBuffer = nullptr;
	element->instanceColorBuffer = nullptr;

	_AddElementToBatch( element, batch );
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
	element->next = nullptr;
	element->Clear();

	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceMatrixBuffer = nullptr;
	element->instanceColorBuffer = nullptr;
	element->list = batchList;

	_AddElementToBatch( element, batch );
}

void
vsRenderQueueStage::AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batchList, vsShaderValues *values, vsShaderOptions *options )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->shaderOptions = options;
	element->instanceMatrixBuffer = matrixBuffer;
	element->instanceColorBuffer = colorBuffer;
	element->list = batchList;

	_AddElementToBatch( element, batch );
}

void
vsRenderQueueStage::AddInstanceBatch( vsMaterial *material, vsVertexArrayObject *vao, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batchList, vsShaderValues *values, vsShaderOptions *options )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->shaderOptions = options;
	element->vao = vao;
	element->instanceMatrixBuffer = matrixBuffer;
	element->instanceColorBuffer = colorBuffer;
	element->list = batchList;

	_AddElementToBatch( element, batch );
}

void
vsRenderQueueStage::AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsDisplayList *batchList, vsShaderValues *values, vsShaderOptions *options )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->shaderOptions = options;
	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceColor = color;
	element->list = batchList;

	_AddElementToBatch( element, batch );
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
	element->next = nullptr;
	element->Clear();

	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceMatrixBuffer = nullptr;
	element->instanceColorBuffer = nullptr;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;

	_AddElementToBatch( element, batch );
}

void
vsRenderQueueStage::AddSimpleInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values, vsShaderOptions *options )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->shaderOptions = options;
	element->instanceMatrixBuffer = matrixBuffer;
	element->instanceColorBuffer = colorBuffer;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;

	_AddElementToBatch( element, batch );
}

void
vsRenderQueueStage::AddSimpleInstanceBatch( vsMaterial *material, vsVertexArrayObject *vao, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values, vsShaderOptions *options )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->shaderOptions = options;
	element->instanceMatrixBuffer = matrixBuffer;
	element->instanceColorBuffer = colorBuffer;
	element->vao = vao;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;

	_AddElementToBatch( element, batch );
}

void
vsRenderQueueStage::AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values, vsShaderOptions *options )
{
	Batch *batch = FindBatch(material);

	if ( !m_batchElementPool )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = element->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->shaderValues = values;
	element->shaderOptions = options;
	element->instanceMatrixCount = matrixCount;
	element->instanceMatrix = matrix;
	element->instanceColor = color;
	element->vbo = vbo;
	element->ibo = ibo;
	element->simpleType = simpleType;

	_AddElementToBatch( element, batch );
}

vsDisplayList *
vsRenderQueueStage::MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size )
{
	Batch *batch = FindBatch(material);

	if ( m_batchElementPool == nullptr )
	{
		m_batchElementPool = new BatchElement;
	}
	BatchElement *element = m_batchElementPool;
	m_batchElementPool = m_batchElementPool->next;
	element->next = nullptr;
	element->Clear();

	element->material = material;
	element->matrix = matrix;
	element->list = new vsDisplayList(size);

	_AddElementToBatch( element, batch );
	m_temporaryLists.AddItem(element->list);

	return element->list;
}

void
vsRenderQueueStage::StartRender()
{
	m_batchCount = 0;
	vsAssert( m_batch == nullptr, "Batches not cleared?" );
	//	m_batch = nullptr;


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
				if ( e->vao )
					list->SetVertexArrayObject( e->vao );
				else
					list->ClearVertexArrayObject();

				if ( e->instanceMatrixBuffer )
					list->SetMatrices4x4Buffer( e->instanceMatrixBuffer );
				else if ( e->instanceMatrix )
					list->SetMatrices4x4( e->instanceMatrix, e->instanceMatrixCount );
				else
					list->SetMatrix4x4( e->matrix );

				// We need to set shader values even if it's nullptr, to remove whatever
				// shader values object was last used.  If we were smart we would
				// remember what went in last so we weren't re-setting nullptrs, but
				// maybe I'll leave that as a [TODO] rather than worrying about it
				// riht now while I'm working on shader options.  -- Trevor 24/6/2020
				if ( e->shaderValues )
					list->SetShaderValues( e->shaderValues );
				if ( e->shaderOptions )
					list->PushShaderOptions( *e->shaderOptions );
				if ( e->instanceColorBuffer )
					list->SetColorsBuffer( e->instanceColorBuffer );
				else if ( e->instanceColor )
					list->SetColors( e->instanceColor, e->instanceMatrixCount );

				if ( e->list )
					list->Append( *e->list );
				else
				{
					if ( e->ibo )
					{
						if ( e->vbo )
							list->BindBuffer( e->vbo );

						if ( e->simpleType == vsFragment::SimpleType_TriangleList )
							list->TriangleListBuffer( e->ibo );
						else if ( e->simpleType == vsFragment::SimpleType_TriangleFan )
							list->TriangleFanBuffer( e->ibo );
						else if ( e->simpleType == vsFragment::SimpleType_TriangleStrip )
							list->TriangleStripBuffer( e->ibo );
					}
				}
				list->ClearArrays();

				if ( e->shaderOptions )
					list->PopShaderOptions();
				if ( e->shaderValues )
					list->ClearShaderValues();
				list->PopTransform();
			}
		}
	}
}

void
vsRenderQueueStage::EndRender()
{
	m_batchCount = 0;
	Batch *last = nullptr;
	for (Batch *b = m_batch; b; b = b->next)
	{
		last = b;
		BatchElement *lastElement = b->elementList;
		while ( lastElement->next )
		{
			lastElement->instanceMatrix = nullptr;
			lastElement->instanceMatrixCount = 0;
			lastElement = lastElement->next;
		}
		lastElement->instanceMatrix = nullptr;
		lastElement->instanceMatrixCount = 0;
		lastElement->next = m_batchElementPool;
		m_batchElementPool = b->elementList;
		b->elementList = nullptr;
	}
	if ( last )
	{
		last->next = m_batchPool;
		m_batchPool = m_batch;
	}
	m_batch = nullptr;

	m_temporaryLists.Clear();
	m_batchMap->map.clear();
}

vsRenderQueue::vsRenderQueue():
	m_scene(nullptr),
	m_genericList(new vsDisplayList(1024 * 100, true)),
	m_stage(new vsRenderQueueStage[4]),
	m_stageCount(4),
	m_transformStack(),
	m_transformStackLevel(0),
	m_rendering(false)
	// m_orthographic(true)
{
}

vsRenderQueue::~vsRenderQueue()
{
	vsAssert( !m_rendering, "vsRenderQueue destructor called when already in the middle of a render" );

	vsDeleteArray( m_stage );
	vsDelete( m_genericList );
}

void
vsRenderQueue::StartRender(vsScene *parent, int materialHideFlags)
{
	vsAssert( !m_rendering, "vsRenderQueue::StartRender called when already in the middle of a render" );
	m_rendering = true;

	m_materialHideFlags = materialHideFlags;
	m_scene = parent;

	m_screenBox.Set( m_scene->GetTopLeftCorner(), m_scene->GetBottomRightCorner() );

	InitialiseTransformStack();

	for ( int i = 0; i < m_stageCount; i++ )
	{
		m_stage[i].StartRender();
	}
	m_genericList->Clear();
}

void
vsRenderQueue::StartRender( const vsMatrix4x4& projection, const vsMatrix4x4& worldToView, const vsMatrix4x4& iniMatrix, const vsBox2D& screenBox, int materialHideFlags )
{
	vsAssert( !m_rendering, "vsRenderQueue::StartRender called when already in the middle of a render" );
	m_rendering = true;

	m_materialHideFlags = materialHideFlags;
	m_scene = nullptr;
	m_screenBox = screenBox;
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
	PROFILE("RenderQueue::Draw");

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
	PROFILE("RenderQueue::EndRender");
	for ( int i = 0; i < m_stageCount; i++ )
	{
		m_stage[i].EndRender();
	}
	m_genericList->Clear();
	m_rendering = false;
}

void
vsRenderQueue::InitialiseTransformStack()
{
	m_worldToView = m_scene->CalculateWorldToViewMatrix();
	m_transformStack[0] = vsMatrix4x4::Identity;
	m_transformStackLevel = 1;

	for ( int i = 0; i < m_stageCount; i++ )
		m_stage[i].SetWorldToView( m_worldToView );
}

bool
vsRenderQueue::IsOrthographic()
{
	if ( m_scene->Is3D() )
	{
		if ( m_scene->GetCamera3D()->GetProjectionType() == vsCamera3D::PT_Perspective )
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
vsRenderQueue::AddSimpleBatch( vsMaterial *material, vsVertexArrayObject *vao, const vsMatrix4x4 &matrix, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );

	m_stage[stageId].AddSimpleBatch( material, vao, matrix, vbo, ibo, simpleType );
}

void
vsRenderQueue::AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );

	m_stage[stageId].AddBatch( material, nullptr, matrix, batch );
}

void
vsRenderQueue::AddBatch( vsMaterial *material, vsVertexArrayObject *vao, const vsMatrix4x4 &matrix, vsDisplayList *batch )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );

	m_stage[stageId].AddBatch( material, vao, matrix, batch );
}

void
vsRenderQueue::AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batch, vsShaderValues *values, vsShaderOptions *options  )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddInstanceBatch( material, matrixBuffer, colorBuffer, batch, values );
}

void
vsRenderQueue::AddInstanceBatch( vsMaterial *material, vsVertexArrayObject *vao, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batch, vsShaderValues *values, vsShaderOptions *options )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddInstanceBatch( material, vao, matrixBuffer, colorBuffer, batch, values );
}

void
vsRenderQueue::AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsDisplayList *batch, vsShaderValues *values, vsShaderOptions *options )
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
vsRenderQueue::AddSimpleInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values, vsShaderOptions *options )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddSimpleInstanceBatch( material, matrixBuffer, colorBuffer, vbo, ibo, simpleType, values, options );
}

void
vsRenderQueue::AddSimpleInstanceBatch( vsMaterial *material, vsVertexArrayObject *vao, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values, vsShaderOptions *options )
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddSimpleInstanceBatch( material, vao, matrixBuffer, colorBuffer, vbo, ibo, simpleType, values, options );
}

void
vsRenderQueue::AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType, vsShaderValues *values, vsShaderOptions *options)
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return; // don't draw!

	int stageId = PickStageForMaterial( material );
	m_stage[stageId].AddSimpleInstanceBatch( material, matrix, color, instanceCount, vbo, ibo, simpleType, values, options );
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
		AddSimpleBatch( fragment->GetMaterial(), fragment->GetVAO(), GetMatrix(), fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType() );
	else
		AddBatch( fragment->GetMaterial(), fragment->GetVAO(), GetMatrix(), fragment->GetDisplayList() );
}

void
vsRenderQueue::AddFragmentInstanceBatch( vsFragment *fragment, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsShaderValues *values, vsShaderOptions *options )
{
	if ( fragment->IsSimple() )
		AddSimpleInstanceBatch( fragment->GetMaterial(), matrix, color, instanceCount, fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType(), values, options );
	else
		AddInstanceBatch( fragment->GetMaterial(), matrix, color, instanceCount, fragment->GetDisplayList(), values, options );
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
vsRenderQueue::AddFragmentInstanceBatch( vsFragment *fragment, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsShaderValues *values, vsShaderOptions *options )
{
	if ( fragment->IsSimple() )
		AddSimpleInstanceBatch( fragment->GetMaterial(), fragment->GetVAO(), matrixBuffer, colorBuffer, fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType(), values, options );
	else
		AddInstanceBatch( fragment->GetMaterial(), fragment->GetVAO(), matrixBuffer, colorBuffer, fragment->GetDisplayList(), values, options );
}

void
vsRenderQueue::AddFragmentInstanceBatch( vsFragment *fragment, vsVertexArrayObject *vao, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsShaderValues *values , vsShaderOptions *options )
{
	if ( fragment->IsSimple() )
		AddSimpleInstanceBatch( fragment->GetMaterial(), vao, matrixBuffer, colorBuffer, fragment->GetSimpleVBO(), fragment->GetSimpleIBO(), fragment->GetSimpleType(), values, options );
	else
		AddInstanceBatch( fragment->GetMaterial(), vao, matrixBuffer, colorBuffer, fragment->GetDisplayList(), values, options );
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

	return nullptr;
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

bool
vsRenderQueue::WouldMaterialBeHidden( vsMaterial *material ) const
{
	if ( (material->GetResource()->m_flags & m_materialHideFlags) != 0 )
		return true;
	return false;
}

