/*
 *  FS_Record.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef FS_RECORD_H
#define FS_RECORD_H

#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_StringTable.h"
#include "VS/Utils/VS_ArrayStore.h"
#include "VS/Math/VS_Quaternion.h"
#include "VS/Utils/VS_Pool.h"

class vsVector2D;
class vsColor;
class vsFile;
class vsSerialiser;

class vsSerialiserReadStream;

#include "VS_Token.h"

class vsRecord
{
	vsToken		m_label;

	vsArray<vsToken>	m_token;

	vsArray<vsRecord*>	m_childList;
	vsRecord *	m_lastChild;

	// vsPool<vsRecord> *m_pool;

	bool		m_inBlock;
	bool		m_hasLabel;
	bool		m_lineIsOpen;

	int			m_streamModeChildCount;
	bool		m_streamMode;

	float		GetArg(int i);

	void		LoadBinaryV1( vsFile *file );
	void		SerialiseBinaryV1( vsSerialiser *s, vsStringTable& stringTable );
	void		SerialiseBinaryV2( vsSerialiser *s );
	void		PopulateStringTable( vsStringTable& array );
	void		Clean();

public:
	vsRecord();
	vsRecord( const char* fromString );
	vsRecord( const vsString& fromString );
	~vsRecord();

	// void		SetPool( vsPool<vsRecord> *pool ) { m_pool = pool; }
	void		Init();

	bool		Parse( vsFile *file );                 // attempt to fill out this vsRecord from a vsString
	bool		ParseString( vsString string );
	bool		AppendToken( const vsToken &token );   // add this token to me.
	vsString	ToString( int childLevel = 0 ) const;  // convert this vsRecord into a vsString.

	void LoadFromFilename( const vsString& filename );

	bool		LoadBinary( vsFile *file );
	void		SaveBinary( vsFile *file );
	bool		SerialiseBinary( vsSerialiser *s );

	vsToken &			GetLabel() { return m_label; }
	const vsToken &		GetLabel() const { return m_label; }
	const vsToken &		Label() const { return GetLabel(); }
	void				SetLabel(const vsString &label);

	vsToken &			GetToken(int i) { return const_cast<vsToken&>( const_cast<const vsRecord*>(this)->GetToken(i)); }
	const vsToken &		GetToken(int i) const;
	int					GetTokenCount() const { return m_token.ItemCount(); }
	void				SetTokenCount( int count );

	vsRecord *			GetChild(int i);
	int					GetChildCount() const { return m_streamMode ? m_streamModeChildCount : m_childList.ItemCount(); }
	int					GetChildCount(const vsString& label) const;	// returns number of children with this label
	void				AddChild(vsRecord *record);
	void				SetExpectedChildCount( int count );
	//void				SetChildCount( int count );

	bool				Bool() const;
	int					Int() const;
	vsString			String() const;
	vsColor				Color() const;
	vsVector2D			Vector2D() const;
	vsVector3D			Vector3D() const;
	vsVector4D			Vector4D() const;
	vsQuaternion		Quaternion() const;

	void				SetRect(float x, float y, float width, float height);
	void				SetInt(int value);
	void				SetFloat(float value);

	bool operator==(const char* string) const { return operator==( vsString(string) ); }
	bool operator==(const vsString& string) const;


	// Testbed for streaming record loads
	int LoadBinary_Stream_Init( vsSerialiserReadStream *s );
	int LoadBinary_Stream( vsSerialiserReadStream *s );
};

#endif // FS_RECORD_H

