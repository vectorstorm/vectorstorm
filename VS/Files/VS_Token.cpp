/*
 *  FS_Token.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 19/01/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Token.h"

#include "VS/Memory/VS_Serialiser.h"

#ifdef MSVC
// visual studio defines its own "secure" sscanf.  So use that to keep
// Microsoft happy.
#define sscanf sscanf_s
#endif

// tokens may be any of the following:
//
// sequences of letters (mixed case)
// sequences of numbers (including '-' and '.')
// '{'
// '}'
//
// 'whitespace' is made up of spaces, tabs, or commas.

static bool IsWhitespace( char c )
{
	char whiteSpace[] =
	{
		' ',
		'\t',
		','
	};
	int whiteSpaceTypes = sizeof(whiteSpace);

	for ( int i = 0; i < whiteSpaceTypes; i++ )
	{
		if ( whiteSpace[i] == c )
			return true;
	}
	return false;
}

static bool IsAlpha( char c )
{
	c = toupper(c);
	return ( (c >= 'A' && c <= 'Z') || c == '_');	// we also allow '_' as an 'alphabetic' character, just because it's often used that way, to separate words inside a single label string
}

static bool IsNumeric( char c )
{
	return ( c == '.' || c == '-' || c == '+' ||
			(c >= '0' && c <= '9') );
}

/*static bool IsAlphaNumeric( char c )
{
	return IsAlpha(c) || IsNumeric(c);
}*/


static void RemoveLeadingWhitespace( vsString &string )
{
	bool done = false;

	while(!done)
	{
		done = true;

		if ( !string.empty() && IsWhitespace(string[0]) )
		{
			string.erase(0,1);
			done = false;
		}
	}
}


static vsString ExtractStringToken( vsString &string )
{
	vsAssert(string[0] == '\"', "Tried to extract a string that didn't start with \"!");

	vsString result;

	//result.append( 1, string[0] );	// get first '"'
	string.erase(0,1);

	bool escaped = false;
	while( !string.empty() && string[0] ){
		if ( escaped )
		{
			if ( string[0] == 'n' )
			{
				result.append( 1, '\n' );
			}
			else
			{
				result.append( 1, string[0] );
			}
			escaped = false;
		}
		else
		{
			if ( string[0] == '\"' )
			{
				break; // end of string!
			}
			else if ( string[0] == '\\' )
			{
				escaped = true;
			}
			else
			{
				result.append( 1, string[0] );
			}
		}
		string.erase(0,1);
	}
	//result.append( 1, string[0] );	// get last '"'
	string.erase(0,1);

	return result;
}

static vsString ExtractWhitespaceStringToken( vsString &string )	// pull out a string defined by whitespace
{
	vsString label;

	size_t len = string.length();
	size_t index = 0;

	for ( index = 0; index < len; index++ )
	{
		if ( IsWhitespace(string[index]) )	// this character isn't alphabetic, so isn't part of the label
		{
			index--;					// back up one character
			break;						// exit the loop
		}
	}

	label = string.substr(0, index+1);
	string.erase(0,index+1);

	return label;
}

static vsString ExtractLabelToken( vsString &string )
{
	vsAssert(IsAlpha(string[0]), "Tried to extract a label from something that isn't alpha!");
	// okay.  We need to find
	vsString label;

	size_t len = string.length();
	size_t index = 0;

	for ( index = 0; index < len; index++ )
	{
		if ( !IsAlpha(string[index]) )	// this character isn't alphabetic, so isn't part of the label
		{
			index--;					// back up one character
			break;						// exit the loop
		}
	}

	label = string.substr(0, index+1);
	string.erase(0,index+1);

	return label;
}

static vsString ExtractNumberToken( vsString &string )
{
	vsAssert(IsNumeric(string[0]), "Tried to extract a number from something that isn't a number!");
	// okay.  We need to find
	vsString numberString;

	size_t len = string.length();
	size_t index = 0;

	for ( index = 0; index < len; index++ )
	{
		if ( !IsNumeric(string[index]) )	// this character isn't alphabetic, so isn't part of the label
		{
			index--;					// back up one character
			break;						// exit the loop
		}
	}

	numberString = string.substr(0, index+1);
	string.erase(0,index+1);

	return numberString;
}



