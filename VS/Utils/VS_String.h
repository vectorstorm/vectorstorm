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
#include "Utils/fmt/printf.h"

typedef std::string vsString;

// vsString vsFormatString( const char* format, ... );
//
// For historical reasons, vsFormatString() uses a printf-style format,
// fully supporting POSIX argument position extensions (even on non-POSIX
// platforms, yay!)
#define vsFormatString fmt::sprintf

// vsFormatLoc uses a new-style Python-like format.  This lets you do
// this:
//
// vsString string = vsFormatLocString("{1} {0}!", "World", "Hello");
//
// or for maximum localiser-friendliness, even lets you do this:
//
// vsString string = vsFormatLocString("{hello} {world}!", fmt::arg("world","World"), fmt::arg("hello","Hello"));
// or even this:
// vsString string = vsFormatLocString("{hello} {world}!", "world"_a="World", "hello"_a="Hello");
//
#define vsFormatLoc fmt::format

vsString vsNumberString(int number);

vsString vsTrimWhitespace( const vsString& input );

#define STR vsFormatString

extern const vsString vsEmptyString;


#endif // VS_STRING_H

