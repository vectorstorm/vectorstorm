/*
 *  VS_DisplayList.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_DisplayList.h"
#include "VS_File.h"
#include "VS_Record.h"
#include "VS_Store.h"

#include "VS_Box.h"
#include "VS_Light.h"
#include "VS_MaterialInternal.h"
#include "VS_RenderBuffer.h"
#include "VS_Screen.h"
#include "VS_Shader.h"
#include "VS_System.h"
#include "VS_Texture.h"
#include "VS_TextureManager.h"

#if !TARGET_OS_IPHONE
#include "SDL2/SDL_opengl.h"
#endif

static vsString g_opCodeName[vsDisplayList::OpCode_MAX] =
{
	"SetColor",
	"SetColors",

	"PushTransform",
	"PushTranslation",
	"PushMatrix4x4",
	"SetMatrix4x4",
	"SetMatrices4x4",
	"SetMatrices4x4Buffer",
	"SetWorldToViewMatrix4x4",
	"PopTransform",
	"SetCameraTransform",
	"Set3DProjection",
	"SetProjectionMatrix4x4",

	"VertexArray",
	"NormalArray",
	"TexelArray",
	"ColorArray",

	"VertexBuffer",
	"NormalBuffer",
	"TexelBuffer",
	"ColorBuffer",

	"BindBuffer",
	"UnbindBuffer",

	"ClearVertexArray",
	"ClearNormalArray",
	"ClearTexelArray",
	"ClearColorArray",
	"ClearArrays",

	"LineListArray",
	"LineStripArray",
	"TriangleListArray",
	"TriangleStripArray",
	"TriangleFanArray",
	"PointsArray",

	"LineListBuffer",
	"LineStripBuffer",
	"TriangleStripBuffer",
	"TriangleListBuffer",
	"TriangleFanBuffer",

	"SetMaterial",
	"SetRenderTarget",
	"ClearRenderTarget",
	"ResolveRenderTarget",
	"BlitRenderTarget",

	"Light",
	"ClearLights",

	"Fog",
	"ClearFog",

	"FlatShading",
	"SmoothShading",

	"EnableStencil",
	"DisableStencil",
	"ClearStencil",

	"EnableScissor",
	"DisableScissor",

	"SetViewport",
	"ClearViewport",

	"SnapMatrix",

	"Debug"
};

const vsString&
vsDisplayList::GetOpCodeString( OpCode code )
{
	return g_opCodeName[code];
}

vsDisplayList *
vsDisplayList::Load_Vec( const vsString &filename )
{
	vsDisplayList *loader = new vsDisplayList(1024 * 50);	// 50k should be enough to load in just about anything.  (famous  last words)

	vsFile *file = new vsFile(filename + vsString(".vec"));
	vsRecord r;

	vsMaterial *materialList[MAX_OWNED_MATERIALS];
	int materialCount = 0;

	while( file->Record(&r) )
	{
		Load_Vec_SingleRecord( loader, &r );
	}

	delete file;

	vsAssert(loader->GetSize() > 0.f, "Didn't get any operations in a loaded display list!" )
	vsDisplayList *result = new vsDisplayList( loader->GetSize() );
	for ( int i = 0; i < materialCount; i++ )
	{
		result->m_material[i] = materialList[i];
		result->m_materialCount = materialCount;
	}
	result->Append(*loader);
	delete loader;

	return result;
}

vsDisplayList *
vsDisplayList::Load_Vec( vsRecord *record )
{
	vsDisplayList *loader = new vsDisplayList(1024 * 50);	// 50k should be enough to load in just about anything.  (famous  last words)

	for( int i = 0; i < record->GetChildCount(); i++ )
	{
		vsRecord *r = record->GetChild(i);
		Load_Vec_SingleRecord( loader, r );
	}

	vsAssert(loader->GetSize() > 0.f, "Didn't get any operations in a loaded display list!" )
	vsDisplayList *result = new vsDisplayList( loader->GetSize() );

	result->Append(*loader);
	delete loader;

	return result;
}

void
vsDisplayList::Load_Vec_SingleRecord( vsDisplayList *loader, vsRecord *r )
{
	vsString label = r->GetLabel().AsString();

	for ( int i = 0; i < OpCode_MAX; i++ )
	{
		if ( label == g_opCodeName[i] )
		{
			OpCode code = (OpCode)i;

			switch(code)
			{
				case OpCode_SetColor:
				{
					vsColor color = r->Color();
					loader->SetColor(color);
					break;
				}
				case OpCode_VertexArray:
				{
					int arrayCount = r->GetChildCount();
					if ( arrayCount )
					{
						vsVector3D *va = new vsVector3D[arrayCount];
						vsRecord subRecord;

						for ( int id = 0; id < arrayCount; id++ )
						{
							vsRecord *s = r->GetChild(id);
							if ( s->GetTokenCount() == 3 )
								va[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
							else
								va[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), 0.f);
						}

						loader->VertexArray(va, arrayCount);
						delete []va;
					}
					break;
				}
				case OpCode_NormalArray:
				{
					int arrayCount = r->GetChildCount();
					if ( arrayCount )
					{
						vsVector3D *va = new vsVector3D[arrayCount];
						vsRecord subRecord;

						for ( int id = 0; id < arrayCount; id++ )
						{
							vsRecord *s = r->GetChild(id);
							if ( s->GetTokenCount() == 3 )
								va[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat());
							else
								va[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), 0.f);
						}

						loader->NormalArray(va, arrayCount);
						delete []va;
					}
					break;
				}
				case OpCode_ClearVertexArray:
				{
					loader->ClearVertexArray();
					break;
				}
				case OpCode_TexelArray:
				{
					int arrayCount = r->GetChildCount();
					vsVector2D *va = new vsVector2D[arrayCount];

					for ( int id = 0; id < arrayCount; id++ )
					{
						vsRecord *s = r->GetChild(id);
						va[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat());
					}

					loader->TexelArray(va, arrayCount);
					delete []va;
					break;
				}
				case OpCode_ClearTexelArray:
				{
					loader->ClearTexelArray();
					break;
				}
				case OpCode_ColorArray:
				{
					int arrayCount = r->GetChildCount();
					vsColor *ca = new vsColor[arrayCount];

					for ( int id = 0; id < arrayCount; id++ )
					{
						vsRecord *s = r->GetChild(id);
						ca[id].Set(s->GetToken(0).AsFloat(), s->GetToken(1).AsFloat(), s->GetToken(2).AsFloat(), s->GetToken(3).AsFloat() );
					}

					loader->ColorArray(ca, arrayCount);
					delete [] ca;
					break;
				}
				case OpCode_ClearColorArray:
				{
					loader->ClearColorArray();
					break;
				}
				case OpCode_ClearArrays:
				{
					loader->ClearArrays();
					break;
				}
				case OpCode_Light:
				{
					vsLight light;
					loader->Light(light);
					break;
				}
				case OpCode_ClearLights:
				{
					loader->ClearLights();
					break;
				}
				case OpCode_LineListArray:
				case OpCode_LineStripArray:
				case OpCode_TriangleListArray:
				case OpCode_TriangleStripArray:
				case OpCode_TriangleFanArray:
				case OpCode_PointsArray:
				{
					int arrayCount = r->GetTokenCount();
					int *idarray = new int[arrayCount];

					for ( int id = 0; id < arrayCount; id++ )
					{
						idarray[id] = r->GetToken(id).AsInteger();
					}

					if ( code == OpCode_LineListArray )
						loader->LineListArray(idarray, arrayCount);
					if ( code == OpCode_LineStripArray )
						loader->LineStripArray(idarray, arrayCount);
					if ( code == OpCode_TriangleListArray )
						loader->TriangleListArray(idarray, arrayCount);
					if ( code == OpCode_TriangleStripArray )
						loader->TriangleStripArray(idarray, arrayCount);
					if ( code == OpCode_TriangleFanArray )
						loader->TriangleFanArray(idarray, arrayCount);

					delete [] idarray;

					break;
				}
				case OpCode_PushTransform:
				case OpCode_PopTransform:
				case OpCode_SetCameraTransform:
				default:
					break;
			}
		}
	}
}

vsDisplayList *
vsDisplayList::Load_Obj( const vsString &filename )
{
	int vertCount = 0;
	int faceIndexCount = 0;
	vsDisplayList *loader = new vsDisplayList(1024 * 200);	// 50k should be enough to load in just about anything.  (famous  last words)

	vsFile *file = new vsFile(filename + vsString(".obj"));
	vsRecord r;

	vsString faceStr("f");
	vsString vertStr("v");

	while( file->Record(&r) )
	{
		vsString label = r.GetLabel().AsString();

		if ( label == vertStr )
			vertCount++;
		if ( label == faceStr )
			faceIndexCount+=3;
	}

	file->Rewind();

	vsVector3D *vertexPos = new vsVector3D[vertCount];
	int *triangleIndex = new int[faceIndexCount];

	int v = 0;
	int f = 0;

	while( file->Record(&r) )
	{
		vsString label = r.GetLabel().AsString();

		if ( label == vertStr )
			vertexPos[v++].Set(r.GetToken(0).AsFloat(), -r.GetToken(1).AsFloat(), 0.f);
		if ( label == faceStr )
		{
			triangleIndex[f++] = r.GetToken(0).AsInteger()-1;
			triangleIndex[f++] = r.GetToken(1).AsInteger()-1;
			triangleIndex[f++] = r.GetToken(2).AsInteger()-1;
		}
	}

	loader->VertexArray( vertexPos, vertCount );
	loader->TriangleListArray( triangleIndex, faceIndexCount );

	delete file;

	vsAssert(loader->GetSize() > 0, "Didn't get any operations in a loaded display list!" )
	vsLog("Display list is %d bytes", loader->GetSize());
	vsDisplayList *result = new vsDisplayList( loader->GetSize() );
	result->Append(*loader);
	delete loader;

	vsDeleteArray(vertexPos);
	vsDeleteArray(triangleIndex);

	return result;
}
/*
vsDisplayList *
vsDisplayList::Load_CVec( const vsString &filename )
{
	vsFile *file = new vsFile(filename + vsString(".cvec"));

	vsDisplayList *result  = new vsDisplayList( file->GetLength() );
	file->Store(result->m_fifo);

	vsDelete( file );

	return result;
}*/

