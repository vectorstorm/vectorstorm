/*
 *  VS_Lines.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/05/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_LINES_H
#define VS_LINES_H

#include "VS/Math/VS_Vector.h"
#include "VS/Graphics/VS_RenderBuffer.h"
#include "VS/Graphics/VS_Model.h"

class vsMaterial;
class vsRenderQueue;

// for a 2D line set, we can just spit out a static fragment.  Not like
// 3D where we need to check the camera position and recreate our
// renderable model each frame.
vsFragment *vsLineStrip2D( const vsString &material, vsVector2D *array, int count, float width, bool loop );
vsFragment *vsLineList2D( const vsString &material, vsVector2D *array, int count, float width );

class vsLines3D: public vsModel
{
	class Strip;
	Strip **m_strip;
	int m_stripCount;
	int m_maxStripCount;

	static float s_widthFactor;
	float m_width;
	bool m_widthInScreenspace;

	vsVector3D *m_va;

	vsRenderBuffer m_vertices;
	vsRenderBuffer m_indices;
	int m_vertexCursor;
	int m_indexCursor;

	void DrawStrip( vsRenderQueue *queue, Strip *strip );

	size_t GetFinalVertexCount();
	size_t GetFinalIndexCount();

public:
	vsLines3D( int maxStrips, float width = 1.f, bool screenSpaceWidth = true );
	~vsLines3D();

	void SetWidth(float width);

	void Clear();
	void AddLine( vsVector3D &a, vsVector3D &b );
	void AddStrip( vsVector3D *array, int arraySize );
	void AddLoop( vsVector3D *array, int arraySize );

	void DynamicDraw( vsRenderQueue *queue );

	static void SetWidthFactor(float factor) { s_widthFactor = factor; }
};

#endif // VS_LINES_H