vsToken::vsToken():
	m_type(Type_None)
{
}

vsToken::vsToken( vsToken::Type t ):
	m_type(t),
	m_string(NULL)
{
	vsAssert(m_type != vsToken::Type_String, "String type");
	vsAssert(m_type != vsToken::Type_Label, "Label type");
}

vsToken::vsToken(const vsToken& other):
	m_type(Type_None),
	m_string(NULL)
{
	SetType(other.m_type);
	switch ( other.m_type )
	{
		case Type_Label:
		case Type_String:
			m_string = (char*)malloc( strlen(other.m_string)+1 );
			strcpy(m_string, other.m_string);
			break;
		case Type_Float:
			m_float = other.m_float;
			break;
		case Type_Integer:
			m_int = other.m_int;
			break;
		default:
			break;
	}
}

vsToken::~vsToken()
{
	SetInteger(0); // cleanup any string data we had lying around
}

bool
vsToken::ExtractFrom( vsString &string )
{
	SetType( Type_None );

	RemoveLeadingWhitespace(string);

	if ( !string.empty() )
	{
		if ( string[0] == '\"' )
		{
			SetString( ExtractStringToken(string) );
			return true;
		}
		else if ( string[0] == '{' )
		{
			SetType( Type_OpenBrace );
			string.erase(0,1);
			return true;
		}
		else if ( string[0] == '}' )
		{
			SetType( Type_CloseBrace );
			string.erase(0,1);
			return true;
		}
		else if ( string[0] == ';' )
		{
			SetType( Type_Semicolon );
			string.erase(0,1);
			return true;
		}
		else if ( string[0] == '\n' )
		{
			SetType( Type_NewLine );
			string.erase(0,1);
			return true;
		}
		else if ( ::IsAlpha(string[0]) )
		{
			SetLabel( ExtractLabelToken(string) );
			return true;
		}
		else if ( ::IsNumeric( string[0]) )
		{
			vsString token = ExtractNumberToken(string);

			if ( token == "-" ) // Ugly:  handle negative nans as zeroes.
			{
				SetInteger(0);
				return false;
			}

			bool isAFloat = ( token.find('.') != token.npos );
			if ( isAFloat )
			{
				float val = 0.f;
				bool success = (sscanf( token.c_str(), "%f", &val )!=0);
				vsAssert(success, "Couldn't find a floating value where we expected one?");
				SetFloat(val);
				return true;
			}
			else
			{
				int val = 0;
				bool success = sscanf( token.c_str(), "%d", &val) != 0;
				vsAssert(success, "Couldn't find an integer value?" );
				SetInteger(val);
				return true;
			}
		}
		else if ( string[0] == '#' )
		{
			// comment!
			string = vsEmptyString;
		}
		else if ( string[0] == '=' )
		{
			SetType( Type_Equals );
			string.erase(0,1);
			return true;
		}
		else
		{
			// no clue what it was!  Just treat it as a string, breaking at the next whitespace

			SetString( ExtractWhitespaceStringToken(string) );
			return true;
		}
	}

	return false;
}

vsString
vsToken::BackToString()
{
	if ( m_type == Type_String )
	{
		// check for newlines and other special characters;  we need to escape them!
		vsString escapedString = m_string;
		while ( escapedString.find('\n') != vsString::npos )
		{
			size_t pos = escapedString.find('\n');
			escapedString.erase(pos,1);
			escapedString.insert(pos, "\\n");
		}
		while ( escapedString.find('\t') != vsString::npos )
		{
			size_t pos = escapedString.find('\t');
			escapedString.erase(pos,1);
			escapedString.insert(pos, "\\t");
		}
		while ( escapedString.find('\"') != vsString::npos )
		{
			size_t pos = escapedString.find('\"');
			escapedString.erase(pos,1);
			escapedString.insert(pos, "\\\"");
		}
		return "\"" + escapedString + "\"";
	}

	return AsString();
}

vsString
vsToken::AsString() const
{
	vsString result = vsEmptyString;

	switch( m_type )
	{
		case Type_Label:
		case Type_String:
			result = m_string;
			break;
		case Type_NewLine:
			result = "\n";
			break;
		case Type_Float:
			result = vsFormatString("%f", m_float);
			break;
		case Type_Integer:
			result = vsFormatString("%d", m_int);
			break;
		case Type_Equals:
			result = "=";
			break;
		case Type_Semicolon:
			result = ";";
		default:
			break;
	}

	return result;
}

