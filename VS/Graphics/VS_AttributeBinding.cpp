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
	// "SetVertexAttributes" implicitly clears attributes 0-4
	ClearAttribute(0);
	ClearAttribute(1);
	ClearAttribute(2);
	ClearAttribute(3);
	buffer->ApplyAttributeBindingsTo( this );
}

void
vsAttributeBinding::SetAttribute( int attribute, vsRenderBuffer *buffer, int size, int type, bool normalised, int stride, void* offset )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	Binding &b = m_attribute[attribute];

	b.buffer = buffer;
	b.size = size;
	b.type = type;
	b.normalised = normalised;
	b.stride = stride;
	b.offset = offset;

	if ( b.p != NULL )
		b.p = NULL;

	b.dirty = m_dirty = true;
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
	}
}

void
vsAttributeBinding::SetupAndBindVAO()
{
	if ( m_vao == 0 )
		glGenVertexArrays(1, &m_vao);

	glBindVertexArray(m_vao);

	for ( int i = 0; i < m_attribute.ItemCount(); i++ )
	{
		if ( m_attribute[i].dirty )
			DoBindAttribute(i);
	}

	m_dirty = false;
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

	b.dirty = false;
}

void
vsAttributeBinding::SetAttribute( int attribute, vsVector3D *p, int count )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	Binding &b = m_attribute[attribute];

	b.p = (float*)p;
	b.floatsPerVertex = 3;
	b.vertexCount = count;
	b.dirty = true;

	if ( b.buffer != NULL )
		b.buffer = NULL;
	m_dirty = true;
}


void
vsAttributeBinding::SetAttribute( int attribute, vsVector2D *p, int count )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	Binding &b = m_attribute[attribute];

	b.p = (float*)p;
	b.floatsPerVertex = 2;
	b.vertexCount = count;
	b.dirty = true;

	if ( b.buffer != NULL )
		b.buffer = NULL;
	m_dirty = true;
}

void
vsAttributeBinding::SetAttribute( int attribute, vsColor *p, int count )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	Binding &b = m_attribute[attribute];

	b.p = (float*)p;
	b.floatsPerVertex = 4;
	b.vertexCount = count;
	b.dirty = true;

	if ( b.buffer != NULL )
		b.buffer = NULL;
	m_dirty = true;
}

void
vsAttributeBinding::ClearAttribute( int attribute )
{
	if ( m_attribute.ItemCount() <= attribute )
		return;

	Binding &b = m_attribute[attribute];

	if ( b.p != NULL )
	{
		b.p = NULL;
		b.dirty = m_dirty = true;
	}
	if ( b.buffer != NULL )
	{
		b.buffer = NULL;
		b.dirty = m_dirty = true;
	}
}

