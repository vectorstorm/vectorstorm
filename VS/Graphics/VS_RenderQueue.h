/*
 *  VS_RenderQueue.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 5/09/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDER_QUEUE_H
#define VS_RENDER_QUEUE_H

#include "VS/Math/VS_Vector.h"
#include "VS/Math/VS_Transform.h"
#include "VS/Graphics/VS_DisplayList.h"
#include "VS/Graphics/VS_Screen.h"
#include "VS/Utils/VS_LinkedList.h"
#include "VS/Utils/VS_LinkedListStore.h"
#include "VS/Utils/VS_Pool.h"

class vsEntity;
class vsDisplayList;
class vsCamera2D;
class vsCamera3D;
class vsFog;
class vsLight;


class vsRenderQueueStage
{
	struct BatchElement
	{
		vsMatrix4x4		matrix;
		vsDisplayList *	list;
		BatchElement *	next;
	};
	struct Batch
	{
		vsMaterial		material;
		BatchElement*	elementList;
		Batch*			next;

		Batch();
		~Batch();
	};

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

	// usual way to add a batch
//	void			AddBatch( vsMaterial *material, vsDisplayList *batch ) { AddBatch( material, vsMatrix4x4::Identity, batch ); }

	// convenience for fragments, so they don't need to stuff the matrix into the display list -- we'll do it for them.
	void			AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch );

	// For stuff which really doesn't want to keep its display list around, call this to get a temporary display list.
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size );
};

#define MAX_STACK_DEPTH (20)

class vsRenderQueue
{
	vsScene *				m_parent;

	vsDisplayList *			m_genericList;

	vsRenderQueueStage *	m_stage;
	int						m_stageCount;

	vsMatrix4x4				m_transformStack[MAX_STACK_DEPTH];
	int						m_transformStackLevel;

	int				PickStageForMaterial( vsMaterial *material );

	void			InitialiseTransformStack();
	void			DeinitialiseTransformStack();

public:
	vsRenderQueue( int stageCount, int genericListSize);
	~vsRenderQueue();

	void			StartRender( vsScene *scene );
	void			Draw( vsDisplayList *list );	// write our queue contents into here.  Called internally.
	void			EndRender();

	vsMatrix4x4		PushMatrix( const vsMatrix4x4 &matrix );
    vsMatrix4x4     PushTransform2D( const vsTransform2D &transform );
	vsMatrix4x4		PushTranslation( const vsVector3D &vector );

	void			PopMatrix();
	vsMatrix4x4		GetMatrix();


	vsScene *		GetScene() { return m_parent; }

	// usual way to add a batch
	void			AddBatch( vsMaterial *material, vsDisplayList *batch )  { AddBatch( material, vsMatrix4x4::Identity, batch ); }

	// convenience for fragments, so they don't need to stuff the matrix into the display list -- we'll do it for them.
	void			AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch );

	// For stuff which really doesn't want to keep its display list around, call this to get a temporary display list.
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, int size );
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size );

	// BAD, BAD CODE.  :)  Ought to get rid of this.
	vsDisplayList * GetGenericList() { return m_genericList; }

	vsRenderQueueStage * GetStage( int i = 0 );
};

#endif // VS_SCENE_DRAW_H

