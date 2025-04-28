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

#include "VS/Utils/VS_Array.h"
#include "VS/Math/VS_Vector.h"
#include "VS/Graphics/VS_RenderBuffer.h"
#include "VS/Graphics/VS_Model.h"
class vsVertexArrayObject;

class vsLineBuilder2D
{
	float m_epsilonSq;

	enum End
	{
		End_Start,
		End_End
	};
	class strip
	{
	public:
		vsArray<vsVector2D> vert;
		bool loop;
		strip(): loop(false) {}
	};
	vsArray<strip> m_strip;

	struct touches
	{
		int stripId;
		End end;
	};
	touches touchesStripId( const vsVector2D& v );
	void AddVertToStrip( const vsVector2D& v, int stripId, End whichEnd );
	bool isSameAs( const vsVector2D& a, const vsVector2D& b );

public:
	vsLineBuilder2D( float epsilon = 0.01f );

	void AddLineSegment( const vsVector2D& from, const vsVector2D& to );
	void CloseLoops();

	vsFragment *Bake( const vsString& material, float width );
};

class vsMaterial;
class vsRenderQueue;

// for a 2D line set, we can just spit out a static fragment.  Not like
// 3D where we need to check the camera position and recreate our
// renderable model each frame.
vsFragment *vsLineStrip2D( const vsString &material, const vsVector2D *array, const vsColor *carray, int count, float width, bool loop );
vsFragment *vsLineList2D( const vsString &material, const vsVector2D *array, const vsColor *carray, int count, float width );

// Old function signature for backwards-compatibility
vsFragment *vsLineStrip2D( const vsString &material, const vsVector2D *array, int count, float width, bool loop );
vsFragment *vsLineList2D( const vsString &material, const vsVector2D *array, int count, float width );

// utility functions for writing data into render buffers, instead of returning whole fragments.
// The returned VBO and IBO are intended to be rendered as a TRIANGLE LIST.
void vsLineStrip2D_VBO_IBO( vsRenderBuffer *vbo, vsRenderBuffer *ibo, const vsVector2D *array, const vsColor *carray, int count, float width, bool loop, bool finalise = true );
void vsLineStrip2D_VBO_IBO_Append( vsRenderBuffer *vbo, vsRenderBuffer *ibo, const vsVector2D *array, const vsColor *carray, int count, float width, bool loop, bool finalise = true );


void vsMakeOutlineFromLineStrip2D( vsArray<vsVector2D> *result, const vsVector2D *array, int count, float width, bool loop );

vsFragment *vsLineStrip3D( const vsString &material, const vsVector3D *array, int count, float width, bool loop, const vsColor *color = nullptr, float texScale = 1.f );
vsFragment *vsLineList3D( const vsString &material, const vsVector3D *array, int count, float width, const vsColor *color = nullptr, float texScale = 1.f );

vsFragment *vsLineStrip3D( const vsString &material, const vsVector3D *array, const vsColor *carray, int count, float width, bool loop );
vsFragment *vsLineList3D( const vsString &material, const vsVector3D *array, const vsColor *carray, int count, float width );

class vsLines3D: public vsModel
{
	class Strip;
	Strip **m_strip;
	int m_stripCount;
	int m_maxStripCount;

	static float s_widthFactor;
	float m_leftWidth;
	float m_rightWidth;
	float m_texScale;
	bool m_widthInScreenspace;

	vsRenderBuffer *m_vertices;
	vsRenderBuffer *m_indices;

	vsRenderBuffer m_av;
	vsRenderBuffer m_ai;
	vsRenderBuffer m_bv;
	vsRenderBuffer m_bi;

	int m_vertexCursor;
	int m_indexCursor;

	vsVector3D m_constantViewDirection;
	bool m_useConstantViewDirection;

	void DrawStrip( vsRenderQueue *queue, Strip *strip );

	size_t GetFinalVertexCount();
	size_t GetFinalIndexCount();

public:
	vsLines3D( int maxStrips, float width = 1.f, bool screenSpaceWidth = true );
	~vsLines3D();

	void SetLeftRightWidths(float left, float right) { m_leftWidth = left; m_rightWidth = right; }
	void SetWidth(float width) { SetLeftRightWidths( width * 0.5f, width * 0.5f ); }
	void SetTexScale(float scale) { m_texScale = scale; }

	void SetConstantViewDirection( const vsVector3D& direction );

	void Clear();
	void AddLine( const vsVector3D &a, const vsVector3D &b );
	void AddStrip( vsVector3D *array, int arraySize ) { AddStrip(array, nullptr, arraySize); }
	void AddStrip( vsVector3D *array, vsColor *carray, int arraySize );
	void AddLoop( vsVector3D *array, int arraySize ) { AddLoop(array, nullptr, arraySize); }
	void AddLoop( vsVector3D *array, vsColor *carray, int arraySize );

	void DynamicDraw( vsRenderQueue *queue );

	static void SetWidthFactor(float factor) { s_widthFactor = factor; }
};

#endif // VS_LINES_H

