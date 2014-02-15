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

class vsRecord;
class vsStore;
class vsRenderBuffer;
class vsLight;
class vsMaterial;
class vsBox3D;

#define MAX_OWNED_MATERIALS (10)

class vsDisplayList
{
public:
	enum OpCode
	{
		OpCode_SetColor,
		OpCode_SetSpecularColor,
		OpCode_SetTexture,
		OpCode_ClearTexture,

		OpCode_MoveTo,
		OpCode_LineTo,
		OpCode_DrawPoint,
		OpCode_CompiledDisplayList,
		OpCode_PushTransform,
		OpCode_PushTranslation,
		OpCode_PushMatrix4x4,
		OpCode_SetMatrix4x4,
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

		OpCode_LineList,
		OpCode_LineStrip,
		OpCode_TriangleList,
		OpCode_TriangleStrip,
		OpCode_TriangleFan,

		OpCode_LineListBuffer,
		OpCode_LineStripBuffer,
		OpCode_TriangleStripBuffer,
		OpCode_TriangleListBuffer,
		OpCode_TriangleFanBuffer,

		OpCode_SetDrawMode,

		OpCode_SetMaterial,

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

		OpCode_Debug,

		OpCode_MAX
	};

	struct Data
	{
		uint32_t		i;
		vsVector3D	vector;
		vsBox2D		box2D;
		vsColor		color;
		vsTransform2D transform;
		vsLight		light;
		vsFog		fog;
		//vsMaterial	material;
		vsMatrix4x4 matrix4x4;
		vsString	string;
		float		fov;
		float		nearPlane;
		float		farPlane;
		char *p;

		void Set(uint32_t id) {i = id;}
		void Set(const vsBox2D & in) {box2D = in;}
		void Set(const vsVector3D & in) {vector = in;}
		void Set(const vsColor & in) {color = in;}
		void Set(const vsTransform2D &t) {transform = t;}
		//void Set(const vsMaterial &m) { material = m; }
		void Set(const vsMatrix4x4 &m, float fov_in, float nearPlane_in, float farPlane_in) {matrix4x4 = m; fov = fov_in; nearPlane = nearPlane_in; farPlane = farPlane_in;}
		void Set(const vsString &s) {string = s;}
		void SetPointer(char *pointer) {p = pointer;}

		uint32_t GetUInt() { return i; }
		vsBox2D GetBox2D() {return box2D;}
		vsVector3D GetVector3D() {return vector;}
		vsColor GetColor() {return color; }
		vsTransform2D	GetTransform() {return transform;}
		vsMatrix4x4 &	GetMatrix4x4() {return matrix4x4;}
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

	unsigned long	m_displayListId;
	bool			m_compiled;
	bool			m_ownFifo;

	vsDisplayList *	m_next;					// if we're compiled, then we'll be put into a linked list of compiled display lists.
	vsDisplayList *	m_prev;					// we use this linked list internally, so VectorStorm can recompile the display lists if our rendering context changes.  (For example, changing resolution or bitdepth)

	vsDisplayList *	m_instanceParent;		// if set, I'm an instance of this other vsDisplayList, and contain no actual data myself
	int				m_instanceCount;		// The number of instances that have been derived off of me.  If this value isn't zero, assert if someone tries to delete me.

	vsMaterial *	m_material[MAX_OWNED_MATERIALS];
	int				m_materialCount;

	int				m_mark;
	bool			m_colorSet;
	bool			m_modeSet;

	vsColor			m_cursorColor;
	vsColor			m_nextLineColor;
	vsVector3D		m_cursorPos;

	static bool		s_compiling;


//	int		GetSize() const;
//	int		GetSize() const { return m_instructionCount; }
//	int		GetFreeOps() const { return m_maxInstructions - m_instructionCount; }

//	op *	AddOp(OpCode type);

//	static vsDisplayList *	Load_CVec(const vsString &);
	static vsDisplayList *	Load_Vec( const vsString & );
	static vsDisplayList *	Load_Vec( vsRecord *record );
	static vsDisplayList *	Load_Obj(const vsString &);
	void					Write_CVec( const vsString &filename );

	// ====INTERNAL USE ONLY====
	static void	UncompileAll();
	static void	CompileAll();

	void	Uncompile_Internal();	//
	void	Compile_Internal();		//
	// ====INTERNAL USE ONLY====

public:

