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
#include "VS/Graphics/VS_Color.h"
#include "VS/Utils/VS_AutomaticInstanceList.h"

class vsColor;
class vsRendererState;


class vsRenderBuffer : public vsAutomaticInstanceList<vsRenderBuffer>
{
public:
    enum Type
    {
        Type_NoVBO,		// don't use a VBO for this buffer, even if they're supported.  (Use this if we're going to compile the data into a display list)

        Type_Static,		// we're going to set this only once, then render over and over again
        Type_Dynamic,		// we're going to change these values from time to time
        Type_Stream,		// we're going to write data into here once per render.

        TYPE_MAX
    };
private:

	char *			m_array;
	int				m_arrayBytes;
	int				m_glArrayBytes;

	int				m_activeBytes;

	Type	        m_type;

	enum ContentType
	{
		ContentType_Custom,
		ContentType_PC,
		ContentType_PT,
		ContentType_PN,
		ContentType_PCN,
		ContentType_PCT,
		ContentType_PNT,
		ContentType_PCNT
	};
	ContentType		m_contentType;

	unsigned int	m_bufferID;
	bool			m_vbo;
	bool			m_indexType;

	void	SetArray_Internal( char *data, int bytes, bool elementArray );
	void	SetArraySize_Internal( int bytes );

public:

	struct PC
	{
		vsVector3D		position;
		vsColor			color;
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
		vsColor			color;
		vsVector2D		texel;			// total:  36 bytes.
		float			padding[7];		// seven more floats brings us up to 64 bytes.
	};

	struct PCN
	{
		vsVector3D		position;
		vsVector3D		normal;
		vsColor			color;			// total:  40 bytes.  Eew, we're supposed to be a multiple of 32, ideally
		float			padding[6];		// six more floats brings us up to 64 bytes.
	};

	struct PCNT
	{
		vsVector3D		position;
		vsVector3D		normal;
		vsColor			color;
		vsVector2D		texel;			// total:  48 bytes.
		float			padding[4];		// four more floats brings us to 64 bytes.
	};


	static void UnmapAll();
	static void MapAll();

	vsRenderBuffer(Type type = Type_Static);
	~vsRenderBuffer();

	void	SetArray( const PC *array, int size );
	void	SetArray( const PT *array, int size );
	void	SetArray( const PN *array, int size );
	void	SetArray( const PNT *array, int size );
	void	SetArray( const PCN *array, int size );
	void	SetArray( const PCT *array, int size );
	void	SetArray( const PCNT *array, int size );
	void	SetArray( const vsVector3D *array, int size );
	void	SetArray( const vsVector2D *array, int size );
	void	SetArray( const vsColor *array, int size );
	void	SetArray( const uint16 *array, int size );
    void    ResizeArray( int size );

	void	SetActiveSize( int size ) { m_activeBytes = size; }

	void			SetVector3DArraySize( int size );
	int				GetVector3DArraySize() { return m_arrayBytes/sizeof(vsVector3D); }
	vsVector3D *	GetVector3DArray() { return (vsVector3D*)m_array; }

	void			SetIntArraySize( int size );
	int				GetIntArraySize() { return m_arrayBytes/sizeof(uint16); }
	uint16 *		GetIntArray() { return (uint16*)m_array; }

	void			SetColorArraySize( int size );
	vsColor *		GetColorArray() { return (vsColor*)m_array; }
	void			SetVector2DArraySize( int size );
	vsVector2D *	GetVector2DArray() { return (vsVector2D*)m_array; }

	PCT *			GetPCTArray() { return (PCT*)m_array; }
	PCNT *			GetPCNTArray() { return (PCNT*)m_array; }

	void	BakeArray();	// bake any modified array values into our GPU-based storage, if any.
	void	BakeIndexArray();	// bake any modified array values into our GPU-based storage, if any.

	int		GetPositionCount();		// for new-style buffers, which don't offer direct array access.
	vsVector3D	GetPosition(int i);

	void	BindVertexBuffer( vsRendererState *state );
	void	UnbindVertexBuffer( vsRendererState *state );

	void	BindNormalBuffer( vsRendererState *state );
	void	UnbindNormalBuffer( vsRendererState *state );

	void	BindTexelBuffer( vsRendererState *state );
	void	UnbindTexelBuffer( vsRendererState *state );

	void	BindColorBuffer( vsRendererState *state );
	void	UnbindColorBuffer( vsRendererState *state );

	void	Bind( vsRendererState *state );		// for non-custom types
	void	Unbind( vsRendererState *state );	// for non-custom types

	void	TriStripBuffer();
	void	TriListBuffer();
	void	TriFanBuffer();
	void	LineStripBuffer();
	void	LineListBuffer();
};


#endif // VS_GPU_BUFFER_H

