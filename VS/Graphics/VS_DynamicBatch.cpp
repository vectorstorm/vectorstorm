/*
 *  VS_DynamicBatch.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 10/07/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_DynamicBatch.h"
#include "VS_DisplayList.h"
#include "VS_Fragment.h"

vsDynamicBatch::vsDynamicBatch():
	m_vbo(vsRenderBuffer::Type_Stream),
	m_ibo(vsRenderBuffer::Type_Stream)
{
}

void
vsDynamicBatch::Reset()
{
}

bool
vsDynamicBatch::Supports( vsRenderBuffer::ContentType type )
{
	// As I'm rolling this out, add the supported types here.
	switch( type )
	{
		case vsRenderBuffer::ContentType_PC:
		case vsRenderBuffer::ContentType_PT:
		case vsRenderBuffer::ContentType_PCT:
			return true;
		default:
			return false;
	}
}

void
vsDynamicBatch::StartBatch( vsRenderBuffer *vbo, vsRenderBuffer *ibo, const vsMatrix4x4& mat, vsFragment::SimpleType type )
{
	AddToBatch_Internal(vbo, ibo, mat, type, true);
}

void
vsDynamicBatch::AddToBatch( vsRenderBuffer *vbo, vsRenderBuffer *ibo, const vsMatrix4x4& mat, vsFragment::SimpleType type )
{
	AddToBatch_Internal(vbo, ibo, mat, type, false);
}

void
vsDynamicBatch::AddToBatch_Internal( vsRenderBuffer *fvbo, vsRenderBuffer *fibo, const vsMatrix4x4& mat, vsFragment::SimpleType simpleType, bool first )
{
	vsAssert( Supports(fvbo->GetContentType()), "Unsupported content type" );

	// Okay.  So.  In here we need to do a few things.  FIRST:
	//
	// I need to apply the given matrix to every position in the fragment, and
	// also apply (rotation only) to the normals.
	//
	// vertices

	int indexOfFirstVertex = 0;//m_vbo.GetPositionCount();

	if ( fvbo->GetContentType() == vsRenderBuffer::ContentType_PT )
	{
		int size = first ? 0 : m_vbo.GetGenericArraySize();
		indexOfFirstVertex = size / sizeof(vsRenderBuffer::PT);
		m_vbo.ResizeArray( size + fvbo->GetGenericArraySize() );
		vsRenderBuffer::PT* i = fvbo->GetPTArray();
		vsRenderBuffer::PT* o = m_vbo.GetPTArray();

		int oo = size / sizeof(vsRenderBuffer::PT);
		for (int ii = 0; ii < fvbo->GetPositionCount(); ii++)
		{
			o[oo].position = mat.ApplyTo( i[ii].position );
			o[oo].texel = i[ii].texel;
			oo++;
		}
	}
	else if ( fvbo->GetContentType() == vsRenderBuffer::ContentType_PC )
	{
		int size = first ? 0 : m_vbo.GetGenericArraySize();
		indexOfFirstVertex = size / sizeof(vsRenderBuffer::PC);
		m_vbo.ResizeArray( size + fvbo->GetGenericArraySize() );
		vsRenderBuffer::PC* i = fvbo->GetPCArray();
		vsRenderBuffer::PC* o = m_vbo.GetPCArray();

		int oo = size / sizeof(vsRenderBuffer::PC);
		for (int ii = 0; ii < fvbo->GetPositionCount(); ii++)
		{
			o[oo].position = mat.ApplyTo( i[ii].position );
			o[oo].color = i[ii].color;
			oo++;
		}
	}
	else if ( fvbo->GetContentType() == vsRenderBuffer::ContentType_PCT )
	{
		int size = first ? 0 : m_vbo.GetGenericArraySize();
		indexOfFirstVertex = size / sizeof(vsRenderBuffer::PCT);
		m_vbo.ResizeArray( size + fvbo->GetGenericArraySize() );
		vsRenderBuffer::PCT* i = fvbo->GetPCTArray();
		vsRenderBuffer::PCT* o = m_vbo.GetPCTArray();

		int oo = size / sizeof(vsRenderBuffer::PCT);
		for (int ii = 0; ii < fvbo->GetPositionCount(); ii++)
		{
			o[oo].position = mat.ApplyTo( i[ii].position );
			o[oo].color = i[ii].color;
			o[oo].texel = i[ii].texel;
			oo++;
		}
	}
	m_vbo.SetContentType(fvbo->GetContentType());

	// indices
	{
		// int vstart = m_ibo.GetIntArraySize();
		int oo = first ? 0 : m_ibo.GetIntArraySize();

		int fragmentIndexCount = fibo->GetIntArraySize();
		int trianglesForNewFragment = 0;
		switch( simpleType )
		{
			case vsFragment::SimpleType_TriangleList:
				trianglesForNewFragment = fragmentIndexCount / 3;
				break;
			case vsFragment::SimpleType_TriangleStrip:
			case vsFragment::SimpleType_TriangleFan:
				// For a triangle strip or fan, we have (n-2) triangles.
				trianglesForNewFragment = vsMax(0,fragmentIndexCount-2);
				break;
		}

		m_ibo.SetIntArraySize( oo + (3*trianglesForNewFragment) );

		uint16_t* i = fibo->GetIntArray();
		uint16_t* o = m_ibo.GetIntArray();

		switch( simpleType )
		{
			case vsFragment::SimpleType_TriangleList:
			{
				for (int ii = 0; ii < fibo->GetIntArraySize(); ii++)
				{
					o[oo++] = i[ii] + indexOfFirstVertex;
				}
				break;
			}
			case vsFragment::SimpleType_TriangleStrip:
			{
				// Okay.  For a triangle strip we add the first three,
				// then add sets of three most recent
				for (int ii = 0; ii < fibo->GetIntArraySize(); ii++)
				{
					if (ii < 3)
						o[oo++] = i[ii] + indexOfFirstVertex;
					else
					{
						o[oo++] = i[ii-2] + indexOfFirstVertex;
						o[oo++] = i[ii-1] + indexOfFirstVertex;
						o[oo++] = i[ii-0] + indexOfFirstVertex;
					}
				}
				break;
			}
			case vsFragment::SimpleType_TriangleFan:
			{
				// Okay.  For a triangle fan we add the first three,
				// then add first and most recent two
				for (int ii = 0; ii < fibo->GetIntArraySize(); ii++)
				{
					if (ii < 3)
						o[oo++] = i[ii] + indexOfFirstVertex;
					else
					{
						o[oo++] = i[0] + indexOfFirstVertex;
						o[oo++] = i[ii-1] + indexOfFirstVertex;
						o[oo++] = i[ii-0] + indexOfFirstVertex;
					}
				}
				break;
			}
		}
	}
}

void
vsDynamicBatch::Draw( vsDisplayList * list )
{
	m_vbo.BakeArray();
	m_ibo.BakeIndexArray();
	// if there are any indices (there will be!)...
	list->BindBuffer( &m_vbo );
	list->TriangleListBuffer( &m_ibo );
	list->ClearArrays();
}
