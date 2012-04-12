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
vsSerialiserRead::Int8(int8 &value)
{
	value = m_store->ReadInt8();
}

void
vsSerialiserRead::Uint8(uint8 &value)
{
	value = m_store->ReadUint8();
}

void
vsSerialiserRead::Int16(int16 &value)
{
	value = m_store->ReadInt16();
}

void
vsSerialiserRead::Uint16(uint16 &value)
{
	value = m_store->ReadUint16();
}

void
vsSerialiserRead::Int32(int32 &value)
{
	value = m_store->ReadInt32();
}

void
vsSerialiserRead::Uint32(uint32 &value)
{
	value = m_store->ReadUint32();
}

void
vsSerialiserRead::AssertInt32(int32 value)
{
	int32 actualValue = m_store->ReadInt32();

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
vsSerialiserWrite::Int8(int8 &value)
{
	m_store->WriteInt8(value);
}

void
vsSerialiserWrite::Uint8(uint8 &value)
{
	m_store->WriteUint8(value);
}

void
vsSerialiserWrite::Int16(int16 &value)
{
	m_store->WriteInt16(value);
}

void
vsSerialiserWrite::Uint16(uint16 &value)
{
	m_store->WriteUint16(value);
}

void
vsSerialiserWrite::Int32(int32 &value)
{
	m_store->WriteInt32(value);
}

void
vsSerialiserWrite::Uint32(uint32 &value)
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

