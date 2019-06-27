/*
 *  VS_AttributeBinding.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 26/06/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_ATTRIBUTEBINDING_H
#define VS_ATTRIBUTEBINDING_H

#include "VS/Utils/VS_Array.h"
#include "VS_RendererState.h"
class vsRenderBuffer;
class vsVector2D;
class vsVector3D;
class vsColor;

class vsAttributeBinding
{
	struct Binding
	{
		vsRenderBuffer *buffer;
		int size;
		int type; // GL_FLOAT or GL_UNSIGNED_BYTE, usually
		bool normalised;
		int stride;
		void* offset;

		float *p;
		int vertexCount;
		int floatsPerVertex;

		bool dirty;

		Binding(): buffer(NULL), p(NULL), vertexCount(0), floatsPerVertex(0), dirty(0) {}
	};
	vsArray<Binding> m_attribute;

	bool m_dirty;
	uint32_t m_vao;

	void SetupAndBindVAO();
	void DoBindAttribute(int i);

public:
	vsAttributeBinding(int attributeCount = 4);
	~vsAttributeBinding();

	void SetVertexAttributes( vsRenderBuffer *buffer );
	void SetAttribute( int attribute, vsRenderBuffer *buffer, int size, int type, bool normalised, int stride = 0, void* offset = NULL );

	void SetAttribute( int attribute, vsVector3D *p, int count );
	void SetAttribute( int attribute, vsVector2D *p, int count );
	void SetAttribute( int attribute, vsColor *p, int count );

	vsRenderBuffer * GetAttributeBuffer( int attribute );
	void* GetAttributeArray( int attribute );

	void ClearAttribute( int attribute );

	// "Bind()' is only to be called from the main thread.
	void Bind();
};

#endif // VS_ATTRIBUTEBINDING_H

