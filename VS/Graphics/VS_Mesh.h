/*
 *  VS_Mesh.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 31/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MESH_H
#define VS_MESH_H

#include "VS_RenderBuffer.h"

class vsColor;
class vsDisplayList;
class vsFragment;
class vsMaterial;
class vsModel;
class vsVector2D;
class vsVector3D;

class vsMesh
{
	vsRenderBuffer		*m_vertexBuffer;
	vsRenderBuffer		*m_normalBuffer;
	vsRenderBuffer		*m_texelBuffer;
	vsRenderBuffer		*m_colorBuffer;

	vsRenderBuffer		*m_interleavedBuffer;

	int				m_vertexCount;

	vsRenderBuffer::Type	m_type;

	vsMaterial		**m_triangleListMaterial;
	vsRenderBuffer	**m_triangleListBuffer;
	int				*m_triangleListIndexCount;
	int				m_triangleListCount;
	int				m_maxTriangleListCount;

	void			ResizeTriangleList(int list, int stripLength);


public:

	vsMesh( int vertexCount, int triangleListCount, vsRenderBuffer::Type type = vsRenderBuffer::Type_Static );
	~vsMesh();

	void			SetVertexArray( const vsVector3D *array, int arrayCount );
	void			SetNormalArray( const vsVector3D *array, int arrayCount );
	void			SetColorArray( const vsColor *array, int arrayCount );
	void			SetTexelArray( const vsVector2D *array, int arrayCount );

	void			SetVertex(int i, const vsVector3D &v);
	void			SetNormal(int i, const vsVector3D &n);
	void			SetColor(int i, const vsColor &c);
	void			SetTexel(int i, const vsVector2D &t);

	vsVector3D		GetVertex(int i);

	int				GetTriangleCount(int list);
	void			GetTriangle(int list, int triangle, vsVector3D *a, vsVector3D *b, vsVector3D *c);

	int				GetTriangleListCount() { return m_triangleListCount; }
	void			SetTriangleListMaterial( int list, vsMaterial *material );
	void			SetTriangleListTriangleCount( int list, int triangleCount );
	void			AddTriangleToList( int list, int triA, int triB, int triC);

	vsFragment*		MakeFragment( int material );
	void			WriteFragments( vsModel *model );

	void			Bake();

	void			Draw( vsDisplayList *list );
	void			DrawNormals( vsDisplayList *list );
};

#endif // VS_MESH_H

