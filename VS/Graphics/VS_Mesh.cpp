/*
 *  VS_Mesh.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 31/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Mesh.h"

#include "VS_Color.h"
#include "VS_DisplayList.h"
#include "VS_Fragment.h"
#include "VS_Model.h"
#include "VS_RenderBuffer.h"

vsMesh::vsMesh( int vertexCount, int maxTriangleListCount, vsRenderBuffer::Type type )
{
	m_triangleListCount = 0;
	m_maxTriangleListCount = maxTriangleListCount;
	m_vertexCount = vertexCount;
	m_type = type;

	m_vertexBuffer = NULL;
	m_normalBuffer = NULL;
	m_texelBuffer = NULL;
	m_colorBuffer = NULL;
	m_interleavedBuffer = NULL;

	m_triangleListBuffer = new vsRenderBuffer *[maxTriangleListCount];
	m_triangleListMaterial = new vsMaterial *[maxTriangleListCount];
	m_triangleListIndexCount = new int[maxTriangleListCount];

	for ( int i = 0; i < maxTriangleListCount; i++ )
	{
		m_triangleListBuffer[i] = NULL;
		m_triangleListMaterial[i] = NULL;
		m_triangleListIndexCount[i] = 0;
	}
}

vsMesh::~vsMesh()
{
	vsDelete( m_vertexBuffer );
	vsDelete( m_normalBuffer );
	vsDelete( m_texelBuffer );
	vsDelete( m_colorBuffer );
	vsDelete( m_interleavedBuffer );

	for ( int i = 0; i < m_triangleListCount; i++ )
	{
		if ( m_triangleListBuffer[i] )
		{
			vsDelete( m_triangleListBuffer[i] );
		}
		if ( m_triangleListMaterial[i] )
		{
			vsDelete( m_triangleListMaterial[i] );
		}
	}

	vsDeleteArray( m_triangleListMaterial );
	vsDeleteArray( m_triangleListIndexCount );
	vsDeleteArray( m_triangleListBuffer );
}

void
vsMesh::SetVertex(int i, const vsVector3D &v)
{
	vsAssert(i>=0 && i<m_vertexCount, "Error:  tried to set illegal vertex??");
	if ( m_vertexBuffer == NULL )
	{
		m_vertexBuffer = new vsRenderBuffer(m_type);
		m_vertexBuffer->SetVector3DArraySize(m_vertexCount);
	}
	m_vertexBuffer->GetVector3DArray()[i] = v;
}

void
vsMesh::SetVertexArray( const vsVector3D *array, int arrayCount )
{
	if ( m_vertexBuffer == NULL )
	{
		m_vertexBuffer = new vsRenderBuffer(m_type);
	}
	m_vertexBuffer->SetArray( array, arrayCount );
}

void
vsMesh::SetNormalArray( const vsVector3D *array, int arrayCount )
{
	if ( m_normalBuffer == NULL )
	{
		m_normalBuffer = new vsRenderBuffer(m_type);
	}
	m_normalBuffer->SetArray( array, arrayCount );
}

void
vsMesh::SetColorArray( const vsColor *array, int arrayCount )
{
	if ( m_colorBuffer == NULL )
	{
		m_colorBuffer = new vsRenderBuffer(m_type);
	}
	m_colorBuffer->SetArray( array, arrayCount );
}

void
vsMesh::SetTexelArray( const vsVector2D *array, int arrayCount )
{
	if ( m_texelBuffer == NULL )
	{
		m_texelBuffer = new vsRenderBuffer(m_type);
	}
	m_texelBuffer->SetArray( array, arrayCount );
}

vsVector3D
vsMesh::GetVertex(int i)
{
	vsAssert(i>=0 && i<m_vertexCount, "Error:  tried to get illegal vertex??");
	return m_vertexBuffer->GetVector3DArray()[i];
}

void
vsMesh::SetNormal(int i, const vsVector3D &n)
{
	vsAssert(i>=0 && i<m_vertexCount, "Error:  tried to set illegal vertex??");
	if ( m_normalBuffer == NULL )
	{
		m_normalBuffer = new vsRenderBuffer(m_type);
		m_normalBuffer->SetVector3DArraySize(m_vertexCount);
	}
	m_normalBuffer->GetVector3DArray()[i] = n;
}

void
vsMesh::SetColor(int i, const vsColor &c)
{
	vsAssert(i>=0 && i<m_vertexCount, "Error:  tried to set illegal vertex??");
	if ( m_colorBuffer == NULL )
	{
		m_colorBuffer = new vsRenderBuffer(m_type);
		m_colorBuffer->SetColorArraySize(m_vertexCount);
	}
	m_colorBuffer->GetColorArray()[i] = c;
}

void
vsMesh::SetTexel(int i, const vsVector2D &t)
{
	vsAssert(i>=0 && i<m_vertexCount, "Error:  tried to set illegal vertex??");
	if ( m_texelBuffer == NULL )
	{
		m_texelBuffer = new vsRenderBuffer(m_type);
		m_texelBuffer->SetVector2DArraySize(m_vertexCount);
	}
	m_texelBuffer->GetVector2DArray()[i] = t;
}

void
vsMesh::AddTriangleToList(int list, int a, int b, int c)
{
	vsAssert( list >= 0 && list < m_triangleListCount, "Unknown triangle list requested??" );
	vsAssert( m_triangleListBuffer[list]->GetIntArraySize() > m_triangleListIndexCount[list]+2, "Overflowing triangle list??" );

	uint16_t *array = m_triangleListBuffer[list]->GetIntArray();
	int triangleCount = m_triangleListIndexCount[list];

	array[triangleCount++] = a;
	array[triangleCount++] = b;
	array[triangleCount++] = c;

	m_triangleListIndexCount[list] = triangleCount;
}

void
vsMesh::SetTriangleListMaterial( int list, vsMaterial *material )
{
	vsAssert( list >= 0 && list < m_triangleListCount, "Unknown triangle list requested??" );

	vsDelete( m_triangleListMaterial[list] );
	m_triangleListMaterial[list] = new vsMaterial(material);
}

void
vsMesh::SetTriangleListTriangleCount( int list, int count )
{
	ResizeTriangleList( list, count );
}

/*
void
vsMesh::AddTriangle(int a, int b, int c, vsMaterial *material)
{
	for ( int i = 0; i < m_triangleListCount; i++ )
	{
		if ( m_triangleListMaterial[i]->GetResource() == material->GetResource() )
		{
			return AddTriangleToList( a, b, c, i );
		}
	}

	int id = m_triangleListCount++;

	// no existing list for this material.  Start a new one!
	m_triangleListMaterial[id] = new vsMaterial(material);
	ResizeTriangleList(id, 1);
	AddTriangleToList( a, b, c, id );
}*/

