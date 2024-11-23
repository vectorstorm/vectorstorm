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
#include "VS/Graphics/VS_Fragment.h"
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
class vsShaderValues;
class vsRenderQueueStage;

#define MAX_STACK_DEPTH (20)

class vsRenderQueue
{
	vsScene *				m_scene;

	vsBox2D					m_screenBox;
	vsDisplayList *			m_genericList;

	int						m_pixelX; // the pixel dimensions of the render target we're drawing into
	int						m_pixelY; // N.B. these get set during Scene::Draw, so if something *inside* a scene draw changes render target, we won't notice!

	vsRenderQueueStage *	m_stage;
	int						m_stageCount;

	int m_materialHideFlags;

	vsMatrix4x4				m_projection;
	vsMatrix4x4				m_worldToView;
	vsMatrix4x4				m_transformStack[MAX_STACK_DEPTH];
	int						m_transformStackLevel;

	float m_fov;
	bool m_rendering;
	// bool m_orthographic;

	int				PickStageForMaterial( vsMaterial *material );

	void			InitialiseTransformStack();
	void			DeinitialiseTransformStack();

public:
	vsRenderQueue();
	~vsRenderQueue();

	void			StartRender( vsScene *scene, int materialHideFlags = 0 );
	void			StartRender( const vsMatrix4x4& projection, const vsMatrix4x4& worldToView, const vsMatrix4x4& iniMatrix, const vsBox2D& screenBox, int materialHideFlags = 0);
	void			Draw( vsDisplayList *list );	// write our queue contents into here.  Called internally.
	void			EndRender();

	void SetPixelDimensions( int width, int height ) { m_pixelX = width; m_pixelY = height; }
	int GetPixelsX() { return m_pixelX; }
	int GetPixelsY() { return m_pixelY; }

	const vsMatrix4x4& PushMatrix( const vsMatrix4x4 &matrix );
    const vsMatrix4x4& PushTransform2D( const vsTransform2D &transform );
	const vsMatrix4x4& PushTranslation( const vsVector3D &vector );
	const vsBox2D& GetScreenBox() const { return m_screenBox; } // really only makes sense for 2D, where it tells us the coordinate space we're drawing inside.

	const vsMatrix4x4& SetMatrix( const vsMatrix4x4 &matrix ); // explicitly set a full local-to-world matrix.  (The 'Push' commands multiply on top of whatever matrix was already set)

	void			PopMatrix();
	const vsMatrix4x4&		GetMatrix();
	const vsMatrix4x4&		GetWorldToViewMatrix();
	const vsMatrix4x4&		GetProjectionMatrix() { return m_projection; }
	float			GetFOV() { return m_fov; }
	void			SetFOV( float fov ) { m_fov = fov; }
	bool			IsOrthographic();

	void SetProjectionMatrix( const vsMatrix4x4& mat ) { m_projection = mat; }

	vsScene *		GetScene() { return m_scene; }

	// Usual way to add a batch
	void			AddBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsDisplayList *batch );

	// Identity-matrix batches.
	void			AddBatch( vsMaterial *material, vsDisplayList *batch )  { AddBatch( material, vsMatrix4x4::Identity, batch ); }

	// Simple batch with no display list
	void			AddSimpleBatch( vsMaterial *material, const vsMatrix4x4 &matrix, vsRenderBuffer *vbo, vsRenderBuffer *ibo, vsFragment::SimpleType simpleType );

	// batches which will draw in multiple places.
	// Note that the passed array of matrices must exist until the Draw phase ends!
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsDisplayList *batch, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );
	void			AddInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int instanceCount, vsDisplayList *batch );
	void			AddInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsDisplayList *batch, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr  );
	void			AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, const vsColor *color, int matrixCount, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType simpleType, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );
	void			AddSimpleInstanceBatch( vsMaterial *material, const vsMatrix4x4 *matrix, int matrixCount, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType simpleType );
	void			AddSimpleInstanceBatch( vsMaterial *material, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer, vsRenderBuffer* vbo, vsRenderBuffer* ibo, vsFragment::SimpleType simpleType, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr );


	// ultra-convenience for fragments.
	void			AddFragmentBatch( vsFragment *fragment );
	// For fragments using instancing.
	// Note that the passed array of matrices must exist until the Draw phase ends!
	void			AddFragmentInstanceBatch( vsFragment *fragment, const vsMatrix4x4 *matrix, const vsColor *color, int instanceCount, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr);
	void			AddFragmentInstanceBatch( vsFragment *fragment, const vsMatrix4x4 *matrix, int instanceCount );
	void			AddFragmentInstanceBatch( vsFragment *fragment, vsRenderBuffer *matrixBuffer, vsRenderBuffer *colorBuffer = nullptr, vsShaderValues *values = nullptr, vsShaderOptions *options = nullptr);

	// For stuff which really doesn't want to keep its display list around, call this to get a temporary display list.
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, int size );
	vsDisplayList *	MakeTemporaryBatchList( vsMaterial *material, const vsMatrix4x4 &matrix, int size );

	// BAD, BAD CODE.  :)  Ought to get rid of this.
	vsDisplayList * GetGenericList() { return m_genericList; }

	bool WouldMaterialBeHidden( vsMaterial *material ) const;

	vsRenderQueueStage * GetStage( int i = 0 );
};

#endif // VS_SCENE_DRAW_H

