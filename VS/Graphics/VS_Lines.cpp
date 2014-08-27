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

vsFragment *vsLineList2D( const vsString &material, vsVector2D *point, int count, float width )
{
	size_t vertexCount = (count/2) * 4;
	size_t indexCount = (count/2) * 6;

	vsRenderBuffer::P *va = new vsRenderBuffer::P[vertexCount];
	uint16_t *ia = new uint16_t[indexCount];
	int vertexCursor = 0;
	int indexCursor = 0;
	float halfWidth = width * 0.5f;

	for ( int i = 0; i < count; i+=2 )
	{
		int preI = i;
		int postI = i+1;

		vsVector3D dirOfTravel = point[postI] - point[preI];

		dirOfTravel.NormaliseSafe();

		vsVector2D offsetPre( dirOfTravel.y, -dirOfTravel.x );
		offsetPre.NormaliseSafe();

		va[vertexCursor].position = point[preI] - (offsetPre * halfWidth);
		va[vertexCursor+1].position = point[preI] + (offsetPre * halfWidth);
		va[vertexCursor+2].position = point[postI] - (offsetPre * halfWidth);
		va[vertexCursor+3].position = point[postI] + (offsetPre * halfWidth);

		ia[indexCursor] = vertexCursor;
		ia[indexCursor+1] = vertexCursor+1;
		ia[indexCursor+2] = vertexCursor+2;
		ia[indexCursor+3] = vertexCursor+2;
		ia[indexCursor+4] = vertexCursor+1;
		ia[indexCursor+5] = vertexCursor+3;
		indexCursor += 6;
		vertexCursor += 4;
	}

	vsRenderBuffer* vertexBuffer = new vsRenderBuffer( vsRenderBuffer::Type_Static );
	vsRenderBuffer* indexBuffer = new vsRenderBuffer( vsRenderBuffer::Type_Static );
	vertexBuffer->SetArray(va, vertexCursor);
	indexBuffer->SetArray(ia, indexCursor);
	vsFragment *fragment = new vsFragment;
	vsDisplayList *dlist = new vsDisplayList(128);
	dlist->BindBuffer(vertexBuffer);
	dlist->TriangleListBuffer(indexBuffer);
	dlist->ClearArrays();
	fragment->SetDisplayList(dlist);
	fragment->SetMaterial( material );
	fragment->AddBuffer(vertexBuffer);
	fragment->AddBuffer(indexBuffer);

	vsDeleteArray(va);
	vsDeleteArray(ia);

	return fragment;
}

vsFragment *vsLineStrip2D( const vsString& material, vsVector2D *point, int count, float width, bool loop )
{
	size_t vertexCount = count * 2;
	size_t indexCount = count * 6;

	float halfWidth = width * 0.5f;

	vsRenderBuffer::P *va = new vsRenderBuffer::P[vertexCount];
	uint16_t *ia = new uint16_t[indexCount];
	int vertexCursor = 0;
	int indexCursor = 0;

	for ( int i = 0; i < count; i++ )
	{
		vsVector3D dirOfTravel;

		int midI = i;
		int preI = midI-1;
		int postI = midI+1;

		// float widthHere = m_width;

		if ( postI >= count )
		{
			if ( loop )
				postI = 0;
			else
				postI = count-1;
		}
		if ( preI < 0 )
		{
			if ( loop )
				preI = count-1;
			else
				preI = 0;
		}

		vsVector2D dirOfTravelPre = point[midI] - point[preI];
		vsVector2D dirOfTravelPost = point[postI] - point[midI];
		float distanceOfTravelPre = dirOfTravelPre.Length();
		float distanceOfTravelPost = dirOfTravelPost.Length();
		dirOfTravelPre.NormaliseSafe();
		dirOfTravelPost.NormaliseSafe();

		vsVector2D offsetPre( dirOfTravelPre.y, -dirOfTravelPre.x );
		vsVector2D offsetPost( dirOfTravelPost.y, -dirOfTravelPost.x );
		offsetPre.NormaliseSafe();
		offsetPost.NormaliseSafe();

		vsVector3D vertexPosition;
		if ( offsetPre != offsetPost )
		{
			vsVector3D closestPre, closestPost;
			vsVector3D insidePre = point[preI] - (offsetPre * halfWidth);
			vsVector3D insidePost = point[postI] - (offsetPost * halfWidth);

			vsSqDistanceBetweenLineSegments( insidePre,
					insidePre + dirOfTravelPre * (distanceOfTravelPre + 3.f * halfWidth),
					insidePost,
					insidePost - dirOfTravelPost * (distanceOfTravelPost + 3.f * halfWidth),
					&closestPre, &closestPost );

			vertexPosition = vsInterpolate(0.5f, closestPre, closestPost);
		}
		else
		{
			vertexPosition = point[midI] - offsetPre * halfWidth;
		}

		va[vertexCursor].position = vertexPosition;

		if ( offsetPre != offsetPost )
		{
			vsVector3D closestPre, closestPost;
			vsVector3D outsidePre = point[preI] + (offsetPre * halfWidth);
			vsVector3D outsidePost = point[postI] + (offsetPost * halfWidth);

			vsSqDistanceBetweenLineSegments( outsidePre,
					outsidePre + dirOfTravelPre * (distanceOfTravelPre + 3.f * halfWidth),
					outsidePost,
					outsidePost - dirOfTravelPost * (distanceOfTravelPost + 3.f * halfWidth),
					&closestPre, &closestPost );

			vertexPosition = vsInterpolate(0.5f, closestPre, closestPost);
		}
		else
		{
			vertexPosition = point[midI] + offsetPre * halfWidth;
		}

		va[vertexCursor+1].position = vertexPosition;

		if ( loop || i != count - 1 ) // not at the end of the strip
		{
			int otherSide = vertexCursor+2;
			if ( i == count-1 )
				otherSide = 0;
			ia[indexCursor] = vertexCursor;
			ia[indexCursor+1] = vertexCursor+1;
			ia[indexCursor+2] = otherSide;
			ia[indexCursor+3] = otherSide;
			ia[indexCursor+4] = vertexCursor+1;
			ia[indexCursor+5] = otherSide+1;
			indexCursor += 6;
		}
		vertexCursor += 2;
	}

	vsRenderBuffer* vertexBuffer = new vsRenderBuffer( vsRenderBuffer::Type_Static );
	vsRenderBuffer* indexBuffer = new vsRenderBuffer( vsRenderBuffer::Type_Static );
	vertexBuffer->SetArray(va, vertexCursor);
	indexBuffer->SetArray(ia, indexCursor);
	vsFragment *fragment = new vsFragment;
	vsDisplayList *dlist = new vsDisplayList(128);
	dlist->BindBuffer(vertexBuffer);
	dlist->TriangleListBuffer(indexBuffer);
	dlist->ClearArrays();
	fragment->SetDisplayList(dlist);
	fragment->SetMaterial( material );
	fragment->AddBuffer(vertexBuffer);
	fragment->AddBuffer(indexBuffer);

	vsDeleteArray(va);
	vsDeleteArray(ia);

	return fragment;
}

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