	// for use by vsFragment.
	static void	Load_Vec_SingleRecord( vsDisplayList *list, vsRecord *record );	// put the instruction from the record into the display list

	static vsDisplayList *	Load(const vsString &);
	static vsDisplayList *	Load( vsRecord *record );
	static vsDisplayList *	Compile(const vsString &);

			vsDisplayList();		// if no memory size is specified, we size dynamically, in 4kb chunks.
			vsDisplayList(size_t memSize);
//			vsDisplayList(const vsDisplayList &list);
	virtual	~vsDisplayList();

	vsStore *		GetFifo() { return m_fifo; }

	vsDisplayList *	CreateInstance();

	size_t		GetSize() const;
	size_t		GetMaxSize() const;
	void	Clear();
	void	Rewind();	// reset to the start of the display list

	void	Uncompile();	// uncompile this display list.		Shouldn't ever need to call this;  Compile() will call this automatically, if the list had previously been compiled.
	void	Compile();		// compile this display list.

	bool	IsCompiled() { return m_compiled; }

	void		Mark();		// crunch from the last mark to the current position.

	 //	The following functions do not draw primitives immediately;  instead, they add them to this display list.
	 // Instructions given using these functions will be carried out when this vsDisplayList is passed to a vsRenderer,
	 // using the DrawDisplayList() function.  (Normally, this happens automatically at the end of processing each frame)

	//void	SetTexture( vsTexture *t );
	void	SetColor( const vsColor &color );
	void	SetSpecularColor( const vsColor &color );
	void	MoveTo( const vsVector3D &pos );
	void	LineTo( const vsVector3D &pos );
	void	DrawPoint( const vsVector3D &pos );
	void	DrawLine( const vsVector3D &from, const vsVector3D &to );
	void	DrawCompiledDisplayList( unsigned int displayListId );
	void	PushTransform( const vsTransform2D &t );
	void	PushTransform( const vsTransform3D &t );
	void	PushTranslation( const vsVector3D &translation );
	void	PushMatrix4x4( const vsMatrix4x4 &m );
	void	SetMatrix4x4( const vsMatrix4x4 &m );
	void	SetWorldToViewMatrix4x4( const vsMatrix4x4 &m );
	void	PopTransform();
	void	SetCameraTransform( const vsTransform2D &t );	// no stack of camera transforms;  they an only be set absolutely!
	void	Set3DProjection( float fov, float nearPlane, float farPlane );
	void	SetProjectionMatrix4x4( const vsMatrix4x4 &m );

	void	VertexArray( vsVector2D *vertexArray, int vertexCount );	// will automatically be promoted into vsVertex3Ds
	void	VertexArray( vsVector3D *vertexArray, int vertexCount );
	void	NormalArray( vsVector3D *normalArray, int normalCount );
	void	TexelArray( vsVector2D *texelArray, int texelCount );
	void	ColorArray( vsColor *colorArray, int colorCount );

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

	void	LineList( int *idArray, int vertexCount );
	void	LineStrip( uint16_t *idArray, int vertexCount );
	void	LineStrip( int *idArray, int vertexCount );
	void	TriangleList( int *idArray, int vertexCount );
	void	TriangleStrip( int *idArray, int vertexCount );
	void	TriangleFan( int *idArray, int vertexCount );

	void	TriangleStripBuffer( vsRenderBuffer *buffer );
	void	TriangleListBuffer( vsRenderBuffer *buffer );
	void	TriangleFanBuffer( vsRenderBuffer *buffer );
	void	LineListBuffer( vsRenderBuffer *buffer );
	void	LineStripBuffer( vsRenderBuffer *buffer );

	//void	SetDrawMode( vsDrawMode mode );

	void	SetMaterial( vsMaterial *material );
	void	SetMaterial( vsMaterialInternal *material );

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

	void operator= ( const vsDisplayList &list ) { Clear(); Append(list); }


	vsDisplayList *	GetNextCompiled() { return m_next; }
	vsDisplayList *	GetPrevCompiled() { return m_prev; }
	void			AppendToCompiledList( vsDisplayList *next );
	void			ExtractFromCompiledList();

	static bool		IsCompiling() { return s_compiling; }

	friend class vsSystem;
};

#endif // VS_DISPLAYLIST
