/*
 *  MEM_Store.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *q
 */

#include "VS_Store.h"

#include "VS_Angle.h"
#include "VS_Box.h"
#include "VS_Color.h"
#include "VS_Fog.h"
#include "VS_Light.h"
#include "VS_Material.h"
#include "VS_Matrix.h"
#include "VS_Transform.h"
#include "VS_Vector.h"

#if defined(_WIN32)
#include <winsock2.h>
#elif defined(__GNUC__)
#include <netinet/in.h> // for access to ntohl, et al
#endif

#include <zlib.h>

vsStore::vsStore():
	m_buffer( nullptr ),
	m_bufferLength( 0 ),
	m_bufferEnd( nullptr ),
	m_readHead( nullptr ),
	m_writeHead ( nullptr ),
	m_bufferIsExternal( true ),
	m_resizable(false)
{
}

vsStore::vsStore( size_t maxSize ):
	m_buffer( new char[maxSize] ),
	m_bufferLength( maxSize ),
	m_bufferEnd( &m_buffer[m_bufferLength] ),
	m_readHead( m_buffer ),
	m_writeHead( m_buffer ),
	m_bufferIsExternal( false ),
	m_resizable(false)
{
}

vsStore::vsStore( char *buffer, int bufferLength ):
	m_buffer( buffer ),
	m_bufferLength( bufferLength ),
	m_bufferEnd( &m_buffer[m_bufferLength] ),
	m_readHead( m_buffer ),
	m_writeHead( m_bufferEnd ),
	m_bufferIsExternal( true ),
	m_resizable(false)
{
}

vsStore::vsStore( const vsStore& other ):
	m_buffer( new char[ other.m_bufferLength ] ),
	m_bufferLength( other.m_bufferLength ),
	m_bufferEnd( &m_buffer[m_bufferLength] ),
	m_readHead( m_buffer ),
	m_writeHead( m_bufferEnd ),
	m_bufferIsExternal( false ),
	m_resizable(false)
{
	memcpy( m_buffer, other.m_buffer, m_bufferLength );
}

vsStore::~vsStore()
{
	if( !m_bufferIsExternal )
		delete [] m_buffer;
	m_buffer = nullptr;
}

void
vsStore::SetLength(size_t l)
{
	vsAssert(l <= m_bufferLength, "Tried to set length of vsStore to larger than its buffer!");
	m_writeHead = m_readHead + l;
}

void
vsStore::Rewind()
{
	m_readHead = m_buffer;
}

void
vsStore::EraseReadBytes()
{
	if ( m_readHead != m_buffer )
	{
		int bytesRead = m_readHead - m_buffer;
		memmove( m_buffer, m_readHead, BytesLeftForReading() );
		m_readHead -= bytesRead;
		m_writeHead -= bytesRead;
		vsAssert( m_readHead == m_buffer, "Maths error in vsStore" );
	}
}

void
vsStore::AdvanceReadHead( size_t bytes )
{
	m_readHead += bytes;
}

void
vsStore::AdvanceWriteHead( size_t bytes )
{
	m_writeHead += bytes;
}

void
vsStore::SeekReadHeadTo( size_t l )
{
	m_readHead = m_buffer + l;
}

size_t
vsStore::GetReadHeadPosition()
{
	return m_readHead - m_buffer;
}

void
vsStore::RewindWriteHeadTo( size_t l )
{
	m_writeHead = m_buffer + l;
}

void
vsStore::Clear()
{
	m_writeHead = m_readHead = m_buffer;
}

void
vsStore::_AssertBytesLeftForWriting(size_t bytes)
{
	if ( BytesLeftForWriting() < bytes )
	{
		vsLog("Tried to write past the end of a vsStore");
		vsLog("Buffer size:  %d bytes", BufferLength());
		vsLog("Currently in use:  %d bytes", Length());
		vsLog("Tried to write:  %d bytes", bytes);
		vsAssert( BytesLeftForWriting() >= bytes, "Tried to write past the end of the vsStore!" );
	}
}

void
vsStore::Append( vsStore *o )
{
	_EnsureBytesLeftForWriting( o->Length() );

	memcpy( m_writeHead, o->m_buffer, o->Length() );
	m_writeHead += o->Length();
}

void
vsStore::WriteInt8( int8_t v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}

