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

#include "VS/Files/VS_Record.h"

vsFragment::vsFragment():
	m_material(NULL),
	m_displayList(NULL)
{
}

vsFragment::~vsFragment()
{
	vsDelete( m_material );
	vsDelete( m_displayList );
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
				if ( ssrLabel == "BindNormalBuffer" )
				{
					int bufferId = ssr->GetToken(0).AsInteger();

					if ( bufferId < result->m_bufferList.ItemCount() )
					{
						loader->NormalBuffer( result->m_bufferList[bufferId] );
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
	m_material = new vsMaterial(material);
}

void
vsFragment::SetMaterial( const vsString &name )
{
	vsDelete( m_material );
	m_material = new vsMaterial(name);
}

void
vsFragment::AddBuffer( vsRenderBuffer *buffer )
{
	m_bufferList.AddItem( buffer );
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
