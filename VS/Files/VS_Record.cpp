/*
 *  FS_Record.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS/Files/VS_Record.h"
#include "VS/Files/VS_File.h"

#include "VS/Graphics/VS_Color.h"
#include "VS/Math/VS_Vector.h"


vsRecord::vsRecord()
{
	m_childList.Clear();
	m_hasLabel = false;
	m_lastChild = NULL;

	Init();
}

vsRecord::vsRecord( const char* fromString )
{
	m_childList.Clear();
	m_hasLabel = false;
	m_lastChild = NULL;

	Init();
	ParseString( fromString );
}

vsRecord::vsRecord( const vsString& fromString )
{
	m_childList.Clear();
	m_hasLabel = false;
	m_lastChild = NULL;

	Init();
	ParseString( fromString );
}

vsRecord::~vsRecord()
{
	Init();
}

void
vsRecord::Init()
{
	m_childList.Clear();
	m_token.Clear();
	m_inBlock = false;
	m_hasLabel = false;
	m_lineIsOpen = true;
	m_lastChild = NULL;
}

vsString
vsRecord::ToString( int childLevel )
{
	vsString tabbing = vsEmptyString;
	vsString result = vsEmptyString;

	for( int i = 0; i < childLevel; i++ )
	{
		tabbing += "\t";
	}



	result += tabbing + m_label.BackToString();

	for ( int i = 0; i < m_token.ItemCount(); i++ )
	{
		result += " ";
		result += m_token[i].BackToString();
	}

	if ( !m_childList.IsEmpty() )
	{
		result += "\n";
		result += tabbing + "{\n";

		for ( vsLinkedListStore<vsRecord>::Iterator iter = m_childList.Begin(); iter != m_childList.End(); iter++ )
		{
			vsRecord *child = *iter;
			result += child->ToString( childLevel+1 );
		}

		result += tabbing + "}";
	}
	result += "\n";
	return result;
}


vsToken &
vsRecord::GetToken(int id)
{
	vsAssert( id >= 0 && id < m_token.ItemCount(),  "Requested token with too high a token ID number!" );

	return m_token[id];
}

void
vsRecord::SetTokenCount(int count)
{
	while ( m_token.ItemCount() > count )
	{
		m_token.RemoveItem( --m_token.End() );
	}
	while ( m_token.ItemCount() < count )
	{
		m_token.AddItem( vsToken() );
	}
}

vsRecord *
vsRecord::GetChild(int i)
{
	vsLinkedListStore<vsRecord>::Iterator iter = m_childList.Begin();
	while(i-->0)
	{
		iter++;
	}
	return *iter;
}

int
vsRecord::GetChildCount(const vsString& label)
{
	int count = 0;
	for ( vsLinkedListStore<vsRecord>::Iterator iter = m_childList.Begin();
			iter != m_childList.End();
			iter++ )
	{
		if ( (*iter)->GetLabel().AsString() == label )
		{
			count++;
		}
	}
	return count;
}

void
vsRecord::AddChild(vsRecord *record)
{
	m_childList.AddItem(record);
	m_lastChild = record;
}

bool
vsRecord::Parse( vsFile *file )
{
	bool done = false;
	bool valid = false;
	bool haveLine = true;

	while ( haveLine && (!valid || !done))
	{
		done = true;

		vsString parseString;
		haveLine = file->ReadLine(&parseString);

		if ( haveLine )
		{
			vsString nextLine;
			bool haveNextLine = file->PeekLine( &nextLine );
			if ( haveNextLine )
			{
				vsToken t;
				t.ExtractFrom(nextLine);
				if( t.GetType() == vsToken::Type_OpenBrace )
				{
					// next line starts with an open brace -- append the next line to this one, for the purposes of parsing!

					file->ReadLine( &nextLine );
					parseString = parseString + nextLine;
				}
			}

			valid = ParseString( parseString );
			if ( m_inBlock )
			{
				done = false;
			}
		}
		else
		{
			if ( m_inBlock )
			{
				vsAssert( valid, "Error, reached end of file and didn't find closing braces!" );
			}
		}
	}

	return valid;
}

bool
vsRecord::ParseString( vsString parseString )
{
	vsToken t;

	bool valid = false;


	while ( !parseString.empty() )
	{
		t.ExtractFrom(parseString);
		if( t.GetType() != vsToken::Type_None )
		{
			valid = true;
		}
		AppendToken(t);
	}

	AppendToken( vsToken( vsToken::Type_NewLine ) );
	return valid;
}

bool
vsRecord::AppendToken( const vsToken &t )
{
	if ( t.GetType() == vsToken::Type_None )	// if we couldn't get even one token from the line, then we've failed to get a record.
		return true;	// sure, I'll take credit for a none-token.

	if ( !m_inBlock )	// I'm not in a block, therefore, add this to my main line.
	{
		if ( m_token.IsEmpty() && m_hasLabel == false && t.GetType() == vsToken::Type_Label )
		{
			m_label = t;
			m_hasLabel = true;
		}
		else if ( t.GetType() == vsToken::Type_OpenBrace )
		{
			m_inBlock = true;
		}
		else if ( t.GetType() == vsToken::Type_CloseBrace )
		{
			return false;	// can't accept a close brace if we're not in a block.
		}
		else if ( t.GetType() == vsToken::Type_NewLine )
		{
			if ( m_hasLabel || !m_token.IsEmpty() )
			{
				if ( m_lineIsOpen )
				{
					m_lineIsOpen = false;
					return true;	// okay, I've swallowed that newline.
				}
				return false;	// couldn't swallow that newline, since I'm already closed.
			}
		}
		else if ( m_lineIsOpen )
		{
			// regular token type;  add it to me, or to my child?

			m_token.AddItem(t);
		}
		else
		{
			return false;	// can't take this new token;  I'm closed!
		}
	}
	else
	{
		// I'm in my block, so add this to the last child record (if it will accept it), or else create a new child record.
		if ( t.GetType() == vsToken::Type_CloseBrace )
		{
			if ( m_childList.IsEmpty() || !m_lastChild->AppendToken(t) )	// if my child won't take it, then it's mine.
			{
				m_inBlock = false;
				m_lineIsOpen = false;
				return true;
			}
		}
		else if ( t.GetType() == vsToken::Type_NewLine )
		{
			if ( !m_childList.IsEmpty() )
			{
				m_lastChild->AppendToken(t);
				return true;
			}
		}
		else if ( m_childList.IsEmpty() || (m_lastChild->AppendToken(t) == false) )
		{
			AddChild( new vsRecord );
			m_lastChild->AppendToken(t);
		}
	}

	return true;
}

void
vsRecord::SetInt(int value)
{
	SetTokenCount(1);
	GetToken(0).SetInteger(value);
}

void
vsRecord::SetFloat(float value)
{
	SetTokenCount(1);
	GetToken(0).SetFloat(value);
}

void
vsRecord::SetRect(float x, float y, float width, float height)
{
	SetTokenCount(4);
	GetToken(0).SetFloat(x);
	GetToken(1).SetFloat(y);
	GetToken(2).SetFloat(width);
	GetToken(3).SetFloat(height);
}

void
vsRecord::SetLabel(const vsString &label)
{
	m_label.SetLabel(label);
}

bool
vsRecord::Bool()
{
	if ( GetToken(0).GetType() == vsToken::Type_Integer )
	{
		return !!(GetToken(0).AsInteger());
	}
	else
	{
		if ( GetToken(0).AsString() == "false" )
			return false;
		else
			return true;
	}
}

int
vsRecord::Int()
{
	return GetToken(0).AsInteger();
}

vsColor
vsRecord::Color()
{
	vsAssert(GetTokenCount() == 4, "Wrong number of tokens to read a color!");

	return vsColor( GetToken(0).AsFloat(),
				   GetToken(1).AsFloat(),
				   GetToken(2).AsFloat(),
				   GetToken(3).AsFloat() );
}

vsString
vsRecord::String()
{
	return GetToken(0).AsString();
}

vsVector2D
vsRecord::Vector2D()
{
	vsAssert(GetTokenCount() == 2, "Wrong number of tokens to read a Vector2D!");

	return vsVector2D( GetToken(0).AsFloat(),
					  GetToken(1).AsFloat() );
}

vsVector3D
vsRecord::Vector3D()
{
	vsAssert(GetTokenCount() == 3, "Wrong number of tokens to read a Vector3D!");

	return vsVector3D( GetToken(0).AsFloat(),
					  GetToken(1).AsFloat(),
					  GetToken(2).AsFloat() );
}

vsVector4D
vsRecord::Vector4D()
{
	vsAssert(GetTokenCount() == 4, "Wrong number of tokens to read a Vector3D!");

	return vsVector4D( GetToken(0).AsFloat(),
					  GetToken(1).AsFloat(),
					  GetToken(2).AsFloat(),
					  GetToken(3).AsFloat() );
}

vsQuaternion
vsRecord::Quaternion()
{
	vsAssert(GetTokenCount() == 4, "Wrong number of tokens to read a Quaternion!");

	return vsQuaternion( GetToken(0).AsFloat(),
					  GetToken(1).AsFloat(),
					  GetToken(2).AsFloat(),
					  GetToken(3).AsFloat() );
}

bool
vsRecord::operator==(const vsString& string)
{
	vsRecord other;
	other.ParseString(string);

	if ( GetTokenCount() != other.GetTokenCount() )
		return false;
	if ( GetLabel() != other.GetLabel() )
		return false;
	for ( int i = 0; i < GetTokenCount(); i++ )
	{
		if ( GetToken(i) != other.GetToken(i) )
			return false;
	}
	return true;
}