void
vsDisplayList::Write_CVec( const vsString &filename )
{
	vsFile *file = new vsFile(filename + vsString(".cvec"), vsFile::MODE_Write);

	file->Store(m_fifo);

	vsDelete( file );
}

vsDisplayList *
vsDisplayList::Load( vsRecord *record )
{
	vsDisplayList *result = Load_Vec(record);

	return result;
}

vsDisplayList *
vsDisplayList::Load( const vsString &filename )
{
//	int vec = filename.find(".vec");
//	int obj = filename.find(".obj");

	vsDisplayList *result = NULL;
	// check for a precompiled version of this file.

	{
		if ( vsFile::Exists(filename + vsString(".vec")) )
		{
			result = Load_Vec(filename);
			//result->Write_CVec(filename);
		}
		else if ( vsFile::Exists(filename + vsString(".obj")) )
		{
			result = Load_Obj(filename);
			//result->Write_CVec(filename);
		}
	}

	vsAssert(result, vsFormatString("Unable to load %s", filename.c_str()) );

	return result;
}

vsDisplayList::vsDisplayList():
	m_instanceParent(NULL),
	m_instanceCount(0),
	m_materialCount(0),
	m_colorSet(false)
{
	Clear();
}

vsDisplayList::vsDisplayList( size_t memSize ):
	m_instanceParent(NULL),
	m_instanceCount(0),
	m_materialCount(0),
	m_colorSet(false)
{
	if ( memSize )
	{
		m_fifo = new vsStore(memSize);
	}
	else
	{
		m_fifo = NULL;
	}

	Clear();
}

