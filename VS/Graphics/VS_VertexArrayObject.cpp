/*
 *  VS_VertexArrayObject.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/04/2025
 *  Copyright 2025 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_VertexArrayObject.h"

#include "VS_OpenGL.h"
#include "VS_Profile.h"

vsVertexArrayObject::vsVertexArrayObject():
	m_id(0),
	m_set(false),
	m_attribute(),
	m_in(false)
{
}

vsVertexArrayObject::~vsVertexArrayObject()
{
	if ( m_set )
		glDeleteVertexArrays(1, &m_id);
}

void
vsVertexArrayObject::Enter()
{
	if ( !m_set )
	{
		glGenVertexArrays(1, &m_id);
		m_set = true;

		for ( int i = 0; i < MAX_ATTRIBS; i++ )
		{
			m_attribute[i].id = i;
			m_attribute[i].vbo = -1;
			m_attribute[i].divisor = 0;

			m_lastAttribute[i] = m_attribute[i];
		}

	}
	glBindVertexArray( m_id );

	m_in = true;
}

void
vsVertexArrayObject::Exit()
{
	m_in = false;
}

bool
vsVertexArrayObject::IsSet( int id ) const
{
	return m_attribute[id].vbo >= 0;
}

void
vsVertexArrayObject::Flush()
{
	PROFILE("vsVertexArrayObject::Flush");
	vsAssert(m_in, "vsVertexArrayObject::Flush called when we're not in it??");

	// if ( m_anyDirty )
	{
		PROFILE("vsVertexArrayObject::Flush - DIRTY");
		// m_anyDirty = false;

		for ( int i = 0; i < MAX_ATTRIBS; i++ )
		{
			Attribute &p = m_lastAttribute[i];
			Attribute &a = m_attribute[i];
			if ( p != a )
			{
				p = a;

				// a.dirty = false;

				if ( a.vbo < 0 ) // disabled, move to the next one!
				{
					glDisableVertexAttribArray( i );
					continue;
				}

				if ( a.isExplicit )
				{
					glDisableVertexAttribArray( i );
					glVertexAttrib4f( i, a.explicitValues.x, a.explicitValues.y, a.explicitValues.z, a.explicitValues.w );
				}
				else
				{
					glEnableVertexAttribArray( i );

					if ( a.vbo > 0 )
						glBindBuffer(GL_ARRAY_BUFFER, a.vbo);

					int glType = GL_FLOAT;
					switch ( a.type )
					{
						case ComponentType_Byte:
							glType = GL_BYTE;
							break;
						case ComponentType_UByte:
							glType = GL_UNSIGNED_BYTE;
							break;
						case ComponentType_Int16:
							glType = GL_SHORT;
							break;
						case ComponentType_UInt16:
							glType = GL_UNSIGNED_SHORT;
							break;
						case ComponentType_Int32:
							glType = GL_INT;
							break;
						case ComponentType_UInt32:
							glType = GL_UNSIGNED_INT;
							break;
						case ComponentType_Int_2_10_10_10_REV:
							glType = GL_INT_2_10_10_10_REV;
							break;
						case ComponentType_UInt_2_10_10_10_REV:
							glType = GL_UNSIGNED_INT_2_10_10_10_REV;
							break;
						case ComponentType_Float:
							glType = GL_FLOAT;
							break;
					}

					glVertexAttribPointer( i, a.componentCount, glType, a.saturate ? GL_TRUE : GL_FALSE, a.stride, a.offset );
					glVertexAttribDivisor( i, a.divisor );
				}
			}
		}
	}
}

// bool
// vsVertexArrayObject::IsBound( int id, vsRenderBuffer *buf ) const
// {
// 	return m_attribute.
// }

void
vsVertexArrayObject::BindAttribute( int attributeId, int vbo, int componentCount, ComponentType type, bool saturate, int stride, void* offset )
{
	vsAssert( attributeId < MAX_ATTRIBS, "Out of range attribute requested");
	Attribute &a = m_attribute[attributeId];
	if ( a.vbo != vbo ||
			a.componentCount != componentCount ||
			a.type != type ||
			a.saturate != saturate ||
			a.stride != stride ||
			a.offset != offset )
	{
		a.id = attributeId;
		a.vbo = vbo;
		a.componentCount = componentCount;
		a.type = type;
		a.saturate = saturate;
		a.stride = stride;
		a.offset = offset;
		a.isExplicit = false;
		// a.dirty = true;
		// m_anyDirty = true;
	}
}

void
vsVertexArrayObject::SetStaticAttribute4F( int attributeId, const vsVector4D& vals )
{
	vsAssert( attributeId < MAX_ATTRIBS, "Out of range attribute requested");
	Attribute &a = m_attribute[attributeId];
	a.vbo = 0;
	a.componentCount = 4;
	a.type = ComponentType_Float;
	a.isExplicit = true;
	a.explicitValues = vals;
}

void
vsVertexArrayObject::SetAttributeDivisor( int attributeId, int divisor )
{
	vsAssert( attributeId < MAX_ATTRIBS, "Out of range attribute requested");
	Attribute &a = m_attribute[attributeId];
	a.divisor = divisor;
}

void
vsVertexArrayObject::UnbindAttribute( int attributeId )
{
	vsAssert( attributeId < MAX_ATTRIBS, "Out of range attribute requested");
	Attribute &a = m_attribute[attributeId];
	if ( a.vbo != -1 )
	{
		a.vbo = -1;
		a.divisor = 0;
		// a.dirty = true;
		// m_anyDirty = true;
	}
}

void
vsVertexArrayObject::UnbindAll()
{
	// and now we disable all attributes, blah
	for ( int i = 0; i < MAX_ATTRIBS; i++ )
	{
		m_attribute[i].vbo = -1;
		m_attribute[i].divisor = 0;
	}
}


