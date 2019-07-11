/*
 *  VS_DynamicBatch.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 10/07/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_DYNAMICBATCH_H
#define VS_DYNAMICBATCH_H

#include "VS/Graphics/VS_Fragment.h"
#include "VS/Graphics/VS_RenderBuffer.h"
#include "VS/Math/VS_Matrix.h"
class vsDisplayList;

// Internal class;  not to be exposed through VS_Headers.h
//
// Idea here is that a dynamic batch can consolidate multiple
// "simple" vsFragments into a single VBO and single IBO, and
// then issue them all in a single draw by calling Draw().
//
// These dynamic batch objects are owned by a singleton
// vsDynamcicBatchManager, which is notified after each
// frame is drawn, so these dynamic batch items return to
// the global pool.

class vsDynamicBatch
{
	vsRenderBuffer m_vbo;
	vsRenderBuffer m_ibo;

	void AddToBatch_Internal( vsRenderBuffer *vbo, vsRenderBuffer *ibo, const vsMatrix4x4& mat, vsFragment::SimpleType type, bool first );
public:
	vsDynamicBatch();

	void Reset();
	static bool Supports( vsRenderBuffer::ContentType type );
	bool CanFitVertices( int vertexCount ) const;

	void StartBatch( vsRenderBuffer *vbo, vsRenderBuffer *ibo, const vsMatrix4x4& mat, vsFragment::SimpleType type );
	void AddToBatch( vsRenderBuffer *vbo, vsRenderBuffer *ibo, const vsMatrix4x4& mat, vsFragment::SimpleType type );
	void Draw( vsDisplayList * list );
};

#endif // VS_DYNAMICBATCH_H

