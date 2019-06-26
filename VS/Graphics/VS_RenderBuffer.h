/*
 *  VS_RenderBuffer.h
 *  Lord
 *
 *  Created by Trevor Powell on 5/01/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_GPU_BUFFER_H
#define VS_GPU_BUFFER_H

#include "VS/Math/VS_Vector.h"
#include "VS/Math/VS_Matrix.h"
#include "VS/Graphics/VS_Color.h"
#include "VS/Utils/VS_AutomaticInstanceList.h"

class vsAttributeState;

struct vsVector4D_i32
{
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t w;
};
struct vsVector4D_ui32
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t w;
};

class vsRenderBuffer
{
public:
	enum BindType
	{
		BindType_Array,
		BindType_ElementArray,
		BindType_TextureBuffer,
		BindType_MAX
	};
	enum Type
	{
		Type_NoVBO,		// don't use a VBO for this buffer, even if they're supported.  (Use this if we're going to compile the data into a display list)

		Type_Static,		// we're going to set this only once, then render over and over again
		Type_Dynamic,		// we're going to change these values from time to time
		Type_Stream,		// we're going to write data into here once per render.

		TYPE_MAX
	};
	enum ContentType
	{
		ContentType_Custom,
		ContentType_P,
		ContentType_PC,
		ContentType_PT,
		ContentType_PN,
		ContentType_PCN,
		ContentType_PCT,
		ContentType_PNT,
		ContentType_PCNT,
		ContentType_Matrix,
		ContentType_Color,
		ContentType_Float,
		ContentType_UInt16,
		ContentType_UInt32,
		ContentType_I32Vec4,
		ContentType_UI32Vec4
	};
private:

	char *			m_array;
	int				m_arrayBytes;
	int				m_glArrayBytes;

	int				m_activeBytes;

	Type	        m_type;

	ContentType		m_contentType;

	unsigned int	m_bufferID;
	bool			m_vbo;
	BindType		m_bindType;

	void	SetArray_Internal( char *data, int bytes, BindType bindType);
	void	SetArraySize_Internal( int bytes );
	void	ResizeArray_Internal( int bytes ); // like the above, but retain saved array data.

public:

	struct P
	{
		vsVector3D		position;
	};

	struct PC
	{
		vsVector3D		position;
		vsColorPacked			color;
		float			padding[1];		// total:  32 bytes.
	};

	struct PT
	{
		vsVector3D		position;
		vsVector2D		texel;			// total:  20 bytes
		float			padding[3];		// three more floats brings us to 32 bytes
	};

	struct PN
	{
		vsVector3D		position;
		vsVector3D		normal;			// total:  24 bytes
		float			padding[2];		// two more floats brings us to 32 bytes
	};

	struct PNT
	{
		vsVector3D		position;
		vsVector3D		normal;
		vsVector2D		texel;			// total:  32 bytes.
	};

	struct PCT
	{
		vsVector3D		position;
		vsColorPacked			color;
		vsVector2D		texel;			// total:  36 bytes.
		float			padding[7];		// seven more floats brings us up to 64 bytes.
	};

	struct PCN
	{
		vsVector3D		position;
		vsVector3D		normal;
		vsColorPacked			color;			// total:  40 bytes.  Eew, we're supposed to be a multiple of 32, ideally
		float			padding[6];		// six more floats brings us up to 64 bytes.
	};

	struct PCNT
	{
		vsVector3D		position;
		vsVector3D		normal;
		vsColorPacked			color;
		vsVector2D		texel;			// total:  48 bytes.
		float			padding[4];		// four more floats brings us to 64 bytes.
	};


	vsRenderBuffer(Type type = Type_Static);
	~vsRenderBuffer();

	void	SetArray( const P *array, int size );
	void	SetArray( const PC *array, int size );
	void	SetArray( const PT *array, int size );
	void	SetArray( const PN *array, int size );
	void	SetArray( const PNT *array, int size );
	void	SetArray( const PCN *array, int size );
	void	SetArray( const PCT *array, int size );
	void	SetArray( const PCNT *array, int size );
	void	SetArray( const vsMatrix4x4 *array, int size );
	void	SetArray( const vsVector3D *array, int size );
	void	SetArray( const vsVector2D *array, int size );
	void	SetArray( const vsColor *array, int size );
	void	SetArray( const uint16_t *array, int size );
	void	SetArray( const uint32_t *array, int size );
	void	SetArray( const float *array, int size );
	void	SetArray( const vsVector4D_ui32 *array, int size );
	void	SetArray( const vsVector4D_i32 *array, int size );
    void    ResizeArray( int size );

	void	SetActiveSize( int size );

	void			SetVector3DArraySize( int size );
	int				GetVector3DArraySize() { return m_arrayBytes/sizeof(vsVector3D); }
	vsVector3D *	GetVector3DArray() { return (vsVector3D*)m_array; }

	void			SetIntArraySize( int size );
	int				GetIntArraySize() { return m_arrayBytes/sizeof(uint16_t); }
	uint16_t *		GetIntArray() { return (uint16_t*)m_array; }

	void			SetColorArraySize( int size );
	vsColor *		GetColorArray() { return (vsColor*)m_array; }
	void			SetVector2DArraySize( int size );
	vsVector2D *	GetVector2DArray() { return (vsVector2D*)m_array; }

	void*			GetGenericArray() { return m_array; }

	int				GetMatrix4x4ArraySize() { return m_arrayBytes/sizeof(vsMatrix4x4); }
	int				GetActiveMatrix4x4ArraySize() { return m_activeBytes/sizeof(vsMatrix4x4); }

	P *				GetPArray() { return (P*)m_array; }
	PN *			GetPNArray() { return (PN*)m_array; }
	PC *			GetPCArray() { return (PC*)m_array; }
	PT *			GetPTArray() { return (PT*)m_array; }
	PCT *			GetPCTArray() { return (PCT*)m_array; }
	PCN *			GetPCNArray() { return (PCN*)m_array; }
	PNT *			GetPNTArray() { return (PNT*)m_array; }
	PCNT *			GetPCNTArray() { return (PCNT*)m_array; }

	void	BakeArray();	// bake any modified array values into our GPU-based storage, if any.
	void	BakeIndexArray();	// bake any modified array values into our GPU-based storage, if any.

	int		GetPositionCount();		// for new-style buffers, which don't offer direct array access.
	vsVector3D	GetPosition(int i);
	vsVector3D	GetNormal(int i);
	vsVector2D	GetTexel(int i);
	vsColor		GetColor(int i);

	// Probably only useful for santity checking that the correct drawing
	// functions are being called, for our known buffer types.
	ContentType	GetContentType() { return m_contentType; }

	void	BindAsAttribute( int attributeId );
	void	BindAsAttribute( int attributeId, int size, int stride = 0, void* offset = NULL );
	void	BindAsTexture();

	void	BindVertexBuffer( vsAttributeState *state );
	void	UnbindVertexBuffer( vsAttributeState *state );

	void	BindNormalBuffer( vsAttributeState *state );
	void	UnbindNormalBuffer( vsAttributeState *state );

	void	BindTexelBuffer( vsAttributeState *state );
	void	UnbindTexelBuffer( vsAttributeState *state );

	void	BindColorBuffer( vsAttributeState *state );
	void	UnbindColorBuffer( vsAttributeState *state );

	void	Bind( vsAttributeState *state );		// for non-custom types
	void	Unbind( vsAttributeState *state );	// for non-custom types

	static void EnsureSpaceForVertexColorTexelNormal( int vertexCount, int colorCount, int texelCount, int normalCount );
	static void BindArrayToAttribute( void* buffer, size_t bufferSize, int attribute, int elementCount );
	static void BindVertexArray( vsAttributeState *state, void* buffer, int vertexCount );
	static void BindColorArray( vsAttributeState *state, void* buffer, int vertexCount );
	static void BindTexelArray( vsAttributeState *state, void* buffer, int vertexCount );
	static void BindNormalArray( vsAttributeState *state, void* buffer, int vertexCount );

	static void DrawElementsImmediate( int type, void* buffer, int count, int instanceCount );

	void	TriStripBuffer(int instanceCount);
	void	TriListBuffer(int instanceCount);
	void	TriFanBuffer(int instanceCount);
	void	LineStripBuffer(int instanceCount);
	void	LineListBuffer(int instanceCount);

	const bool IsVBO() { return m_vbo; }


	// Advanced interface;  TODO is to figure out whether there's a nicer
	// way to provide this kind of functionality.

	void* BindRange(int startByte, int length);
	void UnbindRange( void* ptr );
};


#endif // VS_GPU_BUFFER_H

