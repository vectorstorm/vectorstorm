/*
 *  VS_String.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_STRING_H
#define VS_STRING_H
#include "Utils/tinyformat.h"

typedef std::string vsString;


vsString vsNumberString(int number);

vsString vsTrimWhitespace( const vsString& input );

#define STR vsFormatString

extern const vsString vsEmptyString;


vsString vsFormatString(const vsString& fmt);
vsString vsFormatString(const char* fmt);

template<typename... Args>
vsString vsFormatString(const vsString& fmt, Args&& ...args)
{
	return tfm::format(fmt.c_str(), args...);
}

template<typename... Args>
vsString vsFormatString(const char* fmt, Args&& ...args)
{
	return tfm::format(fmt, args...);
}

#endif // VS_STRING_H

