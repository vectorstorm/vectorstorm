/*
 *  VS_Lines.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/05/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Lines.h"
#include "VS_RenderQueue.h"
#include "VS_Scene.h"
#include "VS_Camera.h"

float vsLines::s_widthFactor = 1.f;

class vsLines::Strip
{
public:
	vsVector3D *m_vertex;
	int m_length;

	Strip( vsVector3D *array, int length ):
		m_vertex( new vsVector3D[length] ),
		m_length( length )
	{
		for ( int i = 0; i < length; i++ )
		{
			m_vertex[i] = array[i];
		}
	}
	~Strip()
	{
		vsDeleteArray( m_vertex );
	}
};

vsLines::vsLines( int maxStrips, float width ):
	m_strip( new Strip*[maxStrips] ),
	m_stripCount( 0 ),
	m_maxStripCount( maxStrips ),
	m_width( width ),
	m_vertices( vsRenderBuffer::Type_Stream ),
	m_indices( vsRenderBuffer::Type_Stream )
{
}

vsLines::~vsLines()
{
	Clear();
	vsDeleteArray( m_strip );
}

void
vsLines::Clear()
{
	for ( int i = 0; i < m_stripCount; i++ )
	{
		vsDelete( m_strip[i] );
	}
	m_stripCount = 0;
}

void
vsLines::AddLine( vsVector3D &a, vsVector3D &b )
{
	vsVector3D vert[2] = { a, b };
	AddStrip(vert, 2);
}

void
vsLines::AddStrip( vsVector3D *array, int arraySize )
{
	vsAssert( m_stripCount < m_maxStripCount, "Too many strips in vsLines" );
	int i = m_stripCount++;

	m_strip[i] = new Strip(array, arraySize);
}

size_t
vsLines::GetFinalVertexCount()
{
	size_t result = 0;
	for ( int i = 0; i < m_stripCount; i++ )
	{
		// we're going to emit two vertices per strip vertex.
		result += m_strip[i]->m_length * 2;
	}
	return result;
}

size_t
vsLines::GetFinalIndexCount()
{
	size_t result = 0;
	for ( int i = 0; i < m_stripCount; i++ )
	{
		// we're going to emit six indices for each quad.  We're going
		// to emit one quad for each strip vertex, except for the last one
		// of each strip.  ('n' vertices means 'n-1' quads)
		result += (m_strip[i]->m_length-1) * 6;
	}
	return result;
}

void
vsLines::DrawWithMaterial( vsRenderQueue *queue, vsMaterial *material )
{
	m_vertexCursor = 0;
	m_indexCursor = 0;

	size_t vertexCount = GetFinalVertexCount();
	size_t indexCount = GetFinalIndexCount();
	m_vertices.ResizeArray( sizeof(vsVector3D) * vertexCount );
	m_indices.ResizeArray( sizeof(uint16_t) * indexCount );

	for ( int i = 0; i < m_stripCount; i++ )
	{
		DrawStripWithMaterial( queue, m_strip[i], material );
	}

	m_vertices.BakeArray();
	m_indices.BakeArray();

	vsDisplayList *	list = queue->MakeTemporaryBatchList( material, queue->GetMatrix(), 1024 );
	list->VertexBuffer(&m_vertices);
	list->TriangleListBuffer(&m_indices);
	list->ClearBuffers();
}

void
vsLines::DrawStripWithMaterial( vsRenderQueue *queue, Strip *strip, vsMaterial *material )
{
	const vsVector3D &camPos = (queue->GetWorldToViewMatrix() * queue->GetMatrix()).w;
	// const vsVector3D &camPos = queue->GetScene()->GetCamera3D()->GetPosition();
	// const vsVector3D &camPos = vsVector3D::Zero;

	vsVector3D *va = m_vertices.GetVector3DArray();
	uint16_t *ia = m_indices.GetIntArray();

	for ( int i = 0; i < strip->m_length; i++ )
	{
		vsVector3D dirOfTravel;

		int midI = i;
		int preI = midI-1;
		int postI = midI+1;

		if ( postI >= strip->m_length )
		{
			postI = strip->m_length-1;
		}
		if ( preI < 0 )
		{
			preI = 0;
		}

		dirOfTravel = ( strip->m_vertex[postI] - strip->m_vertex[preI] );

		if ( dirOfTravel.SqLength() > 0.f )
		{
			dirOfTravel.Normalise();
		}
		else
		{
			dirOfTravel = vsVector3D::XAxis;
		}
		vsVector3D cameraForward = strip->m_vertex[midI] - camPos;
		if ( cameraForward.SqLength() > 0.f )
		{
			cameraForward.Normalise();
		}
		else
		{
			cameraForward = vsVector3D::ZAxis;
		}
		vsVector3D bestOffsetDirection = dirOfTravel.Cross( cameraForward );
		if ( bestOffsetDirection.SqLength() < 0.001f )
		{
			bestOffsetDirection = vsVector3D::XAxis;
		}
		bestOffsetDirection.Normalise();
		//		bestOffsetDirection = dirOfTravel.Cross( bestOffsetDirection );

		// od is "offset distance"
		float od = s_widthFactor * m_width * 0.5f;

		va[m_vertexCursor] = strip->m_vertex[midI] + bestOffsetDirection * od;
		va[m_vertexCursor+1] = strip->m_vertex[midI] - bestOffsetDirection * od;

		if ( postI != midI ) // not at the end of the strip
		{
			ia[m_indexCursor] = m_vertexCursor;
			ia[m_indexCursor+1] = m_vertexCursor+1;
			ia[m_indexCursor+2] = m_vertexCursor+2;
			ia[m_indexCursor+3] = m_vertexCursor+2;
			ia[m_indexCursor+4] = m_vertexCursor+1;
			ia[m_indexCursor+5] = m_vertexCursor+3;
			m_indexCursor += 6;
		}
		m_vertexCursor += 2;
	}
}