vsDisplayList::~vsDisplayList()
{
	vsAssert( m_instanceCount == 0, "Deleted a display list while something was still referencing it!" );

	for ( int i = 0; i < m_materialCount; i++ )
	{
		vsDelete( m_material[i] );
	}

	if ( m_fifo )
	{
		delete m_fifo;
		m_fifo = NULL;
	}
	else if ( m_instanceParent )
	{
		m_instanceParent->m_instanceCount--;
	}
}

vsDisplayList *
vsDisplayList::CreateInstance()
{
	vsDisplayList *result = new vsDisplayList(0);

	result->m_instanceParent = this;
	m_instanceCount++;

	return result;
}

size_t
vsDisplayList::GetSize() const
{
	return m_fifo->Length();
}

size_t
vsDisplayList::GetMaxSize() const
{
	return m_fifo->BufferLength();
}

void
vsDisplayList::Clear()
{
	if ( m_fifo )
	{
		m_fifo->Clear();
	}
	m_colorSet = false;
}

void
vsDisplayList::Rewind()
{
	m_fifo->Rewind();
}

void
vsDisplayList::SetColor( const vsColor &color )
{
	m_fifo->WriteUint8( OpCode_SetColor );
	m_fifo->WriteColor( color );
	m_nextLineColor = color;
	m_colorSet = true;
}

void
vsDisplayList::MoveTo( const vsVector3D &pos )
{
	m_cursorPos = pos;
	m_cursorColor = m_nextLineColor;
}

void
vsDisplayList::LineTo( const vsVector3D &pos )
{
	vsVector3D v[2];
	vsColor c[2];
	int i[2];

	v[0] = m_cursorPos;
	v[1] = pos;

	if ( m_colorSet )
	{
		m_fifo->WriteUint8( OpCode_SetColor );
		m_fifo->WriteColor( c_white );

		c[0] = m_cursorColor;
		c[1] = m_nextLineColor;
	}

	i[0] = 0;
	i[1] = 1;

	VertexArray(v,2);
	if ( m_colorSet )
	{
		ColorArray(c,2);
	}
	LineListArray(i,2);
	if ( m_colorSet )
	{
		ClearColorArray();
	}
	ClearVertexArray();

	m_cursorPos = pos;
	m_cursorColor = m_nextLineColor;
}

void
vsDisplayList::DrawPoint( const vsVector3D &pos )
{
	int index = 0;
	VertexArray(&pos, 1);
	PointsArray( &index, 1 );
	ClearVertexArray();
}

void
vsDisplayList::Append( const vsDisplayList &list )
{
	if ( list.m_instanceParent )
	{
		Append( *list.m_instanceParent );
	}
	else
	{
		m_fifo->Append( list.m_fifo );
	}
}

void
vsDisplayList::DrawLine( const vsVector3D &from, const vsVector3D &to )
{
	MoveTo(from);
	LineTo(to);
}

void
vsDisplayList::PushTransform( const vsTransform2D &t )
{
	m_fifo->WriteUint8( OpCode_PushTransform );
	m_fifo->WriteTransform2D( t );
//    PushMatrix4x4( t.GetMatrix() );
}

void
vsDisplayList::PushTransform( const vsTransform3D &t )
{
	if ( t.GetRotation() == vsQuaternion::Identity && t.GetScale() == vsVector3D::One )
	{
		PushTranslation( t.GetTranslation() );
	}
	else
	{
		PushMatrix4x4( t.GetMatrix() );
	}
}

void
vsDisplayList::PushMatrix4x4( const vsMatrix4x4 &m )
{
	m_fifo->WriteUint8( OpCode_PushMatrix4x4 );
	m_fifo->WriteMatrix4x4( m );
}

void
vsDisplayList::SetMatrix4x4( const vsMatrix4x4 &m )
{
	m_fifo->WriteUint8( OpCode_SetMatrix4x4 );
	m_fifo->WriteMatrix4x4( m );
}

void
vsDisplayList::SetMatrices4x4( const vsMatrix4x4 *m, int count )
{
	m_fifo->WriteUint8( OpCode_SetMatrices4x4 );
	m_fifo->WriteUint32( count );
	m_fifo->WriteVoidStar( (char*)m );
}

void
vsDisplayList::SetMatrices4x4Buffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_SetMatrices4x4Buffer );
	m_fifo->WriteVoidStar( (char*)buffer );
}

void
vsDisplayList::SetColors( const vsColor *c, int count )
{
	m_fifo->WriteUint8( OpCode_SetColors );
	m_fifo->WriteUint32( count );
	m_fifo->WriteVoidStar( (char*)c );
}

void
vsDisplayList::SnapMatrix()
{
	m_fifo->WriteUint8( OpCode_SnapMatrix );
}

void
vsDisplayList::SetWorldToViewMatrix4x4( const vsMatrix4x4 &m )
{
	m_fifo->WriteUint8( OpCode_SetWorldToViewMatrix4x4 );
	m_fifo->WriteMatrix4x4( m );
}

void
vsDisplayList::PushTranslation( const vsVector3D &offset )
{
	m_fifo->WriteUint8( OpCode_PushTranslation );
	m_fifo->WriteVector3D( offset );
}

void
vsDisplayList::SetCameraTransform( const vsTransform2D &t )
{
	m_fifo->WriteUint8( OpCode_SetCameraTransform );
	m_fifo->WriteTransform2D( t );
}

void
vsDisplayList::Set3DProjection( float fov, float nearPlane, float farPlane )
{
	m_fifo->WriteUint8( OpCode_Set3DProjection );
	m_fifo->WriteFloat(fov);
	m_fifo->WriteFloat(nearPlane);
	m_fifo->WriteFloat(farPlane);
}

void
vsDisplayList::SetProjectionMatrix4x4( const vsMatrix4x4 &m )
{
	m_fifo->WriteUint8( OpCode_SetProjectionMatrix4x4 );
	m_fifo->WriteMatrix4x4( m );
}

void
vsDisplayList::PopTransform()
{
	m_fifo->WriteUint8( OpCode_PopTransform );
}