void
vsStore::WriteUint8( uint8_t v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	//memcpy( m_writeHead, &v, sizeof(v) );
	*(uint8_t*)m_writeHead = v;
	m_writeHead += sizeof(v);
}

void
vsStore::WriteInt16( int16_t v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	v = htons(v);
	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}

void
vsStore::WriteUint16( uint16_t v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	v = htons(v);
	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}

void
vsStore::WriteUint16Native( uint16_t v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}

void
vsStore::WriteInt32( int32_t v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	v = htonl(v);
	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}

void
vsStore::WriteUint32( uint32_t v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	v = htonl(v);
	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}

void
vsStore::WriteFloat( float v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}

void
vsStore::WriteString( const vsString &string )
{
	int16_t s = (int16_t)string.size();

	WriteInt16(s);
	WriteBuffer(string.c_str(), s);
	// for ( int16_t i = 0; i < s; i++ )
	// {
	// 	WriteInt8( string[i] );
	// }
}

void
vsStore::WriteBuffer( const void *buffer, size_t bufferLength )
{
	_EnsureBytesLeftForWriting( bufferLength );

	memcpy( m_writeHead, buffer, bufferLength );
	m_writeHead += bufferLength;
}

size_t
vsStore::ReadBuffer( void *buffer, size_t bufferLength )
{
	size_t bytesToRead = vsMin( bufferLength, BytesLeftForReading() );

	memcpy( buffer, m_readHead, bytesToRead );
	m_readHead += bytesToRead;
	return bytesToRead;
}

vsString
vsStore::ReadString()
{
	vsString result;
	int s = ReadInt16();	// how long is the string?
	result.reserve(s);
	for ( int i = 0; i < s; i++ )
	{
		result.append(1, ReadInt8());
	}

	return result;
}

bool
vsStore::ReadLine( vsString *string )
{
	if ( BytesLeftForReading() <= 0 )
	{
		return false;
	}
	*string = vsEmptyString;

	while( m_readHead != m_writeHead )
	{
		if ( m_readHead[0] == '\n' || m_readHead[0] == 0 )
		{
			m_readHead++;
			break;
		}
		if ( m_readHead[0] != '\r' )
		{
			string->append(1, m_readHead[0]);
		}
		m_readHead++;
	}

	return true;
}

void
vsStore::ReadBufferAsString( vsString *string )
{
	*string = vsEmptyString;

	while( BytesLeftForReading() )
	{
		string->append(1, ReadInt8());
	}
}

void
vsStore::WriteVoidStar( void *v )
{
	_EnsureBytesLeftForWriting( sizeof(v) );

	memcpy( m_writeHead, &v, sizeof(v) );
	m_writeHead += sizeof(v);
}



int8_t
vsStore::ReadInt8()
{
	int8_t v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	memcpy( &v, m_readHead, sizeof(v) );
	m_readHead += sizeof(v);

	return v;
}

uint8_t
vsStore::ReadUint8()
{
	uint8_t v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	//memcpy( &v, m_readHead, sizeof(v) );
	v = *(uint8_t *)m_readHead;
	m_readHead += sizeof(v);

	return v;
}

uint8_t
vsStore::PeekUint8()
{
	uint8_t v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	//memcpy( &v, m_readHead, sizeof(v) );
	v = *(uint8_t *)m_readHead;

	return v;
}


int16_t
vsStore::ReadInt16()
{
	int16_t v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	memcpy( &v, m_readHead, sizeof(v) );
	v = ntohs(v);
	m_readHead += sizeof(v);

	return v;
}


uint16_t
vsStore::ReadUint16()
{
	uint16_t v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	memcpy( &v, m_readHead, sizeof(v) );
	v = ntohs(v);
	m_readHead += sizeof(v);

	return v;
}


int32_t
vsStore::ReadInt32()
{
	int32_t v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	memcpy( &v, m_readHead, sizeof(v) );
	v = ntohl(v);
	m_readHead += sizeof(v);

	return v;
}


uint32_t
vsStore::ReadUint32()
{
	uint32_t v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	memcpy( &v, m_readHead, sizeof(v) );
	v = ntohl(v);
	m_readHead += sizeof(v);

	return v;
}


