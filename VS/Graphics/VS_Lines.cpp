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

float vsLines3D::s_widthFactor = 1.f;

class vsLines3D::Strip
{
public:
	vsVector3D *m_vertex;
	int m_length;
	bool m_loop;

	Strip( vsVector3D *array, int length ):
		m_vertex( new vsVector3D[length] ),
		m_length( length ),
		m_loop(false)
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

vsLines3D::vsLines3D( int maxStrips, float width ):
	m_strip( new Strip*[maxStrips] ),
	m_stripCount( 0 ),
	m_maxStripCount( maxStrips ),
	m_width( width ),
	m_vertices( vsRenderBuffer::Type_Stream ),
	m_indices( vsRenderBuffer::Type_Stream )
{
}

vsLines3D::~vsLines3D()
{
	Clear();
	vsDeleteArray( m_strip );
}

void
vsLines3D::Clear()
{
	for ( int i = 0; i < m_stripCount; i++ )
	{
		vsDelete( m_strip[i] );
	}
	m_stripCount = 0;
}

void
vsLines3D::AddLine( vsVector3D &a, vsVector3D &b )
{
	vsVector3D vert[2] = { a, b };
	AddStrip(vert, 2);
}

void
vsLines3D::AddStrip( vsVector3D *array, int arraySize )
{
	vsAssert( m_stripCount < m_maxStripCount, "Too many strips in vsLines3D" );
	int i = m_stripCount++;

	m_strip[i] = new Strip(array, arraySize);
}

void
vsLines3D::AddLoop( vsVector3D *array, int arraySize )
{
	vsAssert( m_stripCount < m_maxStripCount, "Too many strips in vsLines3D" );
	int i = m_stripCount++;

	m_strip[i] = new Strip(array, arraySize);
	m_strip[i]->m_loop = true;
}

size_t
vsLines3D::GetFinalVertexCount()
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
vsLines3D::GetFinalIndexCount()
{
	size_t result = 0;
	for ( int i = 0; i < m_stripCount; i++ )
	{
		// we're going to emit six indices for each quad.  We're going
		// to emit one quad for each strip vertex, except for the last one
		// of each strip.  ('n' vertices means 'n-1' quads)
		result += (m_strip[i]->m_length-1) * 6;
		if ( m_strip[i]->m_loop )
			result += 6;	// six more indices if we're looping, as we connect end->start
	}
	return result;
}

void
vsLines3D::DynamicDraw( vsRenderQueue *queue )
{
	m_vertexCursor = 0;
	m_indexCursor = 0;

	size_t vertexCount = GetFinalVertexCount();
	size_t indexCount = GetFinalIndexCount();
	m_vertices.ResizeArray( sizeof(vsVector3D) * vertexCount );
	m_indices.ResizeArray( sizeof(uint16_t) * indexCount );

	for ( int i = 0; i < m_stripCount; i++ )
	{
		DrawStrip( queue, m_strip[i] );
	}

	m_vertices.BakeArray();
	m_indices.BakeArray();

	vsDisplayList *	list = queue->MakeTemporaryBatchList( GetMaterial(), queue->GetMatrix(), 1024 );
	list->VertexBuffer(&m_vertices);
	list->TriangleListBuffer(&m_indices);
	list->ClearBuffers();
}

void
vsLines3D::DrawStrip( vsRenderQueue *queue, Strip *strip )
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

		// float widthHere = m_width;

		if ( postI >= strip->m_length )
		{
			if ( strip->m_loop )
				postI = 0;
			else
				postI = strip->m_length-1;
		}
		if ( preI < 0 )
		{
			if ( strip->m_loop )
				preI = strip->m_length-1;
			else
				preI = 0;
		}

		vsVector3D cameraForward = strip->m_vertex[midI] - camPos;
		cameraForward.NormaliseSafe();

		vsVector3D dirOfTravelPre = strip->m_vertex[midI] - strip->m_vertex[preI];
		vsVector3D dirOfTravelPost = strip->m_vertex[postI] - strip->m_vertex[midI];
		float distanceOfTravelPre = dirOfTravelPre.Length();
		float distanceOfTravelPost = dirOfTravelPost.Length();
		dirOfTravelPre.NormaliseSafe();
		dirOfTravelPost.NormaliseSafe();

		vsVector3D offsetPre = dirOfTravelPre.Cross( cameraForward );
		vsVector3D offsetPost = dirOfTravelPost.Cross( cameraForward );
		offsetPre.NormaliseSafe();
		offsetPost.NormaliseSafe();

		vsVector3D vertexPosition;
		if ( offsetPre != offsetPost )
		{
			vsVector3D closestPre, closestPost;
			vsVector3D insidePre = strip->m_vertex[preI] - (offsetPre * m_width);
			vsVector3D insidePost = strip->m_vertex[postI] - (offsetPost * m_width);

			vsSqDistanceBetweenLineSegments( insidePre,
					insidePre + dirOfTravelPre * (distanceOfTravelPre + 3.f * m_width),
					insidePost,
					insidePost - dirOfTravelPost * (distanceOfTravelPost + 3.f * m_width),
					&closestPre, &closestPost );

			vertexPosition = vsInterpolate(0.5f, closestPre, closestPost);
		}
		else
		{
			vertexPosition = strip->m_vertex[midI] - offsetPre * m_width;
		}

		va[m_vertexCursor] = vertexPosition;

		if ( offsetPre != offsetPost )
		{
			vsVector3D closestPre, closestPost;
			vsVector3D outsidePre = strip->m_vertex[preI] + (offsetPre * m_width);
			vsVector3D outsidePost = strip->m_vertex[postI] + (offsetPost * m_width);

			vsSqDistanceBetweenLineSegments( outsidePre,
					outsidePre + dirOfTravelPre * (distanceOfTravelPre + 3.f * m_width),
					outsidePost,
					outsidePost - dirOfTravelPost * (distanceOfTravelPost + 3.f * m_width),
					&closestPre, &closestPost );

			vertexPosition = vsInterpolate(0.5f, closestPre, closestPost);
		}
		else
		{
			vertexPosition = strip->m_vertex[midI] + offsetPre * m_width;
		}

		va[m_vertexCursor+1] = vertexPosition;

		if ( i != strip->m_length - 1 ) // not at the end of the strip
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

	if ( strip->m_loop )
	{
		// and join up the end to the start.
		ia[m_indexCursor] = m_vertexCursor-2;
		ia[m_indexCursor+1] = m_vertexCursor-1;
		ia[m_indexCursor+2] = 0;
		ia[m_indexCursor+3] = 0;
		ia[m_indexCursor+4] = m_vertexCursor-1;
		ia[m_indexCursor+5] = 1;
		m_indexCursor += 6;
	}
}

