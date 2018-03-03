/*
 *  VS_RenderBuffer.cpp
 *  Lord
 *
 *  Created by Trevor Powell on 5/01/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_RenderBuffer.h"

#include "VS_RendererState.h"

#include "VS_OpenGL.h"
#include "VS_Profile.h"


#define POS_ATTRIBUTE (0)
#define TEXCOORD_ATTRIBUTE (1)
#define NORMAL_ATTRIBUTE (2)
#define COLOR_ATTRIBUTE (3)

static int s_glBufferType[vsRenderBuffer::TYPE_MAX] =
{
	0,
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW,
#if TARGET_OS_IPHONE
	GL_DYNAMIC_DRAW
#else
	GL_STREAM_DRAW
#endif
};

vsRenderBuffer::vsRenderBuffer(vsRenderBuffer::Type type):
    m_array(NULL),
    m_arrayBytes(0),
    m_glArrayBytes(0),
    m_activeBytes(0),
    m_type(type),
    m_contentType(ContentType_Custom),
    m_bufferID(-1),
    m_vbo(false),
    m_bindType(BindType_Array)
{
	vsAssert( sizeof( uint16_t ) == 2, "I've gotten the size wrong??" );

	// TESTING:  Seems like iPhone runs SLOWER with VBOs than with arrays, so just use our vsRenderBuffer in "array" mode.
#if !TARGET_OS_IPHONE
	if ( glGenBuffers && m_type != Type_NoVBO )
	{
		glGenBuffers(1, (GLuint*)&m_bufferID);
		m_vbo = true;
	}
#endif
}

vsRenderBuffer::~vsRenderBuffer()
{
	if ( m_vbo )
	{
		glDeleteBuffers( 1, (GLuint*)&m_bufferID );
	}

	{
		vsDeleteArray( m_array );
	}
}

void
vsRenderBuffer::ResizeArray( int size )
{
    ResizeArray_Internal( size );
}

void
vsRenderBuffer::SetActiveSize( int size )
{
	vsAssert(size != 0, "Zero-sized buffer??");
	m_activeBytes = size;
}

void
vsRenderBuffer::SetArraySize_Internal( int size )
{
	if ( m_array && size > m_arrayBytes )
	{
		vsDeleteArray( m_array );
	}
	if ( m_array == NULL )
	{
		m_array = new char[size];
		m_arrayBytes = size;
	}
	vsAssert(size != 0, "Zero-sized buffer?");
    SetActiveSize(size);
}

void
vsRenderBuffer::ResizeArray_Internal( int size )
{
	if ( m_array && size > m_arrayBytes )
	{
		char* newArray = new char[size];
		memcpy(newArray, m_array, m_arrayBytes);
		m_arrayBytes = size;
		vsDeleteArray( m_array );
		m_array = newArray;
	}
	SetArraySize_Internal( size );
}

void
vsRenderBuffer::SetArray_Internal( char *data, int size, vsRenderBuffer::BindType bindType )
{
	vsAssert( size, "Error:  Tried to set a zero-length GPU buffer!" );

	int bindPoints[BindType_MAX] =
	{
		GL_ARRAY_BUFFER,
		GL_ELEMENT_ARRAY_BUFFER,
		GL_TEXTURE_BUFFER
	};
	int bindPoint = bindPoints[bindType];

	m_bindType = bindType;

	if ( m_vbo )
	{
		glBindBuffer(bindPoint, m_bufferID);

		if ( size > m_glArrayBytes )
		{
			glBufferData(bindPoint, size, data, s_glBufferType[m_type]);
			m_glArrayBytes = size;
		}
		else
		{
			// glBufferData(bindPoint, size, NULL, s_glBufferType[m_type]);
			// glBufferData(bindPoint, size, data, s_glBufferType[m_type]);
			void *ptr = glMapBufferRange(bindPoint, 0, m_glArrayBytes, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

			if ( ptr )
			{
				memcpy(ptr, data, size);
				glUnmapBuffer(bindPoint);
			}
		}

		glBindBuffer(bindPoint, 0);
	}
	m_activeBytes = size;

	if ( data != m_array )
	{
		SetArraySize_Internal( size );
		memcpy(m_array,data,size);
	}
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::P *array, int size )
{
	m_contentType = ContentType_P;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::P), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::PC *array, int size )
{
	m_contentType = ContentType_PC;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::PC), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::PT *array, int size )
{
	m_contentType = ContentType_PT;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::PT), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::PN *array, int size )
{
	m_contentType = ContentType_PN;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::PN), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::PCT *array, int size )
{
	m_contentType = ContentType_PCT;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::PCT), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::PNT *array, int size )
{
	m_contentType = ContentType_PNT;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::PNT), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::PCN *array, int size )
{
	m_contentType = ContentType_PCN;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::PCN), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsRenderBuffer::PCNT *array, int size )
{
	m_contentType = ContentType_PCNT;

	SetArray_Internal((char *)array, size*sizeof(vsRenderBuffer::PCNT), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsMatrix4x4 *array, int size )
{
	m_contentType = ContentType_Matrix;
	SetArray_Internal((char *)array, size*sizeof(vsMatrix4x4), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsVector3D *array, int size )
{
	m_contentType = ContentType_P;
	SetArray_Internal((char *)array, size*sizeof(vsVector3D), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsVector2D *array, int size )
{
	SetArray_Internal((char *)array, size*sizeof(vsVector2D), BindType_Array);
}

void
vsRenderBuffer::SetArray( const vsColor *array, int size )
{
	m_contentType = ContentType_Color;
	SetArray_Internal((char *)array, size*sizeof(vsColor), BindType_Array);
}

void
vsRenderBuffer::SetArray( const uint16_t *array, int size )
{
	m_contentType = ContentType_UInt16;
	SetArray_Internal((char *)array, size*sizeof(uint16_t), BindType_ElementArray);
}

void
vsRenderBuffer::SetArray( const uint32_t *array, int size )
{
	m_contentType = ContentType_UInt32;
	SetArray_Internal((char *)array, size*sizeof(uint32_t), BindType_ElementArray);
}

void
vsRenderBuffer::SetArray( const float *array, int size )
{
	m_contentType = ContentType_Float;
	SetArray_Internal((char *)array, size*sizeof(float), BindType_TextureBuffer);
}

void
vsRenderBuffer::SetArray( const vsVector4D_ui32 *array, int size )
{
	m_contentType = ContentType_UI32Vec4;
	SetArray_Internal((char *)array, size*sizeof(vsVector4D_ui32), BindType_TextureBuffer);
}

void
vsRenderBuffer::SetArray( const vsVector4D_i32 *array, int size )
{
	m_contentType = ContentType_I32Vec4;
	SetArray_Internal((char *)array, size*sizeof(vsVector4D_i32), BindType_TextureBuffer);
}

void
vsRenderBuffer::SetVector3DArraySize( int size )
{
	SetArraySize_Internal(size*sizeof(vsVector3D));
}

void
vsRenderBuffer::SetVector2DArraySize( int size )
{
	SetArraySize_Internal(size*sizeof(vsVector2D));
}

void
vsRenderBuffer::SetColorArraySize( int size )
{
	SetArraySize_Internal(size*sizeof(vsColor));
}

void
vsRenderBuffer::SetIntArraySize( int size )
{
	SetArraySize_Internal(size*sizeof(uint16_t));
}

void
vsRenderBuffer::BakeArray()
{
	SetArray_Internal( m_array, m_activeBytes, BindType_Array );
}

void
vsRenderBuffer::BakeIndexArray()
{
	SetArray_Internal( m_array, m_activeBytes, BindType_ElementArray );
}

void
vsRenderBuffer::BindAsAttribute( int attributeId )
{
	if ( m_contentType == ContentType_Matrix && m_vbo )
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
		glVertexAttribPointer(attributeId, 4, GL_FLOAT, GL_FALSE, 64, 0);
		glVertexAttribPointer(attributeId+1, 4, GL_FLOAT, GL_FALSE, 64, (void*)16);
		glVertexAttribPointer(attributeId+2, 4, GL_FLOAT, GL_FALSE, 64, (void*)32);
		glVertexAttribPointer(attributeId+3, 4, GL_FLOAT, GL_FALSE, 64, (void*)48);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else if ( m_contentType == ContentType_Color && m_vbo )
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
		glVertexAttribPointer(attributeId, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0 );
	}
	else
	{
		vsAssert(0, "Not yet implemented");
	}
}

void
vsRenderBuffer::BindAsTexture()
{
	GL_CHECK_SCOPED("BufferTexture");
	if ( m_contentType == ContentType_Float && m_vbo )
	{
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_bufferID);
	}
	else if ( m_contentType == ContentType_P )
	{
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_bufferID);
	}
	else if ( m_contentType == ContentType_Color )
	{
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_bufferID);
	}
	else if ( m_contentType == ContentType_UInt16 )
	{
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, m_bufferID);
	}
	else if ( m_contentType == ContentType_UInt32 )
	{
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, m_bufferID);
	}
	else if ( m_contentType == ContentType_I32Vec4 )
	{
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32I, m_bufferID);
	}
	else if ( m_contentType == ContentType_UI32Vec4 )
	{
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, m_bufferID);
	}
	else
	{
		vsAssert(0, "Not yet implemented");
	}
}

void
vsRenderBuffer::BindVertexBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_VertexArray, true );

	if ( m_vbo )
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
		glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, 0 );
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		vsRenderBuffer::BindVertexArray( state, m_array, m_arrayBytes/sizeof(vsVector3D) );
		// glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, m_array );
	}
}

void
vsRenderBuffer::UnbindVertexBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_VertexArray, BindType_ElementArray );
}

void
vsRenderBuffer::BindNormalBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_NormalArray, true );

	if ( m_vbo )
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
		glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, 0 );
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, m_array );
	}
}

void
vsRenderBuffer::UnbindNormalBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_NormalArray, false );
}

void
vsRenderBuffer::BindTexelBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );

	if ( m_vbo )
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
		glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, 0, 0 );
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		BindTexelArray( state, m_array, m_arrayBytes / sizeof(vsVector2D) );
		// glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, 0, m_array );
	}
}

void
vsRenderBuffer::UnbindTexelBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
}

void
vsRenderBuffer::BindColorBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_ColorArray, true );

	if ( m_vbo )
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
		glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_FLOAT, GL_FALSE, 0, 0 );
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_FLOAT, GL_FALSE, 0, m_array );
	}
}

void
vsRenderBuffer::UnbindColorBuffer( vsRendererState *state )
{
	state->SetBool( vsRendererState::ClientBool_ColorArray, false );
}

void
vsRenderBuffer::Bind( vsRendererState *state )
{
	switch( m_contentType )
	{
		case ContentType_P:
		{
			int stride = sizeof(P);

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
			}
			break;
		}
		case ContentType_PC:
		{
			PC dummyArray[2];
			int stride = sizeof(PC);
			size_t cStart = ((char*)&dummyArray[0].color.r - (char*)&dummyArray[0].position.x);
			GLvoid* cStartPtr = (GLvoid*)cStart;

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );
			state->SetBool( vsRendererState::ClientBool_ColorArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);

				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, cStartPtr );
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, &((PC*)m_array)[0].color );
			}
			break;
		}
		case ContentType_PT:
		{
			PT dummyArray[2];
			int stride = sizeof(PT);
			size_t tStart = (&dummyArray[0].texel.x - &dummyArray[0].position.x) * sizeof(float);
			GLvoid* tStartPtr = (GLvoid*)tStart;

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);

				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, tStartPtr );
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, &((PT*)m_array)[0].texel );
			}
			break;
		}
		case ContentType_PN:
		{
			PN dummyArray[2];
			int stride = sizeof(PN);
			size_t nStart = (&dummyArray[0].normal.x - &dummyArray[0].position.x) * sizeof(float);
			GLvoid* nStartPtr = (GLvoid*)nStart;

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );
			state->SetBool( vsRendererState::ClientBool_NormalArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);

				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, nStartPtr );

				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, &((PN*)m_array)[0].normal );
			}
			break;
		}
		case ContentType_PNT:
		{
			PNT dummyArray[2];
			int stride = sizeof(PNT);
			size_t nStart = (&dummyArray[0].normal.x - &dummyArray[0].position.x) * sizeof(float);
			size_t tStart = (&dummyArray[0].texel.x - &dummyArray[0].position.x) * sizeof(float);
			GLvoid* nStartPtr = (GLvoid*)nStart;
			GLvoid* tStartPtr = (GLvoid*)tStart;

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );
			state->SetBool( vsRendererState::ClientBool_NormalArray, true );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);

				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, tStartPtr );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, nStartPtr );

				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, &((PNT*)m_array)[0].texel );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, &((PNT*)m_array)[0].normal );
			}
			break;
		}
		case ContentType_PCNT:
		{
			PCNT dummyArray[2];
			int stride = sizeof(PCNT);
			size_t nStart = (&dummyArray[0].normal.x - &dummyArray[0].position.x) * sizeof(float);
			size_t tStart = (&dummyArray[0].texel.x - &dummyArray[0].position.x) * sizeof(float);
			size_t cStart = ((char*)&dummyArray[0].color.r - (char*)&dummyArray[0].position.x);
			GLvoid* cStartPtr = (GLvoid*)cStart;
			GLvoid* nStartPtr = (GLvoid*)nStart;
			GLvoid* tStartPtr = (GLvoid*)tStart;

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );
			state->SetBool( vsRendererState::ClientBool_NormalArray, true );
			state->SetBool( vsRendererState::ClientBool_ColorArray, true );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);

				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, tStartPtr );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, nStartPtr );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, cStartPtr );

				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, &((PCNT*)m_array)[0].texel );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, &((PCNT*)m_array)[0].normal );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, &((PCNT*)m_array)[0].color );
			}
			break;
		}
		case ContentType_PCN:
		{
			PCN dummyArray[2];
			int stride = sizeof(PCN);
			size_t nStart = (&dummyArray[0].normal.x - &dummyArray[0].position.x) * sizeof(float);
			size_t cStart = ((char*)&dummyArray[0].color.r - (char*)&dummyArray[0].position.x);
			GLvoid* cStartPtr = (GLvoid*)cStart;
			GLvoid* nStartPtr = (GLvoid*)nStart;

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );
			state->SetBool( vsRendererState::ClientBool_NormalArray, true );
			state->SetBool( vsRendererState::ClientBool_ColorArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);

				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, nStartPtr );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, cStartPtr );

				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
				glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, &((PCN*)m_array)[0].normal );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, &((PCN*)m_array)[0].color );
			}
			break;
		}
		case ContentType_PCT:
		{
			PCT dummyArray[2];
			int stride = sizeof(PCT);
			size_t cStart = ((char*)&dummyArray[0].color.r - (char*)&dummyArray[0].position.x);
			size_t tStart = (&dummyArray[0].texel.x - &dummyArray[0].position.x) * sizeof(float);
			GLvoid* cStartPtr = (GLvoid*)cStart;
			GLvoid* tStartPtr = (GLvoid*)tStart;

			state->SetBool( vsRendererState::ClientBool_VertexArray, true );
			state->SetBool( vsRendererState::ClientBool_ColorArray, true );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );

			if ( m_vbo )
			{
				glBindBuffer(GL_ARRAY_BUFFER, m_bufferID);

				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, 0 );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, tStartPtr );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, cStartPtr );

				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else
			{
				glVertexAttribPointer( POS_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, stride, m_array );
				glVertexAttribPointer( TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, stride, &((PCT*)m_array)[0].texel );
				glVertexAttribPointer( COLOR_ATTRIBUTE, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, &((PCT*)m_array)[0].color );
			}
			break;
		}
		default:
		{
			vsAssert(0, "Unknown content type!");
		}
	}
}

void
vsRenderBuffer::Unbind( vsRendererState *state )
{
	switch( m_contentType )
	{
        case ContentType_P:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
            break;
        case ContentType_PC:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
			state->SetBool( vsRendererState::ClientBool_ColorArray, false );
            break;
        case ContentType_PT:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
            break;
        case ContentType_PN:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
			state->SetBool( vsRendererState::ClientBool_NormalArray, false );
            break;
        case ContentType_PNT:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
			state->SetBool( vsRendererState::ClientBool_NormalArray, false );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
            break;
        case ContentType_PCN:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
			state->SetBool( vsRendererState::ClientBool_NormalArray, false );
			state->SetBool( vsRendererState::ClientBool_ColorArray, false );
            break;
        case ContentType_PCT:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
			state->SetBool( vsRendererState::ClientBool_ColorArray, false );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
            break;
        case ContentType_PCNT:
			state->SetBool( vsRendererState::ClientBool_VertexArray, false );
			state->SetBool( vsRendererState::ClientBool_NormalArray, false );
			state->SetBool( vsRendererState::ClientBool_ColorArray, false );
			state->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
            break;
        default:
            vsAssert(0, "Unknown content type!");
	}
}

int
vsRenderBuffer::GetPositionCount()
{
	switch( m_contentType )
	{
		case ContentType_P:
			return m_arrayBytes / sizeof(P);
			break;
		case ContentType_PC:
			return m_arrayBytes / sizeof(PC);
			break;
		case ContentType_PT:
			return m_arrayBytes / sizeof(PT);
			break;
		case ContentType_PN:
			return m_arrayBytes / sizeof(PN);
			break;
		case ContentType_PNT:
			return m_arrayBytes / sizeof(PNT);
			break;
		case ContentType_PCN:
			return m_arrayBytes / sizeof(PCN);
			break;
		case ContentType_PCT:
			return m_arrayBytes / sizeof(PCT);
			break;
		case ContentType_PCNT:
			return m_arrayBytes / sizeof(PCNT);
			break;
        default:
            vsAssert(0, "Unknown content type!");
	}
	return 0;
}

vsVector3D
vsRenderBuffer::GetPosition(int i)
{
	vsAssert( i < GetPositionCount(), "Illegal buffer position request!" );

	switch( m_contentType )
	{
		case ContentType_P:
		{
			P* p = (P*)m_array;
			return p[i].position;
		}
		case ContentType_PC:
		{
			PC* pc = (PC*)m_array;
			return pc[i].position;
		}
		case ContentType_PT:
		{
			PT* pt = (PT*)m_array;
			return pt[i].position;
		}
		case ContentType_PN:
		{
			PN* pn = (PN*)m_array;
			return pn[i].position;
		}
		case ContentType_PCT:
		{
			PCT* pc = (PCT*)m_array;
			return pc[i].position;
		}
		case ContentType_PCN:
		{
			PCN* pnc = (PCN*)m_array;
			return pnc[i].position;
		}
		case ContentType_PNT:
		{
			PNT* pnt = (PNT*)m_array;
			return pnt[i].position;
		}
		case ContentType_PCNT:
		{
			PCNT* pnct = (PCNT*)m_array;
			return pnct[i].position;
		}
		default:
		{
			vsAssert(0, "Unhandled vsRenderBuffer content type!");
		}
	}
	return vsVector3D::Zero;
}

vsVector3D
vsRenderBuffer::GetNormal(int i)
{
	vsAssert( i < GetPositionCount(), "Illegal buffer normal request!" );

	switch( m_contentType )
	{
		case ContentType_P:
		case ContentType_PC:
		case ContentType_PT:
		{
			return vsVector3D::One;
		}
		case ContentType_PN:
		{
			PN* pn = (PN*)m_array;
			return pn[i].normal;
		}
		case ContentType_PCN:
		{
			PCN* pnc = (PCN*)m_array;
			return pnc[i].normal;
		}
		case ContentType_PNT:
		{
			PNT* pnt = (PNT*)m_array;
			return pnt[i].normal;
		}
		case ContentType_PCNT:
		{
			PCNT* pnct = (PCNT*)m_array;
			return pnct[i].normal;
		}
		default:
		{
			vsAssert(0, "Unhandled vsRenderBuffer content type!");
		}
	}
	return vsVector3D::Zero;
}

vsVector2D
vsRenderBuffer::GetTexel(int i)
{
	vsAssert( i < GetPositionCount(), "Illegal buffer texel request!" );

	switch( m_contentType )
	{
		case ContentType_P:
		case ContentType_PC:
		case ContentType_PN:
		case ContentType_PCN:
		{
			return vsVector2D::Zero;
		}
		case ContentType_PT:
		{
			PT* pt = (PT*)m_array;
			return pt[i].texel;
		}
		case ContentType_PNT:
		{
			PNT* pnt = (PNT*)m_array;
			return pnt[i].texel;
		}
		case ContentType_PCNT:
		{
			PCNT* pnct = (PCNT*)m_array;
			return pnct[i].texel;
		}
		default:
		{
			vsAssert(0, "Unhandled vsRenderBuffer content type!");
		}
	}
	return vsVector2D::Zero;
}


vsColor
vsRenderBuffer::GetColor(int i)
{
	vsAssert( i < GetPositionCount(), "Illegal buffer color request!" );

	switch( m_contentType )
	{
		case ContentType_P:
		case ContentType_PT:
		case ContentType_PN:
		case ContentType_PNT:
		{
			return c_white;
		}
		case ContentType_PC:
		{
			PC* pc = (PC*)m_array;
			return pc[i].color;
		}
		case ContentType_PCN:
		{
			PCN* pnc = (PCN*)m_array;
			return pnc[i].color;
		}
		case ContentType_PCNT:
		{
			PCNT* pnct = (PCNT*)m_array;
			return pnct[i].color;
		}
		default:
		{
			vsAssert(0, "Unhandled vsRenderBuffer content type!");
		}
	}
	return c_white;
}


void
vsRenderBuffer::TriStripBuffer(int instanceCount)
{
	if ( m_vbo )
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);
		//glDrawElements(GL_TRIANGLE_STRIP, m_activeBytes/sizeof(int), GL_UNSIGNED_INT, 0);
		// glDrawElements(GL_TRIANGLE_STRIP, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, 0 );
		glDrawElementsInstanced(GL_TRIANGLE_STRIP, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, 0, instanceCount);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		// glDrawElements(GL_TRIANGLE_STRIP, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, m_array );
		DrawElementsImmediate( GL_TRIANGLE_STRIP, m_array, m_activeBytes/sizeof(uint16_t), instanceCount );
	}
}

void
vsRenderBuffer::TriListBuffer(int instanceCount)
{
	if ( m_vbo )
	{
		int elements = m_activeBytes/sizeof(uint16_t);
		// vsString prf;// = "TriListBuffer";
		// if ( elements <= 6 )
		// 	prf = "TriListBufferTiny";
		// else if ( elements > 600 )
		// 	prf = "TriListBufferLarge";
		// else
		// 	prf = "TriListBufferJustRight";
		// {
		// PROFILE_GL(prf);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);
		if ( instanceCount == 1 )
		{
			glDrawElements(GL_TRIANGLES, elements, GL_UNSIGNED_SHORT, 0);
		}
		else
		{
			glDrawElementsInstanced(GL_TRIANGLES, elements, GL_UNSIGNED_SHORT, 0, instanceCount);
		}
		// }
		//glDrawRangeElements(GL_TRIANGLES, 0, m_activeBytes/sizeof(uint16_t), m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, 0);
		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		// glDrawElements(GL_TRIANGLES, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, m_array );
		DrawElementsImmediate( GL_TRIANGLES, m_array, m_activeBytes/sizeof(uint16_t), instanceCount );
	}
}

void
vsRenderBuffer::TriFanBuffer(int instanceCount)
{
	if ( m_vbo )
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);
		// glDrawElements(GL_TRIANGLE_FAN, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, 0);
		glDrawElementsInstanced(GL_TRIANGLE_FAN, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, 0, instanceCount);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		// glDrawElements(GL_TRIANGLE_FAN, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, m_array );
		DrawElementsImmediate( GL_TRIANGLE_FAN, m_array, m_activeBytes/sizeof(uint16_t), instanceCount );
	}
}

void
vsRenderBuffer::LineStripBuffer(int instanceCount)
{
	if ( m_vbo )
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);
		glDrawElementsInstanced(GL_LINE_STRIP, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, 0, instanceCount);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		// glDrawElements(GL_LINE_STRIP, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, m_array );
		DrawElementsImmediate( GL_LINE_STRIP, m_array, m_activeBytes/sizeof(uint16_t), instanceCount );
	}
}

void
vsRenderBuffer::LineListBuffer(int instanceCount)
{
	if ( m_vbo )
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferID);
		glDrawElementsInstanced(GL_LINES, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, 0, instanceCount);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		// glDrawElements(GL_LINES, m_activeBytes/sizeof(uint16_t), GL_UNSIGNED_SHORT, m_array );
		DrawElementsImmediate( GL_LINES, m_array, m_activeBytes/sizeof(uint16_t), instanceCount );
	}
}

#define VBO_SIZE (1024 * 1024)
static GLuint g_vbo = 0xffffffff;
static int g_vboCursor = VBO_SIZE;

void
vsRenderBuffer::BindArrayToAttribute( void* buffer, size_t bufferSize, int attribute, int elementCount )
{
	vsAssert(bufferSize < VBO_SIZE, "Tried to bind too large an array for VBO_SIZE?");
	if ( g_vbo == 0xffffffff )
	{
		glGenBuffers(1, &g_vbo);
	}

	glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

	if ( g_vboCursor + bufferSize >= VBO_SIZE )
	{
		// This shouldn't happen any more;  we should always catch this in the
		// "EnsureSpaceForVertexColorTexelNormal()" function below.  But leave this
		// here just for safety for the moment.
		glBufferData(GL_ARRAY_BUFFER, VBO_SIZE, NULL, GL_DYNAMIC_DRAW);
		g_vboCursor = 0;
	}
	glBufferSubData(GL_ARRAY_BUFFER, g_vboCursor, bufferSize, buffer);

	glVertexAttribPointer( attribute, elementCount, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(g_vboCursor) );
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	g_vboCursor += bufferSize;
}

void
vsRenderBuffer::EnsureSpaceForVertexColorTexelNormal( int vertexCount, int colorCount, int texelCount, int normalCount )
{
	int bufferSize = vertexCount * sizeof(vsVector3D) +
		colorCount * sizeof(vsColor) +
		texelCount * sizeof(vsVector2D) +
		normalCount * sizeof(vsVector2D);

	if ( g_vboCursor + bufferSize >= VBO_SIZE )
	{
		vsAssert(bufferSize < VBO_SIZE, "Tried to bind too large an array for VBO_SIZE?");
		if ( g_vbo == 0xffffffff )
		{
			glGenBuffers(1, &g_vbo);
		}

		glBindBuffer(GL_ARRAY_BUFFER, g_vbo);

		// orphan the buffer and start on the new one.
		glBufferData(GL_ARRAY_BUFFER, VBO_SIZE, NULL, GL_DYNAMIC_DRAW);
		g_vboCursor = 0;
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void
vsRenderBuffer::BindVertexArray( vsRendererState *state, void* buffer, int vertexCount )
{
	int bufferSize = vertexCount * sizeof(vsVector3D);
	BindArrayToAttribute(buffer,bufferSize,POS_ATTRIBUTE,3);
}

void
vsRenderBuffer::BindColorArray( vsRendererState *state, void* buffer, int vertexCount )
{
	int bufferSize = vertexCount * sizeof(vsColor);
	BindArrayToAttribute(buffer,bufferSize,COLOR_ATTRIBUTE,4);
}

void
vsRenderBuffer::BindTexelArray( vsRendererState *state, void* buffer, int count )
{
	int bufferSize = count * sizeof(vsVector2D);
	BindArrayToAttribute(buffer,bufferSize,TEXCOORD_ATTRIBUTE,2);
}

void
vsRenderBuffer::BindNormalArray( vsRendererState *state, void* buffer, int count )
{
	int bufferSize = count * sizeof(vsVector3D);
	BindArrayToAttribute(buffer,bufferSize,NORMAL_ATTRIBUTE,3);
}

#define EVBO_SIZE (1024 * 1024)
static GLuint g_evbo = 0xffffffff;
static int g_evboCursor = EVBO_SIZE;

void
vsRenderBuffer::DrawElementsImmediate( int type, void* buffer, int count, int instanceCount )
{
	int bufferSize = count * sizeof(uint16_t);
	if ( g_evbo == 0xffffffff )
	{
		glGenBuffers(1, &g_evbo);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_evbo);

	if ( g_evboCursor + bufferSize >= EVBO_SIZE )
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, EVBO_SIZE, NULL, GL_STREAM_DRAW);
		g_evboCursor = 0;
	}
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, g_evboCursor, bufferSize, buffer);

	glDrawElementsInstanced(type, count, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(g_evboCursor), instanceCount );

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	g_evboCursor += bufferSize;
}

void*
vsRenderBuffer::BindRange(int startByte, int length)
{
	GL_CHECK_SCOPED("BindRange");
	if ( m_vbo )
	{
		if ( startByte + length > m_glArrayBytes )
		{
			// resize our array if necessary, and reset the buffer with the new size.
			ResizeArray_Internal( startByte + length );
			SetArray_Internal( m_array, startByte + length, BindType_Array );
		}
		int bindPoint = GL_ARRAY_BUFFER;
		glBindBuffer(bindPoint, m_bufferID);
		void *ptr = glMapBufferRange(bindPoint, startByte, length, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
		return ptr;
	}
	// otherwise, just give them a pointer into our array data
	return m_array + startByte;
}

void
vsRenderBuffer::UnbindRange( void* ptr )
{
	GL_CHECK_SCOPED("UnbindRange");
	if ( m_vbo )
	{
		int bindPoint = GL_ARRAY_BUFFER;
		glUnmapBuffer(bindPoint);
		glBindBuffer(bindPoint, 0);
	}
	// nothing to do, if no VBO.
}
