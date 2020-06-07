/*
 *  VS_DisplayList.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_DISPLAYLIST_H
#define VS_DISPLAYLIST_H

#include "VS/Math/VS_Angle.h"
#include "VS/Math/VS_Box.h"
#include "VS/Math/VS_Matrix.h"
#include "VS/Math/VS_Vector.h"
#include "VS/Math/VS_Transform.h"

#include "VS/Graphics/VS_Color.h"
#include "VS/Graphics/VS_Fog.h"
#include "VS/Graphics/VS_Light.h"
#include "VS/Graphics/VS_Material.h"

#include "VS/Utils/VS_Array.h"

class vsRecord;
class vsStore;
class vsRenderBuffer;
class vsLight;
class vsMaterial;
class vsBox3D;
class vsShaderValues;

#define MAX_OWNED_MATERIALS (10)

class vsDisplayList
{
public:
	enum OpCode
	{
		OpCode_SetColor,
		OpCode_SetColors,
		OpCode_SetColorsBuffer,

		OpCode_PushTransform,
		OpCode_PushTranslation,
		OpCode_PushMatrix4x4,
		OpCode_SetMatrix4x4,
		OpCode_SetMatrices4x4,
		OpCode_SetMatrices4x4Buffer,
		OpCode_SetWorldToViewMatrix4x4,
		OpCode_PopTransform,
		OpCode_SetCameraTransform,
		OpCode_Set3DProjection,
		OpCode_SetProjectionMatrix4x4,

		OpCode_VertexArray,
		OpCode_NormalArray,
		OpCode_TexelArray,
		OpCode_ColorArray,

		OpCode_VertexBuffer,
		OpCode_NormalBuffer,
		OpCode_TexelBuffer,
		OpCode_ColorBuffer,

		OpCode_BindBuffer,
		OpCode_UnbindBuffer,

		OpCode_ClearVertexArray,
		OpCode_ClearNormalArray,
		OpCode_ClearTexelArray,
		OpCode_ClearColorArray,
		OpCode_ClearArrays,	// clear them all!

		OpCode_LineListArray,
		OpCode_LineStripArray,
		OpCode_TriangleListArray,
		OpCode_TriangleStripArray,
		OpCode_TriangleFanArray,
		OpCode_PointsArray,

		OpCode_LineListBuffer,
		OpCode_LineStripBuffer,
		OpCode_TriangleStripBuffer,
		OpCode_TriangleListBuffer,
		OpCode_TriangleFanBuffer,

		OpCode_SetMaterial,
		OpCode_SetRenderTarget,
		OpCode_ClearRenderTarget,
		OpCode_ClearRenderTargetColor,
		OpCode_ResolveRenderTarget,
		OpCode_BlitRenderTarget,

		OpCode_Light,
		OpCode_ClearLights,

		OpCode_Fog,
		OpCode_ClearFog,

		OpCode_FlatShading,
		OpCode_SmoothShading,

		OpCode_EnableStencil,
		OpCode_DisableStencil,
		OpCode_ClearStencil,

		OpCode_EnableScissor,
		OpCode_DisableScissor,

		OpCode_SetViewport,
		OpCode_ClearViewport,

		OpCode_SnapMatrix, // snaps localToWorld matrix from wherever it is to pixels, assuming ortho projection.  Counts as a matrix push.

		OpCode_SetShaderValues, // set supplementary shader values which may be used by any current shader

		OpCode_Debug,

		OpCode_MAX
	};

	struct Data
	{
		uint32_t i;
		vsVector3D vector;
		vsBox2D box2D;
		vsColor color;
		vsTransform2D transform;
		vsLight light;
		vsFog fog;
		vsMatrix4x4 matrix4x4;
		vsString string;
		float fov;
		float nearPlane;
		float farPlane;
		char *p;
		char *p2;

		void Set(uint32_t id) {i = id;}
		void Set(const vsBox2D & in) {box2D = in;}
		void Set(const vsVector3D & in) {vector = in;}
		void Set(const vsColor & in) {color = in;}
		void Set(const vsTransform2D &t) {transform = t;}
		void Set(const vsMatrix4x4 &m, float fov_in, float nearPlane_in, float farPlane_in) {matrix4x4 = m; fov = fov_in; nearPlane = nearPlane_in; farPlane = farPlane_in;}
		void Set(const vsString &s) {string = s;}
		void SetPointer(char *pointer) {p = pointer;}
		void SetPointer2(char *pointer) {p2 = pointer;}

		uint32_t GetUInt() { return i; }
		vsBox2D GetBox2D() {return box2D;}
		vsVector3D GetVector3D() {return vector;}
		vsColor GetColor() {return color; }
		vsTransform2D GetTransform() {return transform;}
		vsMatrix4x4 & GetMatrix4x4() {return matrix4x4;}
		vsString GetString() {return string;}
	};

	struct op
	{
		OpCode	type;
		Data	data;
	};

private:

	vsStore *	m_fifo;

	op  m_currentOp;

	vsDisplayList *	m_instanceParent;		// if set, I'm an instance of this other vsDisplayList, and contain no actual data myself
	int				m_instanceCount;		// The number of instances that have been derived off of me.  If this value isn't zero, assert if someone tries to delete me.

	vsMaterial *	m_material[MAX_OWNED_MATERIALS];
	int				m_materialCount;

	bool			m_colorSet;

	vsColor			m_cursorColor;
	vsColor			m_nextLineColor;
	vsVector3D		m_cursorPos;


	static vsDisplayList *	Load_Vec( const vsString & );
	static vsDisplayList *	Load_Vec( vsRecord *record );
	static vsDisplayList *	Load_Obj(const vsString &);
	void					Write_CVec( const vsString &filename );

public:

	// for use by vsFragment.
	static void	Load_Vec_SingleRecord( vsDisplayList *list, vsRecord *record );	// put the instruction from the record into the display list

	static vsDisplayList *	Load(const vsString &);
	static vsDisplayList *	Load( vsRecord *record );

			vsDisplayList();		// if no memory size is specified, we size dynamically, in 4kb chunks.
			vsDisplayList(size_t memSize);
	virtual	~vsDisplayList();

	vsStore *		GetFifo() { return m_fifo; }

	vsDisplayList *	CreateInstance();

	size_t		GetSize() const;
	size_t		GetMaxSize() const;
	void	Clear();
	void	Rewind();	// reset to the start of the display list

	 //	The following functions do not draw primitives immediately;  instead, they add them to this display list.
	 // Instructions given using these functions will be carried out when this vsDisplayList is passed to a vsRenderer,
	 // using the DrawDisplayList() function.  (Normally, this happens automatically at the end of processing each frame)

	void	SetColor( const vsColor &color );
	void	MoveTo( const vsVector3D &pos );
	void	LineTo( const vsVector3D &pos );
	void	DrawPoint( const vsVector3D &pos );
	void	DrawLine( const vsVector3D &from, const vsVector3D &to );
	void	PushTransform( const vsTransform2D &t );
	void	PushTransform( const vsTransform3D &t );
	void	PushTranslation( const vsVector3D &translation );
	void	PushMatrix4x4( const vsMatrix4x4 &m );
	void	SetMatrix4x4( const vsMatrix4x4 &m );
	void	SetMatrices4x4( const vsMatrix4x4 *m, int count );
	void	SetColors( const vsColor *c, int count );
	void	SetColorsBuffer( const vsRenderBuffer *buffer );
	void	SetMatrices4x4Buffer( vsRenderBuffer *buffer );
	void	SnapMatrix();
	void	SetShaderValues( vsShaderValues *values );
	void	SetWorldToViewMatrix4x4( const vsMatrix4x4 &m );
	void	PopTransform();
	void	SetCameraTransform( const vsTransform2D &t );	// no stack of camera transforms;  they an only be set absolutely!
	void	Set3DProjection( float fov, float nearPlane, float farPlane );
	void	SetProjectionMatrix4x4( const vsMatrix4x4 &m );

	void	VertexArray( const vsVector2D *vertexArray, int vertexCount );	// will automatically be promoted into vsVertex3Ds
	void	VertexArray( const vsVector3D *vertexArray, int vertexCount );
	void	NormalArray( const vsVector3D *normalArray, int normalCount );
	void	TexelArray( const vsVector2D *texelArray, int texelCount );
	void	ColorArray( const vsColor *colorArray, int colorCount );

	void	VertexBuffer( vsRenderBuffer *buffer );	// old-style.  Buffer doesn't know what it contains;   different calls on display list to specify what's in the buffer.
	void	NormalBuffer( vsRenderBuffer *buffer );
	void	TexelBuffer( vsRenderBuffer *buffer );
	void	ColorBuffer( vsRenderBuffer *buffer );

	void	BindBuffer( vsRenderBuffer *buffer );		// new-style.  Buffer knows what it contains, binds itself
	void	UnbindBuffer( vsRenderBuffer *buffer );

	void	ClearVertexArray();
	void	ClearNormalArray();
	void	ClearTexelArray();
	void	ClearColorArray();
	void	ClearBuffers() { ClearArrays(); }
	void	ClearArrays();

	void	LineListArray( int *idArray, int vertexCount );
	void	LineStripArray( uint16_t *idArray, int vertexCount );
	void	LineStripArray( int *idArray, int vertexCount );
	void	TriangleListArray( int *idArray, int vertexCount );
	void	TriangleStripArray( int *idArray, int vertexCount );
	void	TriangleFanArray( int *idArray, int vertexCount );
	void	PointsArray( int *idArray, int vertexCount );

	void	TriangleStripBuffer( vsRenderBuffer *buffer );
	void	TriangleListBuffer( vsRenderBuffer *buffer );
	void	TriangleFanBuffer( vsRenderBuffer *buffer );
	void	LineListBuffer( vsRenderBuffer *buffer );
	void	LineStripBuffer( vsRenderBuffer *buffer );

	void	SetMaterial( vsMaterial *material );
	void	SetRenderTarget( vsRenderTarget *target );
	void	ClearRenderTarget(); // clears the currently set render target.
	void	ClearRenderTargetColor(const vsColor& c); // clears the currently set render target to a specific color.
	void	ResolveRenderTarget( vsRenderTarget *target );
	void	BlitRenderTarget( vsRenderTarget *from, vsRenderTarget *to );

	void	Light( const vsLight &light );
	void	ClearLights();

	void	Fog( const vsFog &light );
	void	ClearFog();

	void	FlatShading();
	void	SmoothShading();

	void	GetBoundingCircle( vsVector2D &center, float &radius );
	float	GetBoundingRadius();
	void	GetBoundingBox( vsVector2D &topLeft, vsVector2D &bottomRight );
	void	GetBoundingBox( vsBox3D &box );

	struct Triangle
	{
		vsVector3D vert[3];
	};
	int	GetTriangles(vsArray<struct Triangle>& result);

	struct Stats
	{
		int drawCount;
		int vertexCount;
		int triangleCount;

		Stats& operator+=(const Stats& o)
		{
			drawCount += o.drawCount;
			vertexCount += o.vertexCount;
			triangleCount += o.triangleCount;
			return *this;
		}
	};
	struct Stats CalculateStats();

	void	ApplyOffset(const vsVector2D &offset);

	void	Append( const vsDisplayList &list );	// appends the passed display list onto us.

	void	EnableStencil();   // turns on stencil testing, cull future rendering to INSIDE stencil
	void	DisableStencil();  // turns off stencil testing;  no stencils considered.
	void	ClearStencil();    // clears our stencil so that everything fails.

	void	EnableScissor(const vsBox2D &box);   // turns on scissoring, inside this box.
	void	DisableScissor();                    // turns off scissoring.

	// SetViewport requires 'box' be expressed in [0..1] for X and Y, relative
	// to screen resolution.
	void	SetViewport( const vsBox2D &box );
	void	ClearViewport();

	// Debug will cause the passed message to be printed to the log when the
	// renderer hits it.
	// Can be useful for debugging renderer commands.
	void	Debug(const vsString &message);

	OpCode	PeekOpType();
	op *	PopOp();
	void	AppendOp(op *);

	static const vsString& GetOpCodeString( OpCode code );

	void operator= ( const vsDisplayList &list ) { Clear(); Append(list); }
};

#endif // VS_DISPLAYLIST