float
vsStore::ReadFloat()
{
	float v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	memcpy( &v, m_readHead, sizeof(v) );
	m_readHead += sizeof(v);

	return v;
}

void *
vsStore::ReadVoidStar()
{
	void * v;
	vsAssert( BytesLeftForReading() >= sizeof(v), "Tried to read past the end of the vsStore!" );

	memcpy( &v, m_readHead, sizeof(v) );
	m_readHead += sizeof(v);

	return v;
}


void
vsStore::WriteVector2D(const vsVector2D &v)
{
	WriteFloat( v.x );
	WriteFloat( v.y );
}

void
vsStore::ReadVector2D(vsVector2D *v)
{
	// important note:  when the results of function calls are used as arguments to a function or constructor,
	// their order of operation is NOT DEFINED;  that means that they could be called in any order!  Usually,
	// that doesn't matter.  However, here we might be tempted to just create a result vector using
	// a constructor like this:  result( ReadFloat(), ReadFloat() );.  This might work with some compilers, but
	// will break with others, and yield swapped x and y coordinates!  For this reason, we must assign these values
	// to intermediate values, before reconstructing our vector!

	float x = ReadFloat();
	float y = ReadFloat();
	v->Set( x, y );
}

void
vsStore::WriteVector3D(const vsVector3D &v)
{
	WriteFloat( v.x );
	WriteFloat( v.y );
	WriteFloat( v.z );
}

void
vsStore::ReadVector3D(vsVector3D *v)
{
	float x = ReadFloat();
	float y = ReadFloat();
	float z = ReadFloat();

	v->Set( x, y, z );
}

void
vsStore::WriteVector4D(const vsVector4D &v)
{
	WriteFloat( v.x );
	WriteFloat( v.y );
	WriteFloat( v.z );
	WriteFloat( v.w );
}

void
vsStore::ReadVector4D(vsVector4D *v)
{
	float x = ReadFloat();
	float y = ReadFloat();
	float z = ReadFloat();
	float w = ReadFloat();

	v->Set( x, y, z, w );
}

void
vsStore::WriteColor(const vsColor &v)
{
	WriteFloat( v.r );
	WriteFloat( v.g );
	WriteFloat( v.b );
	WriteFloat( v.a );
}

void
vsStore::ReadColor(vsColor *c)
{
	float r = ReadFloat();
	float g = ReadFloat();
	float b = ReadFloat();
	float a = ReadFloat();

	c->Set( r, g, b, a );
}

void
vsStore::WriteColorPacked(const vsColorPacked &c)
{
	WriteUint8( c.r );
	WriteUint8( c.g );
	WriteUint8( c.b );
	WriteUint8( c.a );
}

void
vsStore::ReadColorPacked(vsColorPacked *c)
{
	uint8_t r = ReadUint8();
	uint8_t g = ReadUint8();
	uint8_t b = ReadUint8();
	uint8_t a = ReadUint8();

	c->Set(r,g,b,a);
}

void
vsStore::WriteLight(const vsLight &l)
{
	WriteUint8( l.GetType() );
	WriteVector3D( l.GetPosition() );
	WriteVector3D( l.GetDirection() );
	WriteColor( l.GetColor() );
	WriteColor( l.GetAmbientColor() );
	WriteColor( l.GetSpecularColor() );
}

void
vsStore::ReadLight(vsLight *l)
{
	l->SetType( (vsLight::Type)ReadUint8() );
	vsVector3D pos, dir;
	vsColor color, ambientColor, specularColor;

	ReadVector3D(&pos);
	ReadVector3D(&dir);
	ReadColor(&color);
	ReadColor(&ambientColor);
	ReadColor(&specularColor);

	l->SetPosition( pos );
	l->SetDirection( dir );
	l->SetColor( color );
	l->SetAmbientColor( ambientColor );
	l->SetSpecularColor( specularColor );
}

void
vsStore::WriteFog(const vsFog &f)
{
	WriteColor( f.GetColor() );
	WriteUint8( f.IsLinear() );
	if ( f.IsLinear() )
	{
		WriteFloat( f.GetStart() );
		WriteFloat( f.GetEnd() );
	}
	else
	{
		WriteFloat( f.GetDensity() );
	}
}