void
vsDisplayList::VertexArray( const vsVector2D *array, int arrayCount )
{
	m_fifo->WriteUint8( OpCode_VertexArray );
	m_fifo->WriteUint32( arrayCount );
	for ( int i = 0; i < arrayCount; i++ )
	{
		m_fifo->WriteVector3D( array[i] );
	}
}

void
vsDisplayList::VertexArray( const vsVector3D *array, int arrayCount )
{
	m_fifo->WriteUint8( OpCode_VertexArray );
	m_fifo->WriteUint32( arrayCount );
	for ( int i = 0; i < arrayCount; i++ )
	{
		m_fifo->WriteVector3D( array[i] );
	}
}

void
vsDisplayList::VertexBuffer( vsRenderBuffer *buffer )
{
	vsAssert(buffer->GetContentType() == vsRenderBuffer::ContentType_Custom ||
			buffer->GetContentType() == vsRenderBuffer::ContentType_P,
			"Known render buffer types should use ::BindBuffer");
	m_fifo->WriteUint8( OpCode_VertexBuffer );
	m_fifo->WriteVoidStar( buffer );
}

void
vsDisplayList::NormalArray( const vsVector3D *array, int arrayCount )
{
	m_fifo->WriteUint8( OpCode_NormalArray );
	m_fifo->WriteUint32( arrayCount );
	for ( int i = 0; i < arrayCount; i++ )
	{
		m_fifo->WriteVector3D( array[i] );
	}
}

void
vsDisplayList::NormalBuffer( vsRenderBuffer *buffer )
{
	vsAssert(buffer->GetContentType() == vsRenderBuffer::ContentType_Custom,
			"Non-custom render buffer types should use ::BindBuffer");
	m_fifo->WriteUint8( OpCode_NormalBuffer );
	m_fifo->WriteVoidStar( buffer );
}

void
vsDisplayList::TexelBuffer( vsRenderBuffer *buffer )
{
	vsAssert(buffer->GetContentType() == vsRenderBuffer::ContentType_Custom,
			"Non-custom render buffer types should use ::BindBuffer");
	m_fifo->WriteUint8( OpCode_TexelBuffer );
	m_fifo->WriteVoidStar( buffer );
}

void
vsDisplayList::ColorBuffer( vsRenderBuffer *buffer )
{
	vsAssert(buffer->GetContentType() == vsRenderBuffer::ContentType_Custom,
			"Non-custom render buffer types should use ::BindBuffer");
	m_fifo->WriteUint8( OpCode_ColorBuffer );
	m_fifo->WriteVoidStar( buffer );
}

void
vsDisplayList::BindBuffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_BindBuffer );
	m_fifo->WriteVoidStar( buffer );
}

void
vsDisplayList::UnbindBuffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_UnbindBuffer );
	m_fifo->WriteVoidStar( buffer );
}

void
vsDisplayList::ClearVertexArray(  )
{
	m_fifo->WriteUint8( OpCode_ClearVertexArray );
}

void
vsDisplayList::ClearNormalArray(  )
{
	m_fifo->WriteUint8( OpCode_ClearNormalArray );
}

void
vsDisplayList::ClearTexelArray(  )
{
	m_fifo->WriteUint8( OpCode_ClearTexelArray );
}

void
vsDisplayList::ClearColorArray(  )
{
	m_fifo->WriteUint8( OpCode_ClearColorArray );
}

void
vsDisplayList::ClearArrays()
{
	m_fifo->WriteUint8( OpCode_ClearArrays );
}

void
vsDisplayList::TexelArray( const vsVector2D *array, int arrayCount )
{
	m_fifo->WriteUint8( OpCode_TexelArray );
	m_fifo->WriteUint32( arrayCount );
	for ( int i = 0; i < arrayCount; i++ )
	{
		m_fifo->WriteVector2D( array[i] );
	}
}

void
vsDisplayList::ColorArray( const vsColor *array, int arrayCount )
{
	m_fifo->WriteUint8( OpCode_ColorArray );
	m_fifo->WriteUint32( arrayCount );
	m_colorSet = true;
	for ( int i = 0; i < arrayCount; i++ )
	{
		m_fifo->WriteColor( array[i] );
	}
}

void
vsDisplayList::LineListArray( int *idArray, int vertexCount )
{
	m_fifo->WriteUint8( OpCode_LineListArray );
	m_fifo->WriteUint32( vertexCount );
	for ( int i = 0; i < vertexCount; i++ )
	{
		m_fifo->WriteUint16Native( idArray[i] );
	}
}

void
vsDisplayList::LineStripArray( uint16_t *idArray, int vertexCount )
{
	m_fifo->WriteUint8( OpCode_LineStripArray );
	m_fifo->WriteUint32( vertexCount );
	for ( int i = 0; i < vertexCount; i++ )
	{
		m_fifo->WriteUint16Native( idArray[i] );
	}
}

void
vsDisplayList::LineStripArray( int *idArray, int vertexCount )
{
	m_fifo->WriteUint8( OpCode_LineStripArray );
	m_fifo->WriteUint32( vertexCount );
	for ( int i = 0; i < vertexCount; i++ )
	{
		m_fifo->WriteUint16Native( idArray[i] );
	}
}

void
vsDisplayList::TriangleListArray( int *idArray, int vertexCount )
{
	m_fifo->WriteUint8( OpCode_TriangleListArray );
	m_fifo->WriteUint32( vertexCount );
	for ( int i = 0; i < vertexCount; i++ )
	{
		m_fifo->WriteUint16Native( idArray[i] );
	}
}

void
vsDisplayList::TriangleStripArray( int *idArray, int vertexCount )
{
	m_fifo->WriteUint8( OpCode_TriangleStripArray );
	m_fifo->WriteUint32( vertexCount );
	for ( int i = 0; i < vertexCount; i++ )
	{
		m_fifo->WriteUint16Native( idArray[i] );
	}
}

void
vsDisplayList::TriangleStripBuffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_TriangleStripBuffer );
	m_fifo->WriteVoidStar(buffer);
}

void
vsDisplayList::TriangleListBuffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_TriangleListBuffer );
	m_fifo->WriteVoidStar(buffer);
}

