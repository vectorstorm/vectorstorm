/*
 *  VS_ModelInstanceGroup.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 21/03/2016
 *  Copyright 2016 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_MODELINSTANCEGROUP_H
#define VS_MODELINSTANCEGROUP_H

#include "VS/Math/VS_Box.h"
#include "VS/Math/VS_Matrix.h"
#include "VS/Graphics/VS_Color.h"
#include "VS/Graphics/VS_Entity.h"
#include "VS/Graphics/VS_RenderBuffer.h"
#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_ArrayStore.h"

class vsModel;
struct vsModelInstance;
class vsModelInstanceGroup;
class vsShaderValues;

#define INSTANCED_MODEL_USES_LOCAL_BUFFER

class vsModelInstanceLodGroup : public vsEntity
{
	vsModelInstanceGroup *m_group; // what group am a part of
	vsModel *m_model;
	size_t m_lodLevel;
	vsShaderValues *m_values; // TEMP:  For testing
	vsArray<vsMatrix4x4> m_matrix;
	vsArray<vsColor> m_color;
	vsArray<int> m_matrixInstanceId;
	vsArray<vsModelInstance*> m_instance;
#ifdef INSTANCED_MODEL_USES_LOCAL_BUFFER
	vsRenderBuffer m_matrixBuffer;
	vsRenderBuffer m_colorBuffer;
	bool m_bufferIsDirty;
#endif // INSTANCED_MODEL_USES_LOCAL_BUFFER
public:

	vsModelInstanceLodGroup( vsModelInstanceGroup *group, vsModel *model, size_t lodLevel );
	virtual ~vsModelInstanceLodGroup();

	void TakeInstancesFromGroup( vsModelInstanceLodGroup *otherGroup );

	void SetShaderValues( vsShaderValues *values ) { m_values = values; }
	vsModelInstance * MakeInstance();		// create an instance of me.
	void AddInstance( vsModelInstance *instance );
	void RemoveInstance( vsModelInstance *instance );
	bool ContainsInstance( vsModelInstance *instance );
	void UpdateInstance( vsModelInstance *instance, bool show = true ); // must be called to change the matrix on this instance
	vsModel * GetModel() { return m_model; }

	// find the bounds of our matrix translations.
	void CalculateMatrixBounds( vsBox3D& out );

	virtual void Draw( vsRenderQueue *queue );
};

class vsModelInstanceGroup: public vsEntity
{
	vsModel *m_model;
	vsArrayStore<vsModelInstanceLodGroup> m_lod;
public:

	vsModelInstanceGroup( vsModel *model );

	void TakeInstancesFromGroup( vsModelInstanceGroup *otherGroup );
	void SetShaderValues( vsShaderValues *values );

	vsModelInstance * MakeInstance( int lod = 0 );		// create an instance of me.
	void AddInstance( vsModelInstance *instance );
	void RemoveInstance( vsModelInstance *instance );
	bool ContainsInstance( vsModelInstance *instance );
	void UpdateInstance( vsModelInstance *instance, bool show = true ); // must be called to change the matrix on this instance
	vsModel * GetModel();

	// find the bounds of our matrix translations.
	void CalculateMatrixBounds( vsBox3D& out );

	virtual void Draw( vsRenderQueue *queue );
};

#endif // VS_MODELINSTANCEGROUP_H

