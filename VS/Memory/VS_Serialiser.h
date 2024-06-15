/*
 *  MEM_Serialiser.h
 *  MMORPG
 *
 *  Created by Trevor Powell on 16/06/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef MEM_SERIALISER_H
#define MEM_SERIALISER_H

class vsFile;
class vsStore;
class vsVector2D;
class vsVector3D;
class vsVector4D;
class vsColor;
class vsColorPacked;

class vsSerialiser
{
public:
	enum Type
	{
		Type_Read,
		Type_Write
	};
protected:
	vsStore *	m_store;
	Type		m_type;
public:

	vsSerialiser( vsStore *store, Type type );
	virtual ~vsSerialiser() {}

	Type			GetType() { return m_type; }

	virtual void	Bool(bool &value) = 0;

	virtual void	Int8(int8_t &value) = 0;
	virtual void	Uint8(uint8_t &value) = 0;

	virtual void	Int16(int16_t &value) = 0;
	virtual void	Uint16(uint16_t &value) = 0;

	virtual void	Int32(int32_t &value) = 0;
	virtual void	Uint32(uint32_t &value) = 0;

	virtual void	AssertInt32(int32_t value) = 0;

	virtual void	Float(float &value) = 0;

	virtual void	String( vsString &value ) = 0;
	virtual void	Vector2D( vsVector2D &value ) = 0;
	virtual void	Vector3D( vsVector3D &value ) = 0;
	virtual void	Vector4D( vsVector4D &value ) = 0;
	virtual void	Color( vsColor &value ) = 0;
	virtual void	ColorPacked( vsColorPacked &value ) = 0;
};

class vsSerialiserRead : public vsSerialiser
{
public:
	vsSerialiserRead( vsStore *store );

	virtual void	Bool(bool &value);

	virtual void	Int8(int8_t &value);
	virtual void	Uint8(uint8_t &value);

	virtual void	Int16(int16_t &value);
	virtual void	Uint16(uint16_t &value);

	virtual void	Int32(int32_t &value);
	virtual void	Uint32(uint32_t &value);

	virtual void	AssertInt32(int32_t value);

	virtual void	Float(float &value);

	virtual void	String( vsString &value );
	virtual void	Vector2D( vsVector2D &value );
	virtual void	Vector3D( vsVector3D &value );
	virtual void	Vector4D( vsVector4D &value );
	virtual void	Color( vsColor &value );
	virtual void	ColorPacked( vsColorPacked &value );
};

class vsSerialiserWrite : public vsSerialiser
{
public:
	vsSerialiserWrite( vsStore *store );

	virtual void	Bool(bool &value);

	virtual void	Int8(int8_t &value);
	virtual void	Uint8(uint8_t &value);

	virtual void	Int16(int16_t &value);
	virtual void	Uint16(uint16_t &value);

	virtual void	Int32(int32_t &value);
	virtual void	Uint32(uint32_t &value);

	virtual void	AssertInt32(int32_t value) { Int32(value); }

	virtual void	Float(float &value);

	virtual void	String( vsString &value );
	virtual void	Vector2D( vsVector2D &value );
	virtual void	Vector3D( vsVector3D &value );
	virtual void	Vector4D( vsVector4D &value );
	virtual void	Color( vsColor &value );
	virtual void	ColorPacked( vsColorPacked &value );
};

class vsSerialiserReadStream : public vsSerialiser
{
	vsFile * m_file;
	vsStore * m_store;

	void Ensure(size_t bytes);
public:
	vsSerialiserReadStream(vsFile *file);
	virtual ~vsSerialiserReadStream();

	bool IsOK() const;
	void SetError( const vsString& error );

	virtual void	Bool(bool &value);

	virtual void	Int8(int8_t &value);
	virtual void	Uint8(uint8_t &value);

	virtual void	Int16(int16_t &value);
	virtual void	Uint16(uint16_t &value);

	virtual void	Int32(int32_t &value);
	virtual void	Uint32(uint32_t &value);

	virtual void	AssertInt32(int32_t value);

	virtual void	Float(float &value);

	virtual void	String( vsString &value );
	virtual void	Vector2D( vsVector2D &value );
	virtual void	Vector3D( vsVector3D &value );
	virtual void	Vector4D( vsVector4D &value );
	virtual void	Color( vsColor &value );
	virtual void	ColorPacked( vsColorPacked &value );
};

class vsSerialiserWriteStream : public vsSerialiser
{
	vsFile * m_file;
	vsStore * m_store;

	void Ensure(size_t bytes);
public:
	vsSerialiserWriteStream( vsFile *file );
	virtual ~vsSerialiserWriteStream();

	void Flush();

	bool IsOK() const;
	void SetError( const vsString& error );

	virtual void	Bool(bool &value);

	virtual void	Int8(int8_t &value);
	virtual void	Uint8(uint8_t &value);

	virtual void	Int16(int16_t &value);
	virtual void	Uint16(uint16_t &value);

	virtual void	Int32(int32_t &value);
	virtual void	Uint32(uint32_t &value);

	virtual void	AssertInt32(int32_t value) { Int32(value); }

	virtual void	Float(float &value);

	virtual void	String( vsString &value );
	virtual void	Vector2D( vsVector2D &value );
	virtual void	Vector3D( vsVector3D &value );
	virtual void	Vector4D( vsVector4D &value );
	virtual void	Color( vsColor &value );
	virtual void	ColorPacked( vsColorPacked &value );
};

#endif // FS_SERIALISER_H
