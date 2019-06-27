/*
 *  VS_AttributeBinding.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 26/06/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_AttributeBinding.h"

#include "VS_RenderBuffer.h"
#include "VS_OpenGL.h"

vsAttributeBinding::vsAttributeBinding(int attributeCount):
	// m_vertexAttributes(NULL),
	m_attribute(attributeCount),
	m_dirty(true),
	m_vao(0)
{
}

vsAttributeBinding::~vsAttributeBinding()
{
	if ( m_vao )
		glDeleteVertexArrays(1, &m_vao);
}

void
vsAttributeBinding::SetVertexAttributes( vsRenderBuffer *buffer )
{
	// "VertexAttributes" implicitly clears attributes 0-4
	ClearAttribute(0);
	ClearAttribute(1);
	ClearAttribute(2);
	ClearAttribute(3);
	buffer->ApplyAttributeBindingsTo( this );
	// m_vertexAttributes = buffer;

	// SetAttribute(0, buffer);
	// SetAttribute(1, buffer);
	// SetAttribute(2, buffer);
	// SetAttribute(3, buffer);

	m_dirty = true;
}

void
vsAttributeBinding::SetAttribute( int attribute, vsRenderBuffer *buffer, int size, int type, bool normalised, int stride, void* offset )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	m_attribute[attribute].buffer = buffer;
	m_attribute[attribute].size = size;
	m_attribute[attribute].type = type;
	m_attribute[attribute].normalised = normalised;
	m_attribute[attribute].stride = stride;
	m_attribute[attribute].offset = offset;

	if ( m_attribute[attribute].p != NULL )
		m_attribute[attribute].p = NULL;

	m_dirty = true;

	// if ( attribute == 0 )
	// 	m_vertexAttributes = NULL; // bah.  This is SO so ugly
}

void
vsAttributeBinding::Bind()
{
	GL_CHECK_SCOPED("vsAttributeBinding::Bind");
	if ( m_dirty )
	{
		SetupAndBindVAO();
	}
	else
	{
		glBindVertexArray(m_vao);
		// m_attributeState.Flush();
	}
}

void
vsAttributeBinding::SetupAndBindVAO()
{
	if ( m_vao == 0 )
		glGenVertexArrays(1, &m_vao);

	glBindVertexArray(m_vao);
	// if ( m_vertexAttributes )
	// {
	// 	GL_CHECK_SCOPED("vsAttributeBinding::BindVertexAttributes");
	// 	m_attributeState.SetBool( vsAttributeState::ClientBool_VertexArray, false );
	// 	m_attributeState.SetBool( vsAttributeState::ClientBool_ColorArray, false );
	// 	m_attributeState.SetBool( vsAttributeState::ClientBool_NormalArray, false );
	// 	m_attributeState.SetBool( vsAttributeState::ClientBool_TextureCoordinateArray, false );
	// 	m_vertexAttributes->Bind(&m_attributeState);
	// 	m_attributeState.Force();
	// }
	// else
	{
		for ( int i = 0; i < 4; i++ )
		{
			DoBindAttribute(i);
		}
	}

	for ( int i = 4; i < m_attribute.ItemCount(); i++ )
	{
		DoBindAttribute(i);
	}

	// m_attributeState.Flush();
	// if ( m_attributeState.GetBool( ClientBool_VertexArray ) )
	// 	glEnableVertexAttribArray( 0 );
	// else
	// 	glDisableVertexAttribArray( 0 );

}

void
vsAttributeBinding::DoBindAttribute(int i)
{
	GL_CHECK_SCOPED("vsAttributeBinding::DoBindAttribute");
	if ( m_attribute.ItemCount() <= i )
		return;

	Binding &b = m_attribute[i];

	if ( b.buffer )
	{
		b.buffer->BindAsAttribute(i, b.size, b.type, b.normalised, b.stride, b.offset);
		glEnableVertexAttribArray( i );
	}
	else if ( b.p )
	{
		vsRenderBuffer::BindArrayToAttribute(
				b.p,
				sizeof(float) * b.floatsPerVertex * b.vertexCount,
				i,
				b.floatsPerVertex );
		glEnableVertexAttribArray( i );
	}
	else
	{
		glDisableVertexAttribArray( i );
	}
}

void
vsAttributeBinding::SetAttribute( int attribute, vsVector3D *p, int count )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	m_attribute[attribute].p = (float*)p;
	m_attribute[attribute].floatsPerVertex = 3;
	m_attribute[attribute].vertexCount = count;

	if ( m_attribute[attribute].buffer != NULL )
		m_attribute[attribute].buffer = NULL;
	m_dirty = true;
	// vsRenderBuffer::BindArrayToAttribute( p, sizeof(vsVector3D) * count, attribute, count );

	// if ( attribute == 0 )
	// 	m_vertexAttributes = NULL; // bah.  This is SO so ugly
}


void
vsAttributeBinding::SetAttribute( int attribute, vsVector2D *p, int count )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	m_attribute[attribute].p = (float*)p;
	m_attribute[attribute].floatsPerVertex = 2;
	m_attribute[attribute].vertexCount = count;

	if ( m_attribute[attribute].buffer != NULL )
		m_attribute[attribute].buffer = NULL;
	m_dirty = true;
	// vsRenderBuffer::BindArrayToAttribute( p, sizeof(vsVector2D) * count, attribute, count );
	// if ( attribute == 0 )
	// 	m_vertexAttributes = NULL; // bah.  This is SO so ugly
}

void
vsAttributeBinding::SetAttribute( int attribute, vsColor *p, int count )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	m_attribute[attribute].p = (float*)p;
	m_attribute[attribute].floatsPerVertex = 4;
	m_attribute[attribute].vertexCount = count;

	if ( m_attribute[attribute].buffer != NULL )
		m_attribute[attribute].buffer = NULL;
	m_dirty = true;
	// vsRenderBuffer::BindArrayToAttribute( p, sizeof(vsColor) * count, attribute, count );
	// if ( attribute == 0 )
	// 	m_vertexAttributes = NULL; // bah.  This is SO so ugly
}

void
vsAttributeBinding::ClearAttribute( int attribute )
{
	if ( m_attribute.ItemCount() <= attribute )
		return;

	if ( m_attribute[attribute].p != NULL )
	{
		m_attribute[attribute].p = NULL;
		m_dirty = true;
	}
	if ( m_attribute[attribute].buffer != NULL )
	{
		m_attribute[attribute].buffer = NULL;
		m_dirty = true;
	}

}