void
vsMesh::ResizeTriangleList(int listId, int length)
{
	vsAssert(listId>=0 && listId<m_maxTriangleListCount, "Error:  tried to use illegal triangle list??");

	m_triangleListCount = vsMax(m_triangleListCount, listId+1);
	if ( m_triangleListBuffer[listId] == NULL )
	{
		m_triangleListBuffer[listId] = new vsRenderBuffer(m_type);
	}
	m_triangleListBuffer[listId]->SetIntArraySize(length*3);
	m_triangleListIndexCount[listId] = 0;
}

vsFragment *
vsMesh::MakeFragment(int listId)
{
	vsFragment *fragment = new vsFragment;
	vsRenderBuffer *buffer;

	if ( m_vertexBuffer && m_normalBuffer && m_colorBuffer && m_texelBuffer )
	{
		buffer = new vsRenderBuffer;

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PCNT *array = new vsRenderBuffer::PCNT[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].normal = m_normalBuffer->GetVector3DArray()[i];
			array[i].color = m_colorBuffer->GetColorArray()[i];
			array[i].texel = m_texelBuffer->GetVector2DArray()[i];
		}
		buffer->SetArray(array, size);
		buffer->BakeArray();
		vsDeleteArray( array );
	}
	else if ( m_vertexBuffer && m_normalBuffer && m_colorBuffer )
	{
		buffer = new vsRenderBuffer;

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PCN *array = new vsRenderBuffer::PCN[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].normal = m_normalBuffer->GetVector3DArray()[i];
			array[i].color = m_colorBuffer->GetColorArray()[i];
		}
		buffer->SetArray(array, size);
		buffer->BakeArray();
		vsDeleteArray( array );
	}
	else if ( m_vertexBuffer && m_normalBuffer && m_texelBuffer )
	{
		buffer = new vsRenderBuffer;

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PNT *array = new vsRenderBuffer::PNT[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].normal = m_normalBuffer->GetVector3DArray()[i];
			array[i].texel = m_texelBuffer->GetVector2DArray()[i];
		}
		buffer->SetArray(array, size);
		buffer->BakeArray();
		vsDeleteArray( array );
	}
	else if ( m_vertexBuffer && m_normalBuffer )
	{
		buffer = new vsRenderBuffer;

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PN *array = new vsRenderBuffer::PN[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].normal = m_normalBuffer->GetVector3DArray()[i];
		}
		buffer->SetArray(array, size);
		buffer->BakeArray();
		vsDeleteArray( array );
	}
	else if ( m_vertexBuffer && m_texelBuffer )
	{
		buffer = new vsRenderBuffer;

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PT *array = new vsRenderBuffer::PT[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].texel = m_texelBuffer->GetVector2DArray()[i];
		}
		buffer->SetArray(array, size);
		buffer->BakeArray();
		vsDeleteArray( array );
	}
	else if ( m_vertexBuffer )
	{
		buffer = new vsRenderBuffer;
		int size = m_vertexBuffer->GetVector3DArraySize();

		buffer->SetArray( m_vertexBuffer->GetVector3DArray(), size );
		buffer->BakeArray();
	}
	else
	{
		vsAssert(0, "Illegal vsMesh format");
		return NULL;
	}

	fragment->AddBuffer(buffer);

	vsRenderBuffer *indexBuffer = new vsRenderBuffer;
	int intsUsed = m_triangleListIndexCount[listId];

	indexBuffer->SetArray( m_triangleListBuffer[listId]->GetIntArray(), intsUsed );
	indexBuffer->BakeIndexArray();

	fragment->SetMaterial( m_triangleListMaterial[listId] );
	fragment->AddBuffer(indexBuffer);

	vsAssert( m_triangleListMaterial[listId], "No material set?" );

	vsDisplayList *list = new vsDisplayList(36);

	list->BindBuffer( buffer );
	list->TriangleListBuffer( indexBuffer );
	list->ClearArrays();

	fragment->SetDisplayList( list );

	return fragment;
}

