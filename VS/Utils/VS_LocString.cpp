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

vsLocString::vsLocString( const vsString& key ):
	m_key(key.c_str()),
	m_literal(vsEmptyString)
{
}

vsLocString
vsLocString::Literal( const vsString& literal )
{
	vsLocString result;
	result.m_literal = literal;
	return result;
}

vsString
vsLocString::AsString() const
{
	if ( m_key.empty() )
		return m_literal;

	vsString str = vsLoc(m_key);

	if ( !m_args.empty() )
	{
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
}

vsString
vsLocArg::AsString( const vsString& fmt_in ) const
{
	switch ( m_type )
	{
		case Type_LocString:
			return m_locString.AsString();
		case Type_String:
			return m_stringLiteral;
		case Type_Int:
			{
				vsString fmt = vsFormatString("{%s}", fmt_in);
				if ( fmt != vsEmptyString )
				{
					return vsFormatLoc(fmt, m_intLiteral);
				}
			}
		case Type_Float:
			{
				vsString fmt = vsFormatString("{%s}", fmt_in);
				if ( fmt != vsEmptyString )
				{
					return vsFormatLoc(fmt, m_floatLiteral);
				}
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
	return m_key == other.m_key && m_literal == other.m_literal && m_args == other.m_args;
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
		case Type_String:
			result = (m_stringLiteral == other.m_stringLiteral);
			break;
		case Type_Int:
			result = (m_intLiteral == other.m_intLiteral);
			break;
		case Type_Float:
			result = (m_floatLiteral == other.m_floatLiteral);
			break;
	}
	return result;
}

bool
vsLocArg::operator!=(const vsLocArg& other) const
{
	return !(*this==other);
}