void
vsDisplayList::TriangleFanBuffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_TriangleFanBuffer );
	m_fifo->WriteVoidStar(buffer);
}

void
vsDisplayList::LineListBuffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_LineListBuffer );
	m_fifo->WriteVoidStar(buffer);
}

void
vsDisplayList::LineStripBuffer( vsRenderBuffer *buffer )
{
	m_fifo->WriteUint8( OpCode_LineStripBuffer );
	m_fifo->WriteVoidStar(buffer);
}

void
vsDisplayList::PointsArray( int *idArray, int vertexCount )
{
	m_fifo->WriteUint8( OpCode_PointsArray );
	m_fifo->WriteUint32( vertexCount );
	for ( int i = 0; i < vertexCount; i++ )
	{
		m_fifo->WriteUint16Native( idArray[i] );
	}
}

void
vsDisplayList::TriangleFanArray( int *idArray, int vertexCount )
{
	m_fifo->WriteUint8( OpCode_TriangleFanArray );
	m_fifo->WriteUint32( vertexCount );
	for ( int i = 0; i < vertexCount; i++ )
	{
		m_fifo->WriteUint16Native( idArray[i] );
	}
}

void
vsDisplayList::SetMaterial( vsMaterial *material )
{
	m_fifo->WriteUint8( OpCode_SetMaterial );
	m_fifo->WriteVoidStar( material );
}


void
vsDisplayList::SetRenderTarget( vsRenderTarget *target )
{
	m_fifo->WriteUint8( OpCode_SetRenderTarget );
	m_fifo->WriteVoidStar( target );
}

void
vsDisplayList::ClearRenderTarget()
{
	m_fifo->WriteUint8( OpCode_ClearRenderTarget );
}

void
vsDisplayList::ResolveRenderTarget( vsRenderTarget *target )
{
	m_fifo->WriteUint8( OpCode_ResolveRenderTarget );
	m_fifo->WriteVoidStar( target );
}

void
vsDisplayList::BlitRenderTarget( vsRenderTarget *from, vsRenderTarget *to )
{
	m_fifo->WriteUint8( OpCode_BlitRenderTarget );
	m_fifo->WriteVoidStar( from );
	m_fifo->WriteVoidStar( to );
}

void
vsDisplayList::Light( const vsLight &light )
{
	m_fifo->WriteUint8( OpCode_Light );
	m_fifo->WriteLight( light );
}

void
vsDisplayList::ClearLights()
{
	m_fifo->WriteUint8( OpCode_ClearLights );
}

void
vsDisplayList::Fog( const vsFog &fog )
{
	m_fifo->WriteUint8( OpCode_Fog );
	m_fifo->WriteFog( fog );
}

void
vsDisplayList::ClearFog()
{
	m_fifo->WriteUint8( OpCode_ClearFog );
}

void
vsDisplayList::FlatShading()
{
	m_fifo->WriteUint8( OpCode_FlatShading );
}

void
vsDisplayList::SmoothShading()
{
	m_fifo->WriteUint8( OpCode_SmoothShading );
}

void
vsDisplayList::EnableStencil()
{
	m_fifo->WriteUint8( OpCode_EnableStencil );
}

void
vsDisplayList::DisableStencil()
{
	m_fifo->WriteUint8( OpCode_DisableStencil );
}

void
vsDisplayList::EnableScissor( const vsBox2D& box )
{
	m_fifo->WriteUint8( OpCode_EnableScissor );
	m_fifo->WriteBox2D( box );
}

void
vsDisplayList::DisableScissor()
{
	m_fifo->WriteUint8( OpCode_DisableScissor );
}

void
vsDisplayList::ClearStencil()
{
	m_fifo->WriteUint8( OpCode_ClearStencil );
}

void
vsDisplayList::SetViewport( const vsBox2D &box )
{
	m_fifo->WriteUint8( OpCode_SetViewport );
	m_fifo->WriteBox2D( box );
}

void
vsDisplayList::ClearViewport()
{
	m_fifo->WriteUint8( OpCode_ClearViewport );
}

void
vsDisplayList::Debug(const vsString &string )
{
	m_fifo->WriteUint8( OpCode_Debug );
	m_fifo->WriteString(string);
}

vsDisplayList::OpCode
vsDisplayList::PeekOpType()
{
	OpCode result = OpCode_MAX;
	if ( !m_fifo->AtEnd() )
	{
		result = (OpCode)m_fifo->PeekUint8();
	}

	return result;
}

static vsColor		s_color;
static vsVector2D	s_vector;
static vsVector3D	s_vector3d;
static vsTransform2D	s_transform;