void
vsMesh::WriteFragments( vsModel *model )
{
	for ( int i = 0; i < m_triangleListCount; i++ )
	{
		vsFragment *fragment = MakeFragment(i);
		model->AddFragment(fragment);
	}
}

void
vsMesh::Bake()
{
	if ( m_vertexBuffer && m_normalBuffer && m_colorBuffer && m_texelBuffer )
	{
		if ( m_interleavedBuffer == NULL )
		{
			m_interleavedBuffer = new vsRenderBuffer;
		}

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PCNT *array = new vsRenderBuffer::PCNT[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].normal = m_normalBuffer->GetVector3DArray()[i];
			array[i].color = m_colorBuffer->GetColorArray()[i];
			array[i].texel = m_texelBuffer->GetVector2DArray()[i];
		}
		m_interleavedBuffer->SetArray(array, size);
		m_interleavedBuffer->BakeArray();
		vsDeleteArray( array );
	}
	else if ( m_vertexBuffer && m_normalBuffer && m_colorBuffer )
	{
		if ( m_interleavedBuffer == NULL )
		{
			m_interleavedBuffer = new vsRenderBuffer;
		}

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PCN *array = new vsRenderBuffer::PCN[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].normal = m_normalBuffer->GetVector3DArray()[i];
			array[i].color = m_colorBuffer->GetColorArray()[i];
		}
		m_interleavedBuffer->SetArray(array, size);
		m_interleavedBuffer->BakeArray();
		vsDeleteArray( array );
	}
	else if ( m_vertexBuffer && m_normalBuffer )
	{
		if ( m_interleavedBuffer == NULL )
		{
			m_interleavedBuffer = new vsRenderBuffer;
		}

		int size = m_vertexBuffer->GetVector3DArraySize();

		vsRenderBuffer::PN *array = new vsRenderBuffer::PN[size];
		for ( int i = 0; i < size; i++ )
		{
			array[i].position = m_vertexBuffer->GetVector3DArray()[i];
			array[i].normal = m_normalBuffer->GetVector3DArray()[i];
		}
		m_interleavedBuffer->SetArray(array, size);
		m_interleavedBuffer->BakeArray();
		vsDeleteArray( array );
	}
	else
	{
		if ( m_vertexBuffer )
		{
			m_vertexBuffer->BakeArray();
		}
		if ( m_normalBuffer )
		{
			m_normalBuffer->BakeArray();
		}
		if ( m_colorBuffer )
		{
			m_colorBuffer->BakeArray();
		}
		if ( m_texelBuffer )
		{
			m_texelBuffer->BakeArray();
		}
	}

	for ( int i = 0; i < m_triangleListCount; i++ )
	{
		//int intArraySize = m_triangleListBuffer[i]->GetIntArraySize();
		int intsUsed = m_triangleListIndexCount[i];

		m_triangleListBuffer[i]->SetActiveSize( intsUsed * sizeof(uint16_t) );
		//vsAssert( intArraySize == intsUsed, "ERROR: Empty space in triangle list buffer??" );
		if ( m_triangleListIndexCount[i] > 0 )
		{
			m_triangleListBuffer[i]->BakeIndexArray();
		}
	}
}

