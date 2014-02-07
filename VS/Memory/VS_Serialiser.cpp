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
vsSerialiserRead::Color( vsColor &value )
{
	m_store->ReadColor(&value);
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
vsSerialiserWrite::Color( vsColor &value )
{
	m_store->WriteColor(value);
}

vsSerialiserReadStream::vsSerialiserReadStream(vsFile *file):
	vsSerialiser(NULL, Type_Read),
	m_file(file)
{
	m_store = new vsStore(1024);
	m_file->StoreBytes(m_store, m_store->BytesLeftForWriting());
}

vsSerialiserReadStream::~vsSerialiserReadStream()
{
	vsDelete(m_store);
}

void
vsSerialiserReadStream::Ensure(size_t bytes)
{
	if ( m_store->BytesLeftForReading() < bytes )
	{
		size_t bytes = m_store->BytesLeftForReading();
		vsStore temp(bytes);
		for ( size_t i = 0; i < bytes; i++ )
		{
			temp.WriteInt8(m_store->ReadInt8());
		}
		m_store->Clear();
		m_store->Append(&temp);
		m_file->StoreBytes(m_store, m_store->BytesLeftForWriting());
		vsAssert(m_store->BytesLeftForReading() >= bytes, "Not enough data to fulfill stream?");
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
	Ensure(sizeof(vsString));
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
vsSerialiserReadStream::Color( vsColor &value )
{
	Ensure(sizeof(vsColor));
	m_store->ReadColor(&value);
}

vsSerialiserWriteStream::vsSerialiserWriteStream(vsFile *file):
	vsSerialiser(NULL, Type_Write),
	m_file(file)
{
	m_store = new vsStore(1024);
}

vsSerialiserWriteStream::~vsSerialiserWriteStream()
{
	vsDelete(m_store);
}

void
vsSerialiserWriteStream::Bool(bool &value)
{
	m_store->WriteInt8(value?1:0);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Int8(int8_t &value)
{
	m_store->WriteInt8(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Uint8(uint8_t &value)
{
	m_store->WriteUint8(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Int16(int16_t &value)
{
	m_store->WriteInt16(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Uint16(uint16_t &value)
{
	m_store->WriteUint16(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Int32(int32_t &value)
{
	m_store->WriteInt32(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Uint32(uint32_t &value)
{
	m_store->WriteUint32(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Float(float &value)
{
	m_store->WriteFloat(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::String( vsString &value )
{
	m_store->WriteString(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Vector2D( vsVector2D &value )
{
	m_store->WriteVector2D(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Vector3D( vsVector3D &value )
{
	m_store->WriteVector3D(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

void
vsSerialiserWriteStream::Color( vsColor &value )
{
	m_store->WriteColor(value);
	m_file->StoreBytes(m_store, m_store->BytesLeftForReading());
	m_store->Clear();
}