vsDisplayList::op *
vsDisplayList::PopOp()
{
	if ( !m_fifo->AtEnd() )
	{
		m_currentOp.type = (OpCode)m_fifo->ReadUint8();

		switch( m_currentOp.type )
		{
			case OpCode_SetColor:
				m_fifo->ReadColor(&m_currentOp.data.color);
				break;
			case OpCode_SetColors:
				{
					int count = m_fifo->ReadUint32();
					m_currentOp.data.Set( count );
					m_currentOp.data.SetPointer( (char*)m_fifo->ReadVoidStar() );
					break;
				}
			case OpCode_PushTranslation:
				m_fifo->ReadVector3D(&m_currentOp.data.vector);
				break;
			case OpCode_VertexArray:
			{
				int count = m_fifo->ReadUint32();
				m_currentOp.data.Set( count );
				m_currentOp.data.SetPointer( m_fifo->GetReadHead() );

				m_fifo->AdvanceReadHead( sizeof(s_vector3d) * count );
				//for ( int i = 0; i < count; i++ )
				//	m_fifo->ReadVector3D(&s_vector3d);

				break;
			}
			case OpCode_NormalArray:
			{
				int count = m_fifo->ReadUint32();
				m_currentOp.data.Set( count );
				m_currentOp.data.SetPointer( m_fifo->GetReadHead() );

				m_fifo->AdvanceReadHead( sizeof(s_vector3d) * count );
//				for ( int i = 0; i < count; i++ )
//					m_fifo->ReadVector3D(&s_vector3d);

				break;
			}
			case OpCode_VertexBuffer:
			case OpCode_NormalBuffer:
			case OpCode_TexelBuffer:
			case OpCode_ColorBuffer:
			case OpCode_BindBuffer:
			case OpCode_UnbindBuffer:
			{
				m_currentOp.data.SetPointer( (char *)m_fifo->ReadVoidStar() );
				break;
			}
			case OpCode_TexelArray:
			{
				int count = m_fifo->ReadUint32();
				m_currentOp.data.Set( count );
				m_currentOp.data.SetPointer( m_fifo->GetReadHead() );

				m_fifo->AdvanceReadHead( sizeof(s_vector) * count );
//				for ( int i = 0; i < count; i++ )
//					m_fifo->ReadVector2D(&s_vector);

				break;
			}
			case OpCode_ColorArray:
			{
				int count = m_fifo->ReadUint32();
				m_currentOp.data.Set( count );
				m_currentOp.data.SetPointer( m_fifo->GetReadHead() );

				m_fifo->AdvanceReadHead( sizeof(s_color) * count );
//				for ( int i = 0; i < count; i++ )
//				{
//					m_fifo->ReadColor(&s_color);
//				}
				break;
			}
			case OpCode_LineListArray:
			case OpCode_LineStripArray:
			case OpCode_TriangleListArray:
			case OpCode_TriangleStripArray:
			case OpCode_TriangleFanArray:
			case OpCode_PointsArray:
			{
				int count = m_fifo->ReadUint32();
				m_currentOp.data.Set( count );
				m_currentOp.data.SetPointer( m_fifo->GetReadHead() );

				m_fifo->AdvanceReadHead( sizeof(uint16_t) * count );
//				for ( int i = 0; i < count; i++ )
//					m_fifo->Readuint16_t();

				break;
			}
			case OpCode_LineListBuffer:
			case OpCode_LineStripBuffer:
			case OpCode_TriangleStripBuffer:
			case OpCode_TriangleListBuffer:
			case OpCode_TriangleFanBuffer:
			{
				m_currentOp.data.SetPointer( (char *)m_fifo->ReadVoidStar() );
				break;
			}
			case OpCode_PushTransform:
			case OpCode_SetCameraTransform:
				m_fifo->ReadTransform2D( &m_currentOp.data.transform );
				break;
			case OpCode_PushMatrix4x4:
			case OpCode_SetMatrix4x4:
			case OpCode_SetWorldToViewMatrix4x4:
				m_fifo->ReadMatrix4x4( &m_currentOp.data.matrix4x4 );
				break;
			case OpCode_SetMatrices4x4:
				{
					int count = m_fifo->ReadUint32();
					m_currentOp.data.SetPointer( (char*)m_fifo->ReadVoidStar() );
					m_currentOp.data.i = count;
					break;
				}
			case OpCode_SetMatrices4x4Buffer:
				{
					m_currentOp.data.SetPointer( (char*)m_fifo->ReadVoidStar() );
					break;
				}
			case OpCode_Set3DProjection:
				m_currentOp.data.fov = m_fifo->ReadFloat();
				m_currentOp.data.nearPlane = m_fifo->ReadFloat();
				m_currentOp.data.farPlane = m_fifo->ReadFloat();
				break;
			case OpCode_SetProjectionMatrix4x4:
				m_fifo->ReadMatrix4x4( &m_currentOp.data.matrix4x4 );
				break;
			case OpCode_SetMaterial:
				m_currentOp.data.SetPointer( (char *)m_fifo->ReadVoidStar() );
				break;
			case OpCode_SetRenderTarget:
				m_currentOp.data.SetPointer( (char *)m_fifo->ReadVoidStar() );
				break;
			case OpCode_ResolveRenderTarget:
				m_currentOp.data.SetPointer( (char *)m_fifo->ReadVoidStar() );
				break;
			case OpCode_BlitRenderTarget:
				m_currentOp.data.SetPointer( (char *)m_fifo->ReadVoidStar() );
				m_currentOp.data.SetPointer2( (char *)m_fifo->ReadVoidStar() );
				break;
			case OpCode_Light:
				m_fifo->ReadLight( &m_currentOp.data.light );
				break;
			case OpCode_Fog:
				m_fifo->ReadFog( &m_currentOp.data.fog );
				break;
			case OpCode_SetViewport:
				m_fifo->ReadBox2D( &m_currentOp.data.box2D );
				break;
			case OpCode_Debug:
				 m_currentOp.data.string = m_fifo->ReadString();
				break;
			case OpCode_EnableScissor:
				m_fifo->ReadBox2D( &m_currentOp.data.box2D );
				break;
			case OpCode_ClearRenderTarget:
			case OpCode_ClearLights:
			case OpCode_PopTransform:
			case OpCode_ClearVertexArray:
			case OpCode_ClearNormalArray:
			case OpCode_ClearTexelArray:
			case OpCode_ClearColorArray:
			case OpCode_ClearArrays:
			case OpCode_FlatShading:
			case OpCode_SmoothShading:
			case OpCode_EnableStencil:
			case OpCode_DisableStencil:
			case OpCode_DisableScissor:
			case OpCode_ClearStencil:
			case OpCode_ClearViewport:
			default:
				break;
		}

		return &m_currentOp;
	}

	return NULL;
}