void
vsMesh::Draw( vsDisplayList *list )
{
	vsMaterial *curMaterial = NULL;
	//bool materialSet = false;

	if ( m_interleavedBuffer )
	{
		list->BindBuffer( m_interleavedBuffer );
	}
	else
	{
		if ( m_normalBuffer )
		{
			list->NormalBuffer( m_normalBuffer );
		}
		if ( m_colorBuffer )
		{
			list->ColorBuffer( m_colorBuffer );
		}
		if ( m_texelBuffer )
		{
			list->TexelBuffer( m_texelBuffer );
		}
		if ( m_vertexBuffer )	// according to NVidia, we should bind our VertexBuffer LAST, for best performance.
			// (ref:  http://developer.nvidia.com/object/using_VBOs.html )
		{
			list->VertexBuffer( m_vertexBuffer );
		}
	}
	for ( int i = 0; i < m_triangleListCount; i++ )
	{
		if ( m_triangleListIndexCount[i] > 0 )
		{
			if ( m_triangleListMaterial[i] )
			{
				//materialSet = true;
				if ( curMaterial != m_triangleListMaterial[i] )
				{
					curMaterial = m_triangleListMaterial[i];
					list->SetMaterial( m_triangleListMaterial[i] );
				}
			}
			list->TriangleListBuffer( m_triangleListBuffer[i] );
		}
	}

	list->ClearArrays();
}

void
vsMesh::DrawNormals( vsDisplayList *list )
{
	if ( !m_normalBuffer || !m_vertexBuffer )	// if we don't have normals or vertices, don't draw!
	{
		return;
	}


	int vertexCount = m_vertexBuffer->GetVector3DArraySize();
	int normalVertexCount = vertexCount * 2;

	vsVector3D *v = new vsVector3D[ normalVertexCount ];
	int *ind = new int[ normalVertexCount ];

	for ( int i = 0; i < vertexCount; i++ )
	{
		int normalI = i<<1;
		int normalII = normalI+1;

		v[normalI] = m_vertexBuffer->GetVector3DArray()[i];
		v[normalII] = v[normalI] + m_normalBuffer->GetVector3DArray()[i];
		ind[normalI] = normalI;
		ind[normalII] = normalII;
	}

	list->VertexArray( v, normalVertexCount );
	list->LineList( ind, normalVertexCount );
	list->ClearVertexArray();

	vsDeleteArray(v);
	vsDeleteArray(ind);
}

int
vsMesh::GetTriangleCount( int list )
{
	vsAssert( list < m_triangleListCount, "Requested a triangle list which doesn't exist??" );

	return m_triangleListIndexCount[list] / 3;
}

void
vsMesh::GetTriangle( int list, int triangle, vsVector3D *a, vsVector3D *b, vsVector3D *c )
{
	vsAssert( triangle < GetTriangleCount( list ), "Requested a triangle which doesn't exist??" );

	uint16_t *vertexIndex = m_triangleListBuffer[list]->GetIntArray();
	vsVector3D *vertex = m_vertexBuffer->GetVector3DArray();

	int i = triangle * 3;

	*a = vertex[ vertexIndex[i] ];
	*b = vertex[ vertexIndex[i+1] ];
	*c = vertex[ vertexIndex[i+2] ];
}

