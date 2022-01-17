/*
 *  MEM_Serialiser.cpp
 *  MMORPG
 *
 *  Created by Trevor Powell on 16/06/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Serialiser.h"
#include "VS_Store.h"

#include "VS_Color.h"
#include "VS_File.h"
#include "VS_Vector.h"

vsSerialiser::vsSerialiser(vsStore *store, Type type):
	m_store(store),
	m_type(type)
{
}

vsSerialiserRead::vsSerialiserRead(vsStore *store):
	vsSerialiser(store, Type_Read)
{
}

void
vsSerialiserRead::Bool(bool &value)
{
	value = !!m_store->ReadInt8();
}

void
vsSerialiserRead::Int8(int8_t &value)
{
	value = m_store->ReadInt8();
}

void
vsSerialiserRead::Uint8(uint8_t &value)
{
	value = m_store->ReadUint8();
}

void
vsSerialiserRead::Int16(int16_t &value)
{
	value = m_store->ReadInt16();
}

void
vsSerialiserRead::Uint16(uint16_t &value)
{
	value = m_store->ReadUint16();
}

void
vsSerialiserRead::Int32(int32_t &value)
{
	value = m_store->ReadInt32();
}

void
vsSerialiserRead::Uint32(uint32_t &value)
{
	value = m_store->ReadUint32();
}

void
vsSerialiserRead::AssertInt32(int32_t value)
{
	int32_t actualValue = m_store->ReadInt32();

	vsAssert(value == actualValue, "Error in serialisation!");
}


void
vsSerialiserRead::Float(float &value)
{
	value = m_store->ReadFloat();
}

void
vsSerialiserRead::String( vsString &value )
{
	value = m_store->ReadString();
}

void
vsSerialiserRead::Vector2D( vsVector2D &value )
{
	m_store->ReadVector2D(&value);
}

void
vsSerialiserRead::Vector3D( vsVector3D &value )
{
	m_store->ReadVector3D(&value);
}

void
vsSerialiserRead::Vector4D( vsVector4D &value )
{
	m_store->ReadVector4D(&value);
}

void
vsSerialiserRead::Color( vsColor &value )
{
	m_store->ReadColor(&value);
}

void
vsSerialiserRead::ColorPacked( vsColorPacked &value )
{
	m_store->ReadColorPacked(&value);
}

vsSerialiserWrite::vsSerialiserWrite(vsStore *store):
vsSerialiser(store, Type_Write)
{
}

void
vsSerialiserWrite::Bool(bool &value)
{
	m_store->WriteInt8(value?1:0);
}

void
vsSerialiserWrite::Int8(int8_t &value)
{
	m_store->WriteInt8(value);
}

void
vsSerialiserWrite::Uint8(uint8_t &value)
{
	m_store->WriteUint8(value);
}

void
vsSerialiserWrite::Int16(int16_t &value)
{
	m_store->WriteInt16(value);
}

void
vsSerialiserWrite::Uint16(uint16_t &value)
{
	m_store->WriteUint16(value);
}

void
vsSerialiserWrite::Int32(int32_t &value)
{
	m_store->WriteInt32(value);
}

void
vsSerialiserWrite::Uint32(uint32_t &value)
{
	m_store->WriteUint32(value);
}

void
vsSerialiserWrite::Float(float &value)
{
	m_store->WriteFloat(value);
}

void
vsSerialiserWrite::String( vsString &value )
{
	m_store->WriteString(value);
}

void
vsSerialiserWrite::Vector2D( vsVector2D &value )
{
	m_store->WriteVector2D(value);
}

void
vsSerialiserWrite::Vector3D( vsVector3D &value )
{
	m_store->WriteVector3D(value);
}

void
vsSerialiserWrite::Vector4D( vsVector4D &value )
{
	m_store->WriteVector4D(value);
}

void
vsSerialiserWrite::Color( vsColor &value )
{
	m_store->WriteColor(value);
}

void
vsSerialiserWrite::ColorPacked( vsColorPacked &value )
{
	m_store->WriteColorPacked(value);
}

vsSerialiserReadStream::vsSerialiserReadStream(vsFile *file):
	vsSerialiser(nullptr, Type_Read),
	m_file(file)
{
	m_store = new vsStore(1024);
	// m_file->PeekBytes(m_store, m_store->BytesLeftForWriting());
	int bytes = m_file->ReadBytes( m_store->GetWriteHead(), m_store->BytesLeftForWriting() );
	m_store->AdvanceWriteHead(bytes);
	// m_file->StoreBytes(m_store, m_store->BytesLeftForWriting());
}

vsSerialiserReadStream::~vsSerialiserReadStream()
{
	m_file->ConsumeBytes( m_store->GetReadHeadPosition() );
	// Okay.  Any bytes that we didn't actually read, back up the file's
	// internal read head by that much.
	vsDelete(m_store);
}

void
vsSerialiserReadStream::Ensure(size_t bytes_required)
{
	if ( m_store->BytesLeftForReading() < bytes_required )
	{
		m_store->EraseReadBytes();
		int bytes = m_file->ReadBytes( m_store->GetWriteHead(), m_store->BytesLeftForWriting() );
		m_store->AdvanceWriteHead(bytes);
		// m_file->ConsumeBytes( m_store->GetReadHeadPosition() );
		// m_store->Clear();
		// m_file->PeekBytes(m_store, m_store->BytesLeftForWriting());
		vsAssertF(m_store->BytesLeftForReading() >= bytes_required, "Not enough data in file '%s' to fulfill stream?", m_file->GetFilename());
	}
}

void
vsSerialiserReadStream::Bool(bool &value)
{
	Ensure(sizeof(bool));
	value = !!m_store->ReadInt8();
}

void
vsSerialiserReadStream::Int8(int8_t &value)
{
	Ensure(sizeof(int8_t));
	value = m_store->ReadInt8();
}

void
vsSerialiserReadStream::Uint8(uint8_t &value)
{
	Ensure(sizeof(uint8_t));
	value = m_store->ReadUint8();
}

void
vsSerialiserReadStream::Int16(int16_t &value)
{
	Ensure(sizeof(int16_t));
	value = m_store->ReadInt16();
}

void
vsSerialiserReadStream::Uint16(uint16_t &value)
{
	Ensure(sizeof(uint16_t));
	value = m_store->ReadUint16();
}

void
vsSerialiserReadStream::Int32(int32_t &value)
{
	Ensure(sizeof(int32_t));
	value = m_store->ReadInt32();
}

void
vsSerialiserReadStream::AssertInt32(int32_t value)
{
	int32_t v;
	Int32(v);
	vsAssert(v == value, "Serialiser AssertInt32 failed?");
}


void
vsSerialiserReadStream::Uint32(uint32_t &value)
{
	Ensure(sizeof(uint32_t));
	value = m_store->ReadUint32();
}

void
vsSerialiserReadStream::Float(float &value)
{
	Ensure(sizeof(float));
	value = m_store->ReadFloat();
}

void
vsSerialiserReadStream::String( vsString &value )
{
	int bytes;
	Ensure( sizeof(int16_t) );

	// first, check how many bytes in this string.
	size_t	readHeadPos = m_store->GetReadHeadPosition();
	bytes = m_store->ReadInt16();
	m_store->SeekReadHeadTo(readHeadPos);

	Ensure( sizeof(int16_t) + bytes );
	value = m_store->ReadString();
}

void
vsSerialiserReadStream::Vector2D( vsVector2D &value )
{
	Ensure(sizeof(vsVector2D));
	m_store->ReadVector2D(&value);
}

void
vsSerialiserReadStream::Vector3D( vsVector3D &value )
{
	Ensure(sizeof(vsVector3D));
	m_store->ReadVector3D(&value);
}

void
vsSerialiserReadStream::Vector4D( vsVector4D &value )
{
	Ensure(sizeof(vsVector4D));
	m_store->ReadVector4D(&value);
}

void
vsSerialiserReadStream::Color( vsColor &value )
{
	Ensure(sizeof(vsColor));
	m_store->ReadColor(&value);
}

void
vsSerialiserReadStream::ColorPacked( vsColorPacked &value )
{
	Ensure(sizeof(vsColorPacked));
	m_store->ReadColorPacked(&value);
}

vsSerialiserWriteStream::vsSerialiserWriteStream(vsFile *file):
	vsSerialiser(nullptr, Type_Write),
	m_file(file)
{
	m_store = new vsStore(1024 * 100);
}

vsSerialiserWriteStream::~vsSerialiserWriteStream()
{
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	vsDelete(m_store);
}

void
vsSerialiserWriteStream::Ensure(size_t bytes)
{
	if ( m_store->BytesLeftForWriting() < bytes )
	{
		m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
		m_store->Clear();
		vsAssertF(m_store->BytesLeftForWriting() >= bytes, "Unable to fit %d bytes into store of size %d, while writing to file '%s'.", bytes, m_store->BytesLeftForWriting(), m_file->GetFilename() );
	}
}

void
vsSerialiserWriteStream::Bool(bool &value)
{
	Ensure(sizeof(int8_t));
	m_store->WriteInt8(value?1:0);
}

void
vsSerialiserWriteStream::Int8(int8_t &value)
{
	Ensure(sizeof(int8_t));
	m_store->WriteInt8(value);
}

void
vsSerialiserWriteStream::Uint8(uint8_t &value)
{
	Ensure(sizeof(uint8_t));
	m_store->WriteUint8(value);
}

void
vsSerialiserWriteStream::Int16(int16_t &value)
{
	Ensure(sizeof(int16_t));
	m_store->WriteInt16(value);
}

void
vsSerialiserWriteStream::Uint16(uint16_t &value)
{
	Ensure(sizeof(uint16_t));
	m_store->WriteUint16(value);
}

void
vsSerialiserWriteStream::Int32(int32_t &value)
{
	Ensure(sizeof(int32_t));
	m_store->WriteInt32(value);
}

void
vsSerialiserWriteStream::Uint32(uint32_t &value)
{
	Ensure(sizeof(uint32_t));
	m_store->WriteUint32(value);
}

void
vsSerialiserWriteStream::Float(float &value)
{
	Ensure(sizeof(float));
	m_store->WriteFloat(value);
}

void
vsSerialiserWriteStream::String( vsString &value )
{
	Ensure( sizeof(int16_t) + value.size() );
	m_store->WriteString(value);
}

void
vsSerialiserWriteStream::Vector2D( vsVector2D &value )
{
	Ensure( sizeof(vsVector2D) );
	m_store->WriteVector2D(value);
}

void
vsSerialiserWriteStream::Vector3D( vsVector3D &value )
{
	Ensure( sizeof(vsVector3D) );
	m_store->WriteVector3D(value);
}

void
vsSerialiserWriteStream::Vector4D( vsVector4D &value )
{
	Ensure( sizeof(vsVector4D) );
	m_store->WriteVector4D(value);
}

void
vsSerialiserWriteStream::ColorPacked( vsColorPacked &value )
{
	Ensure( sizeof(vsColorPacked) );
	m_store->WriteColorPacked(value);
}

void
vsSerialiserWriteStream::Color( vsColor &value )
{
	Ensure( sizeof(vsColor) );
	m_store->WriteColor(value);
}

