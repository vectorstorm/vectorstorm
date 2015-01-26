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
class vsFragment;

class vsRenderQueueStage
{
	struct BatchElement
	{
		vsMatrix4x4		matrix;
		vsMaterial *	material;
		const vsMatrix4x4 *	instanceMatrix;
		const vsColor *	instanceColor;
		int				instanceMatrixCount;
		vsRenderBuffer *instanceMatrixBuffer;
		vsDisplayList *	list;
		BatchElement *	next;

		BatchElement():
			material(NULL),
			instanceMatrix(NULL),
			instanceColor(NULL),
			instanceMatrixCount(0),
			instanceMatrixBuffer(NULL),
			list(NULL),
			next(NULL)
		{
		}

		void Clear()
		{
			material = NULL;
			instanceMatrix = NULL;
			instanceMatrixCount = 0;
			instanceMatrixBuffer = NULL;
			instanceColor = NULL;
			list = NULL;
		}
	};
	struct Batch
	{
		vsMaterialInternal*	material;
		BatchElement*		elementList;
		Batch*				next;

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

	// Add a batch to this stage
	void			AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsDisplayList *batch );

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

	vsMatrix4x4				m_projection;
	vsMatrix4x4				m_worldToView;
	vsMatrix4x4				m_transformStack[MAX_STACK_DEPTH];
	int						m_transformStackLevel;

	float m_fov;
	// bool m_orthographic;

	int				PickStageForMaterial( vsMaterial *material );

	void			InitialiseTransformStack();
	void			DeinitialiseTransformStack();

public:
	vsRenderQueue( int stageCount, int genericListSize);
	~vsRenderQueue();

	void			StartRender( vsScene *scene );
	void			StartRender( const vsMatrix4x4& projection, const vsMatrix4x4& worldToView, const vsMatrix4x4& iniMatrix);
	void			Draw( vsDisplayList *list );	// write our queue contents into here.  Called internally.
	void			EndRender();

	vsMatrix4x4		PushMatrix( const vsMatrix4x4 &matrix );
    vsMatrix4x4     PushTransform2D( const vsTransform2D &transform );
	vsMatrix4x4		PushTranslation( const vsVector3D &vector );

	void			PopMatrix();
	const vsMatrix4x4&		GetMatrix();
	const vsMatrix4x4&		GetWorldToViewMatrix();
	const vsMatrix4x4&		GetProjectionMatrix() { return m_projection; }
	float			GetFOV() { return m_fov; }
	void			SetFOV( float fov ) { m_fov = fov; }
	bool			IsOrthographic();

	void SetProjectionMatrix( const vsMatrix4x4& mat ) { m_projection = mat; }

	vsScene *		GetScene() { return m_parent; }

	// Usual way to add a batch
	void			AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch );

	// Identity-matrix batches.
	void			AddBatch( vsMaterial *material, vsDisplayList *batch )  { AddBatch( material, vsMatrix4x4::Identity, batch ); }

	// batches which will draw in multiple places.
	// Note that the passed array of matrices must exist until the Draw phase ends!
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int instanceCount, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsDisplayList *batch );

	// ultra-convenience for fragments.
	void			AddFragmentBatch( vsFragment *fragment );
	// For fragments using instancing.
	// Note that the passed array of matrices must exist until the Draw phase ends!
	void			AddFragmentInstanceBatch( vsFragment *fragment, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount );
	void			AddFragmentInstanceBatch( vsFragment *fragment, const vsMatrix4x4 *matrix, int instanceCount );
	void			AddFragmentInstanceBatch( vsFragment *fragment, vsRenderBuffer *matrixBuffer );

	// For stuff which really doesn't want to keep its display list around, call this to get a temporary display list.
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, int size );
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size );

	// BAD, BAD CODE.  :)  Ought to get rid of this.
	vsDisplayList * GetGenericList() { return m_genericList; }

	vsRenderQueueStage * GetStage( int i = 0 );
};

#endif // VS_SCENE_DRAW_H

