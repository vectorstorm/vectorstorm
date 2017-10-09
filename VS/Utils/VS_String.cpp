/*
 *  VS_String.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_String.h"

//#include <string>
#include <stdarg.h>
#include "Utils/utfcpp/utf8.h"
#include "Utils/fmt/printf.h"

const vsString vsEmptyString = "";

#ifdef MSVC
#define vsprintf vsprintf_s
#define localtime localtime_s
#define gmtime gmtime_s
#endif

vsString test()
{
	int thing = 42;
	return fmt::sprintf("Foo %d", thing++);
	return fmt::sprintf("Foo %d", thing);
}

// vsString vsFormatString( const char* format, ... )
// {
// 	char sz[1024];
// 	va_list marker;
//
// 	va_start(marker, format);
// 	vsnprintf(sz, 1024, format, marker);
// 	va_end(marker);
//
// 	return sz;
// }

vsString vsNumberString(int number)
{
	vsString result;
	bool negative = false;

	if ( number < 0 )
	{
		negative = true;
		number *= -1;
	}

	int millions = number / 1000000;
	number -= millions * 1000000;
	int thousands = number / 1000;
	number -= thousands * 1000;
	int ones = number;

	if ( millions != 0 )
	{
		result = vsFormatString("%d,%03d,%03d", millions, thousands, ones);
	}
	else if ( thousands != 0 )
	{
		result = vsFormatString("%d,%03d", thousands, ones);
	}
	else
	{
		result = vsFormatString("%d", ones);
	}

	if ( negative )
	{
		result = vsFormatString("-%s",result.c_str());
	}

	return result;
}

static bool vsIsWhitespace( const utf8::iterator<std::string::iterator>& it )
{
	const char *whitespace[] =
	{
		"\x09", // tab
		"\x0a", // line feed
		"\x0d", // carriage return
		"\x20", // space
		"\xc2\xa0", // no-break space
		"\xe1\x9a\x80", // Ogham space mark
		"\xe2\x80\x80", // en quad
		"\xe2\x80\x81", // em quad
		"\xe2\x80\x82", // en space
		"\xe2\x80\x83", // em space
		"\xe2\x80\x84", // three-per-em space
		"\xe2\x80\x85", // four-per-em space
		"\xe2\x80\x87", // figure space
		"\xe2\x80\x88", // punctuation space
		"\xe2\x80\x89", // thin space
		"\xe2\x80\x8a", // hair space
		"\xe2\x80\x8b", // zero width space
		"\xe2\x80\xaf", // narrow no-break space
		"\xe2\x81\x9f", // medium mathematical space
		"\xe3\x80\x80"  // ideographic space
	};
	const int whitespaceEntries = sizeof(whitespace) / sizeof(char*);
	for ( int wi = 0; wi < whitespaceEntries; wi++ )
	{
		utf8::iterator<const char*> wit( whitespace[wi], whitespace[wi], whitespace[wi] + strlen(whitespace[wi]) );
		if ( *it == *wit )
			return true;
	}
	return false;
}

vsString vsTrimWhitespace( const vsString& input )
{
	vsString oldString = input;

	int length = utf8::distance(oldString.begin(), oldString.end());
	int nonWhitespaceEnd = 0;
	int nonWhitespaceStart = length;

	// first, figure out where our content begins and ends
	{
		utf8::iterator<std::string::iterator> it( oldString.begin(), oldString.begin(), oldString.end() );

		for ( int i = 0; i < length; i++ )
		{
			if ( !vsIsWhitespace( it ) )
			{
				nonWhitespaceStart = vsMin(nonWhitespaceStart, i);
				nonWhitespaceEnd = vsMax(nonWhitespaceEnd, i+1);
			}
			it++;
		}
	}

	// construct a trimmed string, with only the content; not the
	// leading/trailing whitespace.
	vsString result;
	{
		utf8::iterator<std::string::iterator> it( oldString.begin(), oldString.begin(), oldString.end() );
		for ( int i = 0; i < nonWhitespaceEnd; i++ )
		{
			if ( i >= nonWhitespaceStart )
				utf8::append( *it, back_inserter(result) );
			it++;
		}
	}
	return result;
}

