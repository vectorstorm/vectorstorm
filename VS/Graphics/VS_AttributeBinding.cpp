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
	// if the renderbuffer doesn't provide one, set our color attribute to pure white
	SetExplicitValue( 3, vsVector4D(1,1,1,1) );
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

	b.p = NULL;
	b.hasExplicitValue = false;

	b.dirty = m_dirty = true;
}

void
vsAttributeBinding::Bind()
{
	GL_CHECK_SCOPED("vsAttributeBinding::Bind");
	if ( m_dirty )
	{
		SetupAndBindVAO();
		for ( int i = 0; i < m_attribute.ItemCount(); i++ )
		{
			if ( m_attribute[i].hasExplicitValue )
				DoBindAttribute(i);
		}
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
		// Explicit values are NOT bound to the VAO, and must be bound explicitly!
		//
		// OpenGL recommends we not use those, so.. need to come up with a clever
		// idea about how to work around that.
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
		glVertexAttribDivisor(i, b.divisor);
	}
	else if ( b.p )
	{
		vsRenderBuffer::BindArrayToAttribute(
				b.p,
				sizeof(float) * b.floatsPerVertex * b.vertexCount,
				i,
				b.floatsPerVertex );
		glEnableVertexAttribArray( i );
		glVertexAttribDivisor(i, b.divisor);
	}
	else if ( b.hasExplicitValue )
	{
		glDisableVertexAttribArray( i );
		glVertexAttrib4f(i, b.explicitValue.x, b.explicitValue.y, b.explicitValue.z, b.explicitValue.w );
	}
	else
	{
		glDisableVertexAttribArray( i );
	}

	b.dirty = false;
}

void
vsAttributeBinding::SetExplicitValue( int attribute, const vsVector4D& value )
{
	if ( m_attribute.ItemCount() <= attribute )
		m_attribute.SetArraySize(attribute+1);

	Binding &b = m_attribute[attribute];
	if ( b.hasExplicitValue && b.explicitValue == value )
	{
		// if ( attribute == 8 )
		{
			vsVector4D cur;
			glGetVertexAttribfv( attribute, GL_CURRENT_VERTEX_ATTRIB, (GLfloat*)&cur );
			if ( cur != value )
				vsLog("ERROR:  We don't know the value??");
		}
		return;
	}

	b.p = NULL;
	b.buffer = NULL;
	b.hasExplicitValue = true;
	b.explicitValue = value;
	b.dirty = true;
	m_dirty = true;
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

	b.buffer = NULL;
	b.hasExplicitValue = false;
	m_dirty = true;
}

void
vsAttributeBinding::SetInstanceAttribute( int attribute, vsRenderBuffer *buffer, int size, int type, bool normalised, int stride, void* offset )
{
	SetAttribute(attribute, buffer, size, type, normalised, stride, offset);
	m_attribute[attribute].divisor = 1;
	m_attribute[attribute].dirty = true;
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

	b.buffer = NULL;
	b.hasExplicitValue = false;
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

	b.buffer = NULL;
	b.hasExplicitValue = false;
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
	if ( b.hasExplicitValue )
	{
		b.hasExplicitValue = false;
		b.dirty = m_dirty = true;
	}
}

vsRenderBuffer *
vsAttributeBinding::GetAttributeBuffer( int attribute )
{
	if ( m_attribute.ItemCount() <= attribute )
		return NULL;

	return m_attribute[attribute].buffer;
}

void*
vsAttributeBinding::GetAttributeArray( int attribute )
{
	if ( m_attribute.ItemCount() <= attribute )
		return NULL;

	return m_attribute[attribute].p;
}