void
vsStore::ReadFog(vsFog *f)
{
	vsColor color;
	float density, start, end;

	ReadColor(&color);
	bool isLinear = ReadUint8() != 0;

	if ( isLinear )
	{
		start = ReadFloat();
		end = ReadFloat();
		f->SetLinear( color, start, end );
	}
	else
	{
		density = ReadFloat();
		f->SetExponential( color, density );
	}
}

void
vsStore::WriteTransform2D(const vsTransform2D &v)
{
	WriteVector2D( v.GetTranslation() );
	WriteFloat( v.GetAngle().Get() );
	WriteVector2D( v.GetScale() );
}

void
vsStore::ReadTransform2D(vsTransform2D *t)
{
    vsVector2D pos, scale;
    float angle;

	ReadVector2D(&pos);
	angle = ReadFloat();
	ReadVector2D(&scale);

    t->SetTranslation(pos);
    t->SetAngle(angle);
    t->SetScale(scale);
}

void
vsStore::WriteMatrix4x4(const vsMatrix4x4 &m)
{
	_EnsureBytesLeftForWriting( sizeof(m) );
	memcpy( m_writeHead, &m, sizeof(m) );
	m_writeHead += sizeof(m);
}

void
vsStore::ReadMatrix4x4(vsMatrix4x4 *m)
{
	memcpy( m, m_readHead, sizeof(vsMatrix4x4) );
	m_readHead += sizeof(vsMatrix4x4);
}

void
vsStore::WriteMaterial(const vsMaterial &m)
{
	//WriteColor(m.m_color);
	//WriteColor(m.m_specularColor);
	//WriteUint8(m.m_drawMode);
	//WriteVoidStar(m.GetTexture());
}

void
vsStore::ReadMaterial(vsMaterial *m)
{
	//ReadColor(&m->m_color);
	//ReadColor(&m->m_specularColor);
	//m->m_drawMode = (vsDrawMode)ReadUint8();
	//m->SetTexture( (vsTexture*)ReadVoidStar() );
}

void
vsStore::WriteBox2D(const vsBox2D &box)
{
	WriteVector2D(box.GetMin());
	WriteVector2D(box.GetMax());
}

void
vsStore::ReadBox2D(vsBox2D *box)
{
	vsVector2D min, max;
	ReadVector2D(&min);
	ReadVector2D(&max);
	box->Set(min,max);
}

bool
vsStore::Compress()
{
	Rewind();

	vsStore zipStore(m_bufferLength + 1024); // a *little* extra space buffer for in case we don't get much compression

	z_stream zipstream;
	zipstream.zalloc = Z_NULL;
	zipstream.zfree = Z_NULL;
	zipstream.opaque = Z_NULL;

	int ret = deflateInit(&zipstream, Z_DEFAULT_COMPRESSION);
	if ( ret != Z_OK )
	{
		vsLog("vsStore::Compress: deflateInit error: %d", ret);
		return false;
	}
	zipstream.avail_in = BytesLeftForReading();
	zipstream.next_in = (Bytef*)GetReadHead();
	zipstream.avail_out = m_bufferLength;
	zipstream.next_out = (Bytef*)zipStore.GetWriteHead();

	ret = deflate(&zipstream, Z_FINISH);
	vsAssert(ret == Z_STREAM_END, "vsStore::Compress: Failed to fit compressed stream into available space");

	int compressedBytes = m_bufferLength - zipstream.avail_out;
	zipStore.AdvanceWriteHead(compressedBytes);
	// if ( compressedBytes > 0 )
	// 	_WriteFinalBytes_Buffered(zipBuffer, compressedBytes);

	deflateEnd(&zipstream);

	Clear();
	Append(&zipStore);

	return true;
}