vsLines3D::vsLines3D( int maxStrips, float width, bool screenspace ):
	m_strip( new Strip*[maxStrips] ),
	m_stripCount( 0 ),
	m_maxStripCount( maxStrips ),
	m_width( width ),
	m_widthInScreenspace( screenspace ),
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
	if ( vertexCount == 0 || indexCount == 0 )
		return;

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
	int startOfStripVertexCursor = m_vertexCursor;
	float fullFov = queue->GetFOV();
	float fovPerPixel = fullFov / vsScreen::Instance()->GetHeight();
	float tanHalfFovPerPixel = 2.f * vsTan( 0.5f * fovPerPixel );

	// vsMatrix4x4 localToView = queue->GetMatrix() * queue->GetWorldToViewMatrix();
	vsMatrix4x4 localToView = queue->GetWorldToViewMatrix() * queue->GetMatrix() ;
	vsMatrix4x4 viewToLocal = localToView.Inverse();
	vsVector3D camPos = viewToLocal.ApplyTo(vsVector3D::Zero);

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
		if ( midI == preI )
			dirOfTravelPre = dirOfTravelPost;
		if ( midI == postI )
			dirOfTravelPost = dirOfTravelPre;
		float distanceOfTravelPre = dirOfTravelPre.Length();
		float distanceOfTravelPost = dirOfTravelPost.Length();
		dirOfTravelPre.NormaliseSafe();
		dirOfTravelPost.NormaliseSafe();

		vsVector3D offsetPre = dirOfTravelPre.Cross( cameraForward );
		vsVector3D offsetPost = dirOfTravelPost.Cross( cameraForward );
		offsetPre.NormaliseSafe();
		offsetPost.NormaliseSafe();

		float widthHere = m_width;
		if ( m_widthInScreenspace )
		{
			if ( queue->IsOrthographic() )
			{
				// TODO:  Figure out conversion factor between screen pixels
				// and meters in ortho.

				widthHere *= fovPerPixel;
			}
			else
			{
				vsVector3D viewPos = localToView.ApplyTo( vsVector4D(strip->m_vertex[midI],1.f) );
				widthHere *= tanHalfFovPerPixel * viewPos.z;
			}
		}
		float halfWidthHere = widthHere * 0.5f;

		vsVector3D vertexPosition;
		if ( offsetPre != offsetPost )
		{
			vsVector3D closestPre, closestPost;
			vsVector3D insidePre = strip->m_vertex[preI] - (offsetPre * halfWidthHere);
			vsVector3D insidePost = strip->m_vertex[postI] - (offsetPost * halfWidthHere);

			vsSqDistanceBetweenLineSegments( insidePre,
					insidePre + dirOfTravelPre * (distanceOfTravelPre + 3.f * halfWidthHere),
					insidePost,
					insidePost - dirOfTravelPost * (distanceOfTravelPost + 3.f * halfWidthHere),
					&closestPre, &closestPost );

			vertexPosition = vsInterpolate(0.5f, closestPre, closestPost);
		}
		else
		{
			vertexPosition = strip->m_vertex[midI] - offsetPre * halfWidthHere;
		}

		va[m_vertexCursor] = vertexPosition;

		if ( offsetPre != offsetPost )
		{
			vsVector3D closestPre, closestPost;
			vsVector3D outsidePre = strip->m_vertex[preI] + (offsetPre * halfWidthHere);
			vsVector3D outsidePost = strip->m_vertex[postI] + (offsetPost * halfWidthHere);

			vsSqDistanceBetweenLineSegments( outsidePre,
					outsidePre + dirOfTravelPre * (distanceOfTravelPre + 3.f * halfWidthHere),
					outsidePost,
					outsidePost - dirOfTravelPost * (distanceOfTravelPost + 3.f * halfWidthHere),
					&closestPre, &closestPost );

			vertexPosition = vsInterpolate(0.5f, closestPre, closestPost);
		}
		else
		{
			vertexPosition = strip->m_vertex[midI] + offsetPre * halfWidthHere;
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
		ia[m_indexCursor+2] = startOfStripVertexCursor;
		ia[m_indexCursor+3] = startOfStripVertexCursor;
		ia[m_indexCursor+4] = m_vertexCursor-1;
		ia[m_indexCursor+5] = startOfStripVertexCursor + 1;
		m_indexCursor += 6;
	}
}