void
vsToken::PopulateStringTable( vsStringTable& table )
{
	if ( m_type == Type_Label || m_type == Type_String )
	{
		table.AddString(m_string);
	}
}

void
vsToken::SerialiseBinaryV1( vsSerialiser *s, vsStringTable& stringTable )
{
	SetType(Type_Integer);

	uint8_t type = m_type;
	s->Uint8(type);
	m_type = (Type)type;
	switch( m_type )
	{
		case Type_Label:
		case Type_String:
			{
				if ( s->GetType() == vsSerialiser::Type_Write )
				{
					// s->String(m_string);
					uint32_t i = stringTable.FindString(m_string);
					s->Uint32(i);
				}
				else
				{
					uint32_t i = 0;
					s->Uint32(i);
					if ( m_type == Type_Label )
						SetLabel( stringTable.GetStrings()[i] );
					else
						SetString( stringTable.GetStrings()[i] );
				}
			}
			break;
		case Type_Float:
			s->Float(m_float);
			break;
		case Type_Integer:
			s->Int32(m_int);
			break;
		default:
			break;
	}
}

void
vsToken::SerialiseBinaryV2( vsSerialiser *s )
{
	uint8_t type = m_type;
	s->Uint8(type);
	if ( s->GetType() == vsSerialiser::Type_Read )
		SetType((Type)type);

	switch( m_type )
	{
		case Type_Label:
		case Type_String:
			{
				vsString string;
				if ( s->GetType() == vsSerialiser::Type_Write )
					string = m_string;
				s->String(string);
				if ( s->GetType() == vsSerialiser::Type_Read )
				{
					m_string = NULL; // set null to avoid bad deallocation,
					// since we'd set our 'type', above.
					if ( m_type == Type_Label )
						SetLabel(string);
					else
						SetString(string);
				}
				break;
			}
		case Type_Float:
			s->Float(m_float);
			break;
		case Type_Integer:
			s->Int32(m_int);
			break;
		default:
			break;
	}
}
int
vsToken::AsInteger() const
{
	vsAssert(m_type == Type_Integer, "Tried to read non-integer token as integer!");

	return m_int;
}

float
vsToken::AsFloat() const
{
	vsAssert(m_type == Type_Float || m_type == Type_Integer, "Tried to read non-numeric token as float!");

	if ( m_type == Type_Float )
		return m_float;
	else
		return (float)m_int;
}

void
vsToken::SetStringField( const vsString& s )
{
	m_string = (char*)malloc( s.size()+1 );
	strcpy(m_string, s.c_str());
}

void
vsToken::SetString(const vsString &value)
{
	SetType( Type_String );
	SetStringField(value);
}

void
vsToken::SetLabel(const vsString &value)
{
	SetType( Type_Label );
	SetStringField(value);
}

void
vsToken::SetInteger(int value)
{
	SetType( Type_Integer );
	m_int = value;
}

void
vsToken::SetFloat(float value)
{
	SetType( Type_Float );
	m_float = value;
}

void
vsToken::SetType(Type t)
{
	if ( m_type == Type_String || m_type == Type_Label )
	{
		// if we're currently a string time, clear our string.
		if ( m_string != NULL )
		{
			free( m_string );
			m_string = NULL;
		}
	}
	m_type = t;
}

bool
vsToken::IsType( vsToken::Type t )
{
	return m_type == t;
}

bool
vsToken::operator==( const vsToken& other )
{
	return AsString() == other.AsString();
}

bool
vsToken::operator==( const vsString& str )
{
	return AsString() == str;
}

vsToken&
vsToken::operator=( const vsToken& other )
{
	SetType( other.m_type );
	m_string = NULL;
	switch ( other.m_type )
	{
		case Type_Label:
			SetLabel(other.m_string);
			break;
		case Type_String:
			SetString(other.m_string);
			break;
		case Type_Float:
			m_float = other.m_float;
			break;
		case Type_Integer:
			m_int = other.m_int;
			break;
		default:
			break;
	}
	return *this;
}

