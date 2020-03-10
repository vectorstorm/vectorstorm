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
	m_key(key.c_str())
{
}

vsLocString::operator vsString() const
{
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
			return vsString(m_locString);
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
	s << vsString(ls);
	return s;
}

// vsLocString vsLocFormat(fmt::CStringRef format_str, fmt::ArgList args)
// {
// 	vsLocString result(format_str);
// 	result.m_args = args;
// 	return result;
// }

// void format_arg(fmt::BasicFormatter<char> &f,
// 		const char *&format_str, const vsLocString &s)
// {
// 	f.writer().write(
// 			vsFormatLoc(
// 				vsLoc( s.m_key ),
// 				s.m_args
// 				)
// 			);
// }

