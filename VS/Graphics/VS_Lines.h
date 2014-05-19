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

class vsMaterial;
class vsRenderQueue;

class vsLines
{
	class Strip;
	Strip **m_strip;
	int m_stripCount;
	int m_maxStripCount;

	static float s_widthFactor;
	float m_width;

	vsVector3D *m_va;

	vsRenderBuffer m_vertices;
	vsRenderBuffer m_indices;
	int m_vertexCursor;
	int m_indexCursor;

	void DrawStripWithMaterial( vsRenderQueue *queue, Strip *strip, vsMaterial *material );

	size_t GetFinalVertexCount();
	size_t GetFinalIndexCount();

public:
	vsLines( int maxStrips, float width = 1.f );
	~vsLines();

	void SetWidth(float width);

	void Clear();
	void AddLine( vsVector3D &a, vsVector3D &b );
	void AddStrip( vsVector3D *array, int arraySize );
	void AddLoop( vsVector3D *array, int arraySize );

	void DrawWithMaterial( vsRenderQueue *queue, vsMaterial *material );

	// vsFragment * BakeFragment();

	static void SetWidthFactor(float factor) { s_widthFactor = factor; }
};

#endif // VS_LINES_H

