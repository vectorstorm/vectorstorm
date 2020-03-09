/*
 *  VS_LocString.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 09/03/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_LocString.h"

	struct Arg
	{
		vsString m_name;

		vsLocString m_valueLocString;
		vsString m_valueLiteral;
		int m_intLiteral;
		float m_floatLiteral;

		enum Type
		{
			Type_LocString,
			Type_String,
			Type_Int,
			Type_Float,
		};
	};

vsLocString::vsLocString( const vsString& key ):
	m_key(key.c_str())
{
}
//
// vsLocString::operator vsString() const
// {
// 	return m_key;
// }
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

