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

class vsStore;
class vsVector2D;
class vsColor;

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
	
	Type			GetType() { return m_type; }
	
	virtual void	Bool(bool &value) = 0;
	
	virtual void	Int8(int8 &value) = 0;
	virtual void	Uint8(uint8 &value) = 0;
	
	virtual void	Int16(int16 &value) = 0;
	virtual void	Uint16(uint16 &value) = 0;
		
	virtual void	Int32(int32 &value) = 0;
	virtual void	Uint32(uint32 &value) = 0;
	
	virtual void	AssertInt32(int32 value) = 0;

	virtual void	Float(float &value) = 0;
	
	virtual void	String( vsString &value ) = 0;
	virtual void	Vector2D( vsVector2D &value ) = 0;
	virtual void	Vector3D( vsVector3D &value ) = 0;
	virtual void	Color( vsColor &value ) = 0;
};

class vsSerialiserRead : public vsSerialiser
{
public:
	vsSerialiserRead( vsStore *store );

	virtual void	Bool(bool &value);

	virtual void	Int8(int8 &value);
	virtual void	Uint8(uint8 &value);
	
	virtual void	Int16(int16 &value);
	virtual void	Uint16(uint16 &value);
	
	virtual void	Int32(int32 &value);
	virtual void	Uint32(uint32 &value);
	
	virtual void	AssertInt32(int32 value);

	virtual void	Float(float &value);
	
	virtual void	String( vsString &value );
	virtual void	Vector2D( vsVector2D &value );
	virtual void	Vector3D( vsVector3D &value );
	virtual void	Color( vsColor &value );
};

class vsSerialiserWrite : public vsSerialiser
{
public:
	vsSerialiserWrite( vsStore *store );
	
	virtual void	Bool(bool &value);

	virtual void	Int8(int8 &value);
	virtual void	Uint8(uint8 &value);
	
	virtual void	Int16(int16 &value);
	virtual void	Uint16(uint16 &value);
		
	virtual void	Int32(int32 &value);
	virtual void	Uint32(uint32 &value);
	
	virtual void	AssertInt32(int32 value) { int32 v = value;  return Int32(v); }

	virtual void	Float(float &value);
	
	virtual void	String( vsString &value );
	virtual void	Vector2D( vsVector2D &value );
	virtual void	Vector3D( vsVector3D &value );
	virtual void	Color( vsColor &value );
};

#endif // FS_SERIALISER_H
