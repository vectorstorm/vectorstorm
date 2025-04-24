/*
 *  VS_VertexArrayObject.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/04/2025
 *  Copyright 2025 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_VERTEXARRAYOBJECT_H
#define VS_VERTEXARRAYOBJECT_H

#include "VS/Utils/VS_Array.h"

// per OpenGL spec, the number of vertex attributes supported must be at least 16.
#define MAX_ATTRIBS (16)

class vsVertexArrayObject
{
public:
	enum ComponentType
	{
		ComponentType_Byte,
		ComponentType_UByte,
		ComponentType_Int16,
		ComponentType_UInt16,
		ComponentType_Int32,
		ComponentType_UInt32,
		ComponentType_Int_2_10_10_10_REV,
		ComponentType_UInt_2_10_10_10_REV,
		ComponentType_Float
	};

	void _DoFlush();

private:
	uint32_t m_id;
	bool m_set;

	struct Attribute
	{
		int id;

		int vbo;
		int componentCount;
		ComponentType type;
		bool saturate;
		int stride;
		void* offset;
		int divisor;

		vsVector4D explicitValues;
		bool isExplicit;

		// bool dirty;
		bool operator==(const Attribute& o) const
		{
			if ( id != o.id ||
					isExplicit != o.isExplicit )
				return false;
			if ( isExplicit )
			{
				return explicitValues == o.explicitValues;
			}

			return (vbo==o.vbo &&
				componentCount==o.componentCount &&
				type==o.type &&
				saturate==o.saturate &&
				stride==o.stride &&
				offset==o.offset &&
				divisor==o.divisor);
		}
		bool operator!=(const Attribute& o) const
		{
			return !( *this == o );
		}
	};
	Attribute m_lastAttribute[MAX_ATTRIBS];
	Attribute m_attribute[MAX_ATTRIBS];
	// bool m_anyDirty;

	int m_elementBuffer;
	int m_lastElementBuffer;

	bool m_generic;
	bool m_in;

public:

	vsVertexArrayObject( bool generic = false );
	~vsVertexArrayObject();

	void Enter();
	void Exit();

	void Flush();

	bool IsSet( int id ) const;

	void BindAttribute( int attributeId, int vbo, int componentCount, ComponentType type, bool saturate, int stride, void* offset );
	void SetStaticAttribute4F( int attributeId, const vsVector4D& vals );
	void SetAttributeDivisor( int attributeId, int divisor );
	void UnbindAttribute( int attributeId );
	void UnbindAll();

	void SetElementBuffer( int vbo );
	void UnbindElementBuffer();

};

#endif // VS_VERTEXARRAYOBJECT_H

