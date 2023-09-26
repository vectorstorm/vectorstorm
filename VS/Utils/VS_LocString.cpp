/*
 *  VS_LocString.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 09/03/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_LocString.h"
#include "VS/Utils/VS_LocalisationTable.h"

vsLocString::vsLocString():
	m_string()
{
}

vsLocString::vsLocString( const char* str ):
	m_string(str)
{
}

vsLocString::vsLocString( const vsString& key ):
	m_string(key)
{
}

vsLocString::vsLocString( int value ):
	m_string("{value}")
{
	m_args.push_back( vsLocArg("value", value) );
}

vsLocString::vsLocString( float value, int places ):
	m_string("{value}")
{
	m_args.push_back( vsLocArg("value", value, places) );
}

// vsLocString
// vsLocString::Literal( const vsString& literal )
// {
// 	vsLocString result;
// 	result.m_literal = literal;
// 	return result;
// }
//
void
vsLocString::SubVars( vsString& str ) const
{
	// Look for {} sets.
	if ( str.empty() )
		return;

	int startVar = -1;
	int startFormat = -1;
	int endVar = -1;
	size_t scan = 0;
	while ( scan < str.size() )
	{
		if ( str[scan] == '{' )
		{
			vsAssertF(startVar == -1, "Nested '{' in locstring??  %s", str);
			startVar = scan;
		}
		if ( str[scan] == ':' )
		{
			if ( startVar != -1 )
				startFormat = scan;
		}
		if ( str[scan] == '}' )
		{
			vsAssertF(startVar != -1, "'}' in locstring without matching '{': %s", str);
			endVar = scan;
		}

		if ( endVar != -1 )
		{
			size_t nameStart = startVar+1;
			size_t nameEnd = (startFormat >= 0) ? startFormat : endVar;
			// process this variable!
			vsString name = str.substr(nameStart, nameEnd-nameStart);
			// check for an arg with the same name
			const vsLocArg* arg = nullptr;
			for ( size_t i = 0; i < m_args.size(); i++ )
				if ( m_args[i].m_name == name )
					arg = &m_args[i];

			vsString replacement;
			if ( arg )
			{
				vsString format;
				if ( startFormat != -1 )
				{
					format = str.substr(startFormat, endVar-startFormat);
					// we have some formatting stuff!
					replacement = arg->AsString(format);
				}
				else
					replacement = arg->AsString();
			}
			else
				replacement = vsLoc(name);

			str = str.replace( startVar, (endVar+1)-startVar, replacement );
			return SubVars(str);
		}
		scan++;
	}
}

bool
vsLocString::IsEmpty() const
{
	return m_string.empty();
}

vsString
vsLocString::AsString() const
{
	vsString str(m_string);
	SubVars( str );
	return str;

#if 0
	if ( !m_args.empty() )
	{
		// Okay.  #if 0 version below worked the other way around; it
		// checked each arg and looked for a variable to substitute itself
		// in for.  Instead, we're going to walk through the string looking
		// for variables, and substitute them if/when we find them.
		//
		// This means that we can also treat unmatched variables as
		// localisation strings




		// let's look for variables to substitute
		for ( size_t i = 0; i < m_args.size(); i++ )
		{
			vsString pattern = vsFormatString("{%s}", m_args[i].m_name);
			size_t pos = str.find(pattern);
			if (pos != vsString::npos)
			{
				// found it!
				vsString valStr = m_args[i].AsString();
				str = str.replace( pos, pattern.size(), valStr );
			}
			else
			{
				// format pattern, woo!
				vsString pattern = vsFormatString("{%s:", m_args[i].m_name);
				size_t pos = str.find(pattern);
				if (pos != vsString::npos)
				{
					size_t formatStart = pos+pattern.size()-1;
					size_t formatEnd = str.find('}', formatStart);
					if ( formatEnd == vsString::npos )
						return str;

					vsString fmt = str.substr(formatStart, formatEnd-formatStart);
					// found it!
					vsString valStr = m_args[i].AsString(fmt);
					str = str.replace( pos, (formatEnd+1)-pos, valStr );
				}
			}
		}
	}
	return str;
#endif // 0
}

static vsString s_thousandsSeparator(",");
vsString s_decimalSeparator(".");

static vsString DoFormatNumber( int value )
{
	// this stupid algorithm doesn't work for negative numbers over 100.
	// So let's switch to positive and append a '-' sign.
	if ( value < 0 )
		return "-" + DoFormatNumber( vsAbs(value) );

	vsString result = vsFormatString("%d",value);
	int insertPosition = result.length() - 3;
	while (insertPosition > 0) {
		result.insert(insertPosition, s_thousandsSeparator);
		insertPosition-=3;
	}
	return result;
}

static vsString DoFormatFloat( float value, int places )
{
	int intPart = vsFloor(value);
	if ( value < 0.f )
		intPart = vsCeil(value);

	value -= intPart;

	int decimalPart = vsAbs( value * pow(10,places+1) );
	// handle rounding

	int factor = (int)pow(10,places);

	if ( decimalPart % factor >= 5 )
		decimalPart += factor;
	decimalPart /= factor;
	if ( decimalPart >= factor ) // rollover!  Increment the integer!
	{
		decimalPart -= factor;
		intPart++;
	}

	vsString result = DoFormatNumber(intPart);
	result = vsFormatString("%s%s%d", result, s_decimalSeparator, decimalPart);
	return result;
}

void
vsLocString::SetNumberThousandsSeparator(const vsString& separator)
{
	s_thousandsSeparator = separator;
}

void
vsLocString::SetNumberDecimalSeparator(const vsString& separator)
{
	s_decimalSeparator = separator;
}

vsString
vsLocArg::AsString( const vsString& fmt_in ) const
{
	switch ( m_type )
	{
		case Type_LocString:
			return m_locString.AsString();
		case Type_Int:
			{
				if ( fmt_in.empty() )
					return DoFormatNumber( m_intLiteral );

				vsString fmt = fmt_in;//vsFormatString("{%s}", fmt_in);
				std::replace( fmt.begin(), fmt.end(), ':', '%');
				if ( fmt != vsEmptyString )
				{
					return tfm::format(fmt.c_str(), m_intLiteral);
				}
				else
					return tfm::format("%d", m_intLiteral);
			}
		case Type_Float:
			{
				if ( m_intLiteral != -1 )
					return DoFormatFloat( m_floatLiteral, m_intLiteral );

				vsString fmt = fmt_in;//vsFormatString("{%s}", fmt_in);
				std::replace( fmt.begin(), fmt.end(), ':', '%');
				if ( fmt != vsEmptyString )
				{
					return tfm::format(fmt.c_str(), m_floatLiteral);
				}
				else
					return tfm::format("%0.1f", m_floatLiteral);
			}
	}
	return vsEmptyString;
}

std::ostream& operator <<(std::ostream &s, const vsLocString &ls)
{
	s << ls.AsString();
	return s;
}

bool
vsLocString::operator==(const vsLocString& other) const
{
	return m_string == other.m_string && m_args == other.m_args;
}

bool
vsLocString::operator!=(const vsLocString& other) const
{
	return !(*this==other);
}

bool
vsLocArg::operator==(const vsLocArg& other) const
{
	if ( m_name != other.m_name )
		return false;
	if ( m_type != other.m_type )
		return false;

	bool result = true;
	switch( m_type )
	{
		case Type_LocString:
			result = (m_locString == other.m_locString);
			break;
		case Type_Int:
			result = (m_intLiteral == other.m_intLiteral);
			break;
		case Type_Float:
			result = (m_floatLiteral == other.m_floatLiteral &&
					m_intLiteral == other.m_intLiteral);
			break;
	}
	return result;
}

bool
vsLocArg::operator!=(const vsLocArg& other) const
{
	return !(*this==other);
}