void
vsDisplayList::AppendOp(vsDisplayList::op * o)
{
	OpCode type = o->type;

	switch( type )
	{
		case OpCode_SetColor:
			SetColor( o->data.GetColor() );
			break;
		case OpCode_SetColors:
			SetColors( (vsColor*)o->data.p, o->data.GetUInt() );
		case OpCode_VertexArray:
			VertexArray( (vsVector3D *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_NormalArray:
			NormalArray( (vsVector3D *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_TexelArray:
			TexelArray( (vsVector2D *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_ColorArray:
			ColorArray( (vsColor *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_VertexBuffer:
			VertexBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_TexelBuffer:
			TexelBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_ColorBuffer:
			ColorBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_BindBuffer:
			BindBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_UnbindBuffer:
			UnbindBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_ClearVertexArray:
			ClearVertexArray();
			break;
		case OpCode_ClearNormalArray:
			ClearNormalArray();
			break;
		case OpCode_ClearTexelArray:
			ClearTexelArray();
			break;
		case OpCode_ClearColorArray:
			ClearColorArray();
			break;
		case OpCode_ClearArrays:
			ClearArrays();
			break;
		case OpCode_LineListArray:
			LineListArray( (int *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_LineStripArray:
			LineStripArray( (int *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_TriangleListArray:
			TriangleListArray( (int *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_TriangleStripArray:
			TriangleStripArray( (int *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_PointsArray:
			PointsArray( (int *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_LineListBuffer:
			LineListBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_LineStripBuffer:
			LineStripBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_TriangleStripBuffer:
			TriangleStripBuffer( (vsRenderBuffer *)o->data.p );
			break;
		case OpCode_TriangleFanArray:
			TriangleFanArray( (int *)o->data.p, o->data.GetUInt() );
			break;
		case OpCode_PushTransform:
			PushTransform( o->data.GetTransform() );
			break;
		case OpCode_SetCameraTransform:
			SetCameraTransform( o->data.GetTransform() );
			break;
		case OpCode_SetMatrix4x4:
			SetMatrix4x4( o->data.GetMatrix4x4() );
			break;
		case OpCode_PushMatrix4x4:
			PushMatrix4x4( o->data.GetMatrix4x4() );
			break;
		case OpCode_SetMatrices4x4:
			SetMatrices4x4( (vsMatrix4x4*)o->data.p, o->data.i );
			break;
		case OpCode_SetMatrices4x4Buffer:
			SetMatrices4x4Buffer( (vsRenderBuffer*)o->data.p );
			break;
		case OpCode_SetWorldToViewMatrix4x4:
			SetWorldToViewMatrix4x4( o->data.GetMatrix4x4() );
			break;
		case OpCode_Set3DProjection:
			Set3DProjection( o->data.fov, o->data.nearPlane, o->data.farPlane );
			break;
		case OpCode_SetProjectionMatrix4x4:
			SetProjectionMatrix4x4( o->data.GetMatrix4x4() );
			break;
		case OpCode_PopTransform:
			PopTransform();
			break;
		case OpCode_EnableStencil:
			EnableStencil();
			break;
		case OpCode_DisableStencil:
			DisableStencil();
			break;
		case OpCode_EnableScissor:
			EnableScissor( o->data.GetBox2D() );
			break;
		case OpCode_DisableScissor:
			DisableScissor();
			break;
		case OpCode_ClearStencil:
			ClearStencil();
			break;
		case OpCode_SetViewport:
			SetViewport( o->data.GetBox2D() );
			break;
		case OpCode_ClearViewport:
			ClearViewport();
			break;
		case OpCode_Debug:
			Debug( o->data.GetString() );
			break;
		default:
			break;
	}
}


void
vsDisplayList::GetBoundingCircle(vsVector2D &center, float &radius)
{
	if ( m_instanceParent )
	{
		m_instanceParent->GetBoundingCircle(center, radius);
	}
	else
	{
		vsVector3D min(1000000.0f,1000000.0f,1000000.f);
		vsVector3D max(-1000000.0f, -1000000.0f,-1000000.f);

		vsTransform2D currentTransform;
		Rewind();
		op *o = PopOp();

		while(o)
		{
			if ( o->type == OpCode_VertexArray )
			{
				vsVector3D pos;
				int count = o->data.GetUInt();
				float *shuttle = (float *) o->data.p;

				for ( int i = 0; i < count; i++ )
				{
					pos.Set(shuttle[0],shuttle[1],shuttle[2]);

					max.x = vsMax( max.x, pos.x );
					max.y = vsMax( max.y, pos.y );
					max.z = vsMax( max.z, pos.z );
					min.x = vsMin( min.x, pos.x );
					min.y = vsMin( min.y, pos.y );
					min.z = vsMin( min.z, pos.z );

					shuttle += 3;
				}
			}
			o = PopOp();
		}

		center = 0.5f * (max + min);
		radius = (max-min).Length() * 0.5f;
	}
}

float
vsDisplayList::GetBoundingRadius()
{
	vsVector2D tl, br;
	GetBoundingBox(tl, br);

	float maxDistance = vsMax( tl.Length(), br.Length() );

	return maxDistance;
}

void
vsDisplayList::GetBoundingBox( vsVector2D &topLeft, vsVector2D &bottomRight )
{
	vsBox2D box;
	topLeft.Set(500000.f,500000.f);
	bottomRight.Set(-50000.f,-50000.f);

	if ( m_instanceParent )
	{
		return m_instanceParent->GetBoundingBox(topLeft, bottomRight);
	}
	else
	{
		vsTransform2D	transformStack[20];
		transformStack[0] = vsTransform2D::Zero;
		int				transformStackLevel = 0;
		vsVector3D		*currentVertexArray = NULL;
		vsRenderBuffer *currentVertexBuffer = NULL;

		Rewind();
		op *o = PopOp();

		while(o)
		{
			switch( o->type )
			{
				case OpCode_PushTransform:
					transformStack[transformStackLevel+1] = transformStack[transformStackLevel] * o->data.transform;
					transformStackLevel++;
					break;
				case OpCode_PushTranslation:
					{
						vsTransform2D transform;
						transform.SetTranslation( o->data.GetVector3D() );
						transformStack[transformStackLevel+1] = transformStack[transformStackLevel] * transform;
						transformStackLevel++;
						break;
					}
				case OpCode_PopTransform:
					transformStackLevel--;
					break;
				case OpCode_VertexArray:
					currentVertexBuffer = NULL;
					currentVertexArray = (vsVector3D *)o->data.p;
					break;
				case OpCode_VertexBuffer:
				case OpCode_BindBuffer:
					currentVertexArray = NULL;
					currentVertexBuffer = (vsRenderBuffer *)o->data.p;
					break;
				case OpCode_LineListBuffer:
				case OpCode_LineStripBuffer:
				case OpCode_TriangleListBuffer:
				case OpCode_TriangleStripBuffer:
				case OpCode_TriangleFanBuffer:
					{
						vsRenderBuffer *buffer = (vsRenderBuffer *)o->data.p;
						uint16_t *shuttle = buffer->GetIntArray();

						for ( int i = 0; i < buffer->GetIntArraySize(); i++ )
						{
							uint16_t index = shuttle[i];
							vsVector3D pos;
							if ( currentVertexArray )
								pos = currentVertexArray[index];
							else
								pos = currentVertexBuffer->GetPosition(index);
							box.ExpandToInclude( transformStack[transformStackLevel].ApplyTo(pos) );
						}
						break;
					}
				case OpCode_LineListArray:
				case OpCode_LineStripArray:
				case OpCode_TriangleListArray:
				case OpCode_TriangleStripArray:
				case OpCode_TriangleFanArray:
				case OpCode_PointsArray:
					{
						uint16_t *shuttle = (uint16_t *)o->data.p;
						int count = o->data.GetUInt();

						for ( int i = 0; i < count; i++ )
						{
							uint16_t index = shuttle[i];
							vsVector3D pos;
							if ( currentVertexArray )
								pos = currentVertexArray[index];
							else
								pos = currentVertexBuffer->GetPosition(index);
							box.ExpandToInclude( transformStack[transformStackLevel].ApplyTo(pos) );
						}
						break;
					}
				default:
					break;
			}

			o = PopOp();
		}
	}

	topLeft = box.GetMin();
	bottomRight = box.GetMax();
}


void
vsDisplayList::GetBoundingBox( vsBox3D &box )
{
	box = vsBox3D();
	if ( m_instanceParent )
	{
		return m_instanceParent->GetBoundingBox(box);
	}
	else
	{
		vsMatrix4x4			transformStack[20];
		transformStack[0] = vsMatrix4x4::Identity;
		int				transformStackLevel = 0;
		vsVector3D		*currentVertexArray = NULL;
		//int			currentVertexArraySize = 0;

		Rewind();
		op *o = PopOp();

		while(o)
		{
			if ( o->type == OpCode_PushMatrix4x4 )
			{
				transformStack[transformStackLevel+1] = transformStack[transformStackLevel] * o->data.matrix4x4;
				transformStackLevel++;
			}
			if ( o->type == OpCode_SetMatrix4x4 )
			{
				transformStack[++transformStackLevel] = o->data.matrix4x4;
			}
			else if ( o->type == OpCode_SetMatrices4x4 )
			{
				vsMatrix4x4 *mat = (vsMatrix4x4*)o->data.p;
				transformStack[++transformStackLevel] = *mat;
			}
			else if ( o->type == OpCode_PopTransform )
			{
				transformStackLevel--;
			}
			else if ( o->type == OpCode_VertexArray )
			{
				vsVector3D pos;
				int count = o->data.GetUInt();
				float *shuttle = (float *) o->data.p;
				currentVertexArray = (vsVector3D *)shuttle;
				//currentVertexArraySize = count*3;

				for ( int i = 0; i < count; i++ )
				{
					pos.Set(shuttle[0],shuttle[1],shuttle[2]);
					pos = transformStack[transformStackLevel].ApplyTo( pos );

					box.ExpandToInclude( pos );

					shuttle += 3;
				}
			}
			else if ( o->type == OpCode_VertexBuffer )
			{
				vsVector3D pos;
				vsRenderBuffer *buffer = (vsRenderBuffer *)o->data.p;
				currentVertexArray = buffer->GetVector3DArray();
				//currentVertexArraySize = buffer->GetVector3DArraySize();

				for ( int i = 0; i < buffer->GetVector3DArraySize(); i++ )
				{
					pos = buffer->GetVector3DArray()[i];
					pos = transformStack[transformStackLevel].ApplyTo( pos );

					box.ExpandToInclude( pos );
				}
			}
			else if ( o->type == OpCode_BindBuffer )
			{
				vsVector3D pos;
				vsRenderBuffer *buffer = (vsRenderBuffer *)o->data.p;
				int positionCount = buffer->GetPositionCount();

				for ( int i = 0; i < positionCount; i++ )
				{
					pos = buffer->GetPosition(i);
					pos = transformStack[transformStackLevel].ApplyTo( pos );

					box.ExpandToInclude( pos );
				}
			}
			else if ( o->type == OpCode_LineListBuffer || o->type == OpCode_LineStripBuffer )
			{
				vsRenderBuffer *buffer = (vsRenderBuffer *)o->data.p;
				uint16_t *shuttle = buffer->GetIntArray();

				for ( int i = 0; i < buffer->GetIntArraySize(); i++ )
				{
					uint16_t index = shuttle[i];
					box.ExpandToInclude( transformStack[transformStackLevel].ApplyTo( currentVertexArray[index] ) );
				}
			}
			else if ( o->type == OpCode_LineStripArray )
			{
				uint16_t *shuttle = (uint16_t *)o->data.p;
				int count = o->data.GetUInt();

				for ( int i = 0; i < count; i++ )
				{
					uint16_t index = shuttle[i];
					box.ExpandToInclude( transformStack[transformStackLevel].ApplyTo( currentVertexArray[index] ) );
				}
			}

			o = PopOp();
		}
	}
}


void
vsDisplayList::ApplyOffset(const vsVector2D &offset)
{
	vsAssert( !m_instanceParent, "Tried to apply an offset to an instanced display list!" );

	vsTransform2D currentTransform;

	Rewind();
	op *o = PopOp();

	while(o)
	{
		if ( o->type == OpCode_VertexArray )
		{
			vsVector3D pos;
			int count = o->data.GetUInt();
			float *shuttle = (float *) o->data.p;

			for ( int i = 0; i < count; i++ )
			{
				shuttle[0] += offset.x;
				shuttle[1] += offset.y;

				shuttle += 3;
			}
		}

		o = PopOp();
	}
}