bool
vsStore::Expand()
{
	// We're going to need to inflate this date twice;  once to see how big it
	// turns out to be, and once to actually read it.
	z_stream zipstream;
	zipstream.zalloc = Z_NULL;
	zipstream.zfree = Z_NULL;
	zipstream.opaque = Z_NULL;
	zipstream.avail_in = BytesLeftForReading();
	zipstream.next_in = (Bytef*)GetReadHead();

	int ret = inflateInit(&zipstream);
	if ( ret != Z_OK )
	{
		vsAssert( ret == Z_OK, vsFormatString("inflateInit error: %d", ret) );
		return false;
	}

	int decompressedSize = 0;
	const int zipBufferSize = 1024 * 100;
	char zipBuffer[zipBufferSize];
	do
	{
		zipstream.avail_out = zipBufferSize;
		zipstream.next_out = (Bytef*)zipBuffer;
		int ret = inflate(&zipstream, Z_NO_FLUSH);
		vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered in destructor");

		int decompressedBytes = zipBufferSize - zipstream.avail_out;
		decompressedSize += decompressedBytes;

	}while( zipstream.avail_out == 0 );
	inflateEnd(&zipstream);

	vsStore *inflateStore = new vsStore( decompressedSize );

	// Now let's decompress it FOR REAL.
	Rewind();
	zipstream.zalloc = Z_NULL;
	zipstream.zfree = Z_NULL;
	zipstream.opaque = Z_NULL;
	zipstream.avail_in = BytesLeftForReading();
	zipstream.next_in = (Bytef*)GetReadHead();
	ret = inflateInit(&zipstream);
	if ( ret != Z_OK )
	{
		vsAssert( ret == Z_OK, vsFormatString("inflateInit error: %d", ret) );
		return false;
	}

	do
	{
		zipstream.avail_out = zipBufferSize;
		zipstream.next_out = (Bytef*)zipBuffer;
		int ret = inflate(&zipstream, Z_NO_FLUSH);

		if ( ret != Z_OK && ret != Z_STREAM_END )
		{
			switch ( ret )
			{
				case Z_NEED_DICT:
					vsLog("Failed to open zip data; dictionary requested");
					break;
				case Z_DATA_ERROR:
					vsLog("Failed to open zip data;  data error:  %s", zipstream.msg);
					break;
				case Z_MEM_ERROR:
					vsLog("Failed to open zip data;  memory error");
					break;
				case Z_BUF_ERROR:
					vsLog("Failed to open zip data;  insufficient buffer space");
					break;
				case Z_OK:
				default:
					break;
			}
			return false;
		}

		// vsAssert(ret != Z_OK, "Zip State ");

		int decompressedBytes = zipBufferSize - zipstream.avail_out;
		inflateStore->WriteBuffer( zipBuffer, decompressedBytes );

	}while( zipstream.avail_out == 0 );
	inflateEnd(&zipstream);


	_ReplaceBuffer( inflateStore->Length() );
	Append(inflateStore);
	vsDelete(inflateStore);

	return true;
}

void
vsStore::_ReplaceBuffer( size_t newLength )
{
	vsAssert( !m_bufferIsExternal, "Replacing an external vsStore buffer isn't supported" );

	vsDeleteArray(m_buffer);
	m_buffer = new char[newLength];
	m_bufferLength = newLength;
	m_bufferEnd = &m_buffer[m_bufferLength];
	m_readHead = m_writeHead = m_buffer;
}

void
vsStore::_ResizeBuffer( size_t newLength )
{
	vsLog("!! Auto-resizing vsStore to %d bytes", newLength);

	char* newBuffer = new char[newLength];
	size_t bytesRead = GetReadHead() - m_buffer;
	size_t bytesWritten = GetWriteHead() - m_buffer;
	memcpy( newBuffer, m_buffer, bytesWritten );

	vsDeleteArray(m_buffer);
	m_bufferLength = newLength;
	m_buffer = newBuffer;
	m_bufferEnd = &m_buffer[m_bufferLength];
	m_readHead = m_buffer + bytesRead;
	m_writeHead = m_buffer + bytesWritten;
}

void
vsStore::SetResizable()
{
	vsAssert(!m_bufferIsExternal, "Can't set a vsStore as resizable if we don't own our own buffer");
	m_resizable = true;
}

void
vsStore::_EnsureBytesLeftForWriting( size_t bytes )
{
	if ( !m_resizable )
		_AssertBytesLeftForWriting( bytes );
	else
	{
		if ( BytesLeftForWriting() < bytes )
		{
			// In general, we're going to double our size.  If doubling
			// isn't enough, then just go however big we have to go to fit
			// these bytes.
			size_t minBytesNeeded = Length() + bytes;
			size_t newLength = vsMax( minBytesNeeded, m_bufferLength * 2 );
			_ResizeBuffer( newLength );
		}
	}
}

