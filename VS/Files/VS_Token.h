/*
 *  FS_Token.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 19/01/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef FS_TOKEN_H
#define FS_TOKEN_H

#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_StringTable.h"
class vsSerialiser;

class vsToken
{
public:
	enum Type
	{
		Type_None,
		Type_Label,
		Type_String,
		Type_Float,
		Type_Integer,
		Type_OpenBrace,
		Type_CloseBrace,
		Type_Equals,
		Type_NewLine,
		Type_Semicolon,

		TYPE_MAX
	};
private:
	Type		m_type;
	vsString	m_string;
	union
	{
		float		m_float;
		int32_t		m_int;
	};
	void SetStringField( const vsString& s );

	bool ExtractLabelString( vsString* output, vsString& input );
	bool ExtractFloat( float* output, vsString& input );
	bool ExtractInteger( int* output, vsString& input );

public:

	vsToken();
	vsToken(Type t);
	vsToken(const vsToken& other);

	explicit vsToken( const vsString& string );
	explicit vsToken( int i );
	explicit vsToken( float f );

	~vsToken();

	bool		ExtractFrom( vsString &string );

	// back to a string, exactly as we were extracted from.
	//
	// N.B.: If we're of "String" type, the returned string will have quotes
	// around it and the string contents will be escaped (\" instead of ", etc)
	vsString	BackToString() const;

	void		SerialiseBinaryV1( vsSerialiser *s, vsStringTable& stringTable );
	void		SerialiseBinaryV2( vsSerialiser *s );
	void		PopulateStringTable( vsStringTable& array );

	void		SetType(Type t);
	Type		GetType() const { return m_type; }
	vsString	AsString() const;			// give us our value as a string.  (If we're of string type, this will NOT have quotes around it)
	int			AsInteger() const;
	float		AsFloat() const;

	void		SetString(const vsString &value);
	void		SetLabel(const vsString &value);
	void		SetInteger(int value);
	void		SetFloat(float value);

	bool		IsType( Type type ) const;
	bool		IsNumeric() const { return IsType( Type_Float ) || IsType( Type_Integer ); }

	vsToken& operator=( const vsToken& other );
	bool operator==( const vsToken& other ) const;
	bool operator!=( const vsToken& other ) const { return ! ((*this) == other); }

	bool operator==( const vsString& str ) const;
	bool operator!=( const vsString& str ) const { return ! ((*this) == str); }
};

#endif // FS_TOKEN_H

