/*
 *  VS_Fragment.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 1/08/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Fragment.h"

#include "VS_DisplayList.h"
#include "VS_VertexArrayObject.h"

#include "VS/Files/VS_Record.h"

namespace
{
	vsArray<vsVertexArrayObject*> s_unusedVao;
}

vsFragment::vsFragment():
	m_material(nullptr),
	m_displayList(nullptr),
	m_vao(nullptr),
	m_visible(true),
	m_simpleVbo(nullptr),
	m_simpleIbo(nullptr)
{
	if ( !s_unusedVao.IsEmpty() )
	{
		m_vao = *s_unusedVao.Back();
		s_unusedVao.PopBack();

		m_vao->UnbindAll();
	}
	else
	{
		m_vao = new vsVertexArrayObject;
	}
}

vsFragment::~vsFragment()
{
	vsDelete( m_material );
	vsDelete( m_displayList );

	if ( m_vao )
		s_unusedVao.AddItem(m_vao);
	// vsDelete( m_vao );
}

vsFragment *
vsFragment::Load( vsRecord *record )
{
	vsFragment *result = new vsFragment;

	for ( int i = 0; i < record->GetChildCount(); i++ )
	{
		vsRecord *sr = record->GetChild(i);

		vsString srLabel = sr->GetLabel().AsString();

		if ( srLabel == "Material" )
		{
			vsDelete( result->m_material );
			result->m_material = new vsMaterial( sr->GetToken(0).AsString() );
		}
		else if ( srLabel == "PBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsRenderBuffer::P *va = new vsRenderBuffer::P[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					vsAssert( s->GetTokenCount() == 3, "Wrong number of tokens in P buffer" );
					va[id].position.Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "PCBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsRenderBuffer::PC *va = new vsRenderBuffer::PC[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					vsAssert( s->GetTokenCount() == 7, "Wrong number of tokens in PC buffer" );
					va[id].position.Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
					va[id].color.Set(s->GetToken(3).AsFloat(), s->GetToken(4).AsFloat(), s->GetToken(5).AsFloat(), s->GetToken(6).AsFloat());
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "PNBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsRenderBuffer::PN *va = new vsRenderBuffer::PN[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					vsAssert( s->GetTokenCount() == 6, "Wrong number of tokens in PN buffer" );
					va[id].position.Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
					va[id].normal = vsVector3D(s->GetToken(3).AsFloat(), s->GetToken(4).AsFloat(), s->GetToken(5).AsFloat());
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "PCTBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsRenderBuffer::PCT *va = new vsRenderBuffer::PCT[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					vsAssert( s->GetTokenCount() == 9, "Wrong number of tokens in PCT buffer" );
					va[id].position.Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
					va[id].color.Set(s->GetToken(3).AsFloat(), s->GetToken(4).AsFloat(), s->GetToken(5).AsFloat(), s->GetToken(6).AsFloat());
					va[id].texel.Set(s->GetToken(7).AsFloat(), s->GetToken(8).AsFloat());
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "PNTBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsRenderBuffer::PNT *va = new vsRenderBuffer::PNT[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					vsAssert( s->GetTokenCount() == 8, "Wrong number of tokens in PNT buffer" );
					va[id].position.Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());

					vsVector3D n(s->GetToken(3).AsFloat(), s->GetToken(4).AsFloat(), s->GetToken(5).AsFloat());
					vsVector2D t(s->GetToken(6).AsFloat(), s->GetToken(7).AsFloat());
					va[id].normal = n;
					va[id].texel = t;
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "PCNBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsRenderBuffer::PCN *va = new vsRenderBuffer::PCN[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					vsAssert( s->GetTokenCount() == 10, "Wrong number of tokens in PCNT buffer" );
					va[id].position.Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
					va[id].color.Set(s->GetToken(3).AsFloat(), s->GetToken(4).AsFloat(), s->GetToken(5).AsFloat(), s->GetToken(6).AsFloat());
					vsVector3D n(s->GetToken(7).AsFloat(), s->GetToken(8).AsFloat(), s->GetToken(9).AsFloat());
					va[id].normal.Set(n);
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "PCNTBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsRenderBuffer::PCNT *va = new vsRenderBuffer::PCNT[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					vsAssert( s->GetTokenCount() == 12, "Wrong number of tokens in PCNT buffer" );
					va[id].position.Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
					va[id].color.Set(s->GetToken(3).AsFloat(), s->GetToken(4).AsFloat(), s->GetToken(5).AsFloat(), s->GetToken(6).AsFloat());
					va[id].normal.Set(s->GetToken(7).AsFloat(), s->GetToken(8).AsFloat(), s->GetToken(9).AsFloat());
					va[id].texel.Set(s->GetToken(10).AsFloat(), s->GetToken(11).AsFloat());
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "VertexBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int arrayCount = sr->GetChildCount();
			if ( arrayCount )
			{
				vsRenderBuffer *buffer = new vsRenderBuffer;

				vsVector3D *va = new vsVector3D[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					vsRecord *s = sr->GetChild(id);
					if ( s->GetTokenCount() == 3 )
						va[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
					else
						va[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), 0.f);
				}

				buffer->SetArray(va, arrayCount);
				vsDeleteArray(va);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "IndexBuffer" )
		{
			// make ourselves a vsRenderBuffer to contain this vertex data.
			int childCount = sr->GetChildCount();
			vsAssert( childCount == 1, "Wrong number of children of IndexBuffer" );
			{
				vsRecord *s = sr->GetChild(0);
				int arrayCount = s->GetTokenCount();
				vsRenderBuffer *buffer = new vsRenderBuffer;

				uint16_t *ind = new uint16_t[arrayCount];
				vsRecord subRecord;

				for ( int id = 0; id < arrayCount; id++ )
				{
					ind[id] = s->GetToken(id).AsInteger();
				}

				buffer->SetArray(ind, arrayCount);
				vsDeleteArray(ind);

				result->AddBuffer(buffer);
			}
		}
		else if ( srLabel == "DisplayList" )
		{
			vsDisplayList *loader = new vsDisplayList(1024*50);

			for ( int j = 0; j < sr->GetChildCount(); j++ )
			{
				vsRecord *ssr = sr->GetChild(j);
				vsString ssrLabel = ssr->GetLabel().AsString();

				if ( ssrLabel == "BindVertexBuffer" )
				{
					int bufferId = ssr->GetToken(0).AsInteger();

					if ( bufferId < result->m_bufferList.ItemCount() )
					{
						loader->VertexBuffer( result->m_bufferList[bufferId] );
					}
				}
				else if ( ssrLabel == "BindNormalBuffer" )
				{
					int bufferId = ssr->GetToken(0).AsInteger();

					if ( bufferId < result->m_bufferList.ItemCount() )
					{
						loader->NormalBuffer( result->m_bufferList[bufferId] );
					}
				}
				else if ( ssrLabel == "BindBuffer" )
				{
					int bufferId = ssr->GetToken(0).AsInteger();

					if ( bufferId < result->m_bufferList.ItemCount() )
					{
						loader->BindBuffer( result->m_bufferList[bufferId] );
					}
				}
				else if ( ssrLabel == "TriangleListBuffer" )
				{
					int bufferId = ssr->GetToken(0).AsInteger();

					if ( bufferId < result->m_bufferList.ItemCount() )
					{
						loader->TriangleListBuffer( result->m_bufferList[bufferId] );
					}
				}
				else
				{
					vsDisplayList::Load_Vec_SingleRecord( loader, ssr );
				}
			}

			vsDisplayList *list = new vsDisplayList( loader->GetSize() );
			list->Append(*loader);
			vsDelete(loader);

			result->SetDisplayList( list );
		}
	}

	return result;
}

void
vsFragment::SetMaterial( vsMaterial *material )
{
	vsDelete( m_material );
	m_material = new vsMaterial(*material);
}

void
vsFragment::SetMaterial( const vsString &name )
{
	vsDelete( m_material );
	m_material = new vsMaterial(name);
}

void
vsFragment::SetSimple( vsRenderBuffer *vbo, vsRenderBuffer *ibo, SimpleType type, SimpleBufferOwnershipType ownershipType )
{
	m_displayList = nullptr;
	m_simpleType = type;
	m_simpleVbo = vbo;
	m_simpleIbo = ibo;

	if ( ownershipType & Owned_VBO )
		AddBuffer(vbo);
	if ( ownershipType & Owned_IBO )
		AddBuffer(ibo);
}

void
vsFragment::AddBuffer( vsRenderBuffer *buffer )
{
	m_bufferList.AddItem( buffer );
}

void
vsFragment::Clear()
{
	vsDelete(m_material);
	vsDelete(m_displayList);
	m_simpleVbo = m_simpleIbo = nullptr;
	m_bufferList.Clear();
}

void
vsFragment::Draw( vsDisplayList *list )
{
	if ( m_displayList )
	{
		list->SetMaterial( m_material );
		list->Append( *m_displayList );
	}
}

int
vsFragment::GetTriangles(vsArray<struct vsDisplayList::Triangle>& result) const
{
	if ( IsSimple() )
	{
		uint16_t *index = m_simpleIbo->GetIntArray();
		int ic = m_simpleIbo->GetIntArraySize();

		if ( m_simpleType == SimpleType_TriangleList )
		{
			for ( int i = 2; i < ic; i+=3 )
			{
				uint16_t index0 = index[i-2];
				uint16_t index1 = index[i-1];
				uint16_t index2 = index[i];
				vsDisplayList::Triangle t;
				t.vertex[0] = m_simpleVbo->GetPosition(index0);
				t.vertex[1] = m_simpleVbo->GetPosition(index1);
				t.vertex[2] = m_simpleVbo->GetPosition(index2);
				result.AddItem(t);
			}
		}
		else if ( m_simpleType == SimpleType_TriangleStrip )
		{
			for ( int i = 2; i < ic; i++ )
			{
				uint16_t index0 = index[i-2];
				uint16_t index1 = index[i-1];
				uint16_t index2 = index[i];
				vsDisplayList::Triangle t;
				t.vertex[0] = m_simpleVbo->GetPosition(index0);
				t.vertex[1] = m_simpleVbo->GetPosition(index1);
				t.vertex[2] = m_simpleVbo->GetPosition(index2);
				result.AddItem(t);
			}
		}
		else if ( m_simpleType == SimpleType_TriangleFan )
		{
			for ( int i = 1; i < ic; i++ )
			{
				uint16_t index0 = index[0];
				uint16_t index1 = index[i-1];
				uint16_t index2 = index[i];
				vsDisplayList::Triangle t;
				t.vertex[0] = m_simpleVbo->GetPosition(index0);
				t.vertex[1] = m_simpleVbo->GetPosition(index1);
				t.vertex[2] = m_simpleVbo->GetPosition(index2);
				result.AddItem(t);
			}
		}
	}
	else if ( m_displayList )
	{
		return m_displayList->GetTriangles(result);
	}

	return result.ItemCount();
}

vsBox3D
vsFragment::GetBoundingBox() const
{
	vsBox3D result;

	for ( int i = 0; i < m_simpleVbo->GetPositionCount(); i++ )
	{
		result.ExpandToInclude( m_simpleVbo->GetPosition(i) );
	}

	return result;
}
