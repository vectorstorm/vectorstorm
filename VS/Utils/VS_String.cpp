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

const vsString vsEmptyString = "";

#ifdef MSVC
#define vsprintf vsprintf_s
#define localtime localtime_s
#define gmtime gmtime_s
#endif

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
	// N.B.:  In C++20 this type will need to be changed to const char8_t
	const char *whitespace[] =
	{
		u8"\u0009", // tab
		u8"\u000a", // line feed
		u8"\u000b", // line tab
		u8"\u000c", // form feed
		u8"\u000d", // carriage return
		u8"\u0020", // space
		u8"\u0085", // next line
		u8"\u00a0", // no-break space
		u8"\u1680", // Ogham space mark
		u8"\u2000", // en quad
		u8"\u2001", // em quad
		u8"\u2002", // en space
		u8"\u2003", // em space
		u8"\u2004", // three-per-em space
		u8"\u2005", // four-per-em space
		u8"\u2006", // six-per-em space
		u8"\u2007", // figure space
		u8"\u2008", // punctuation space
		u8"\u2009", // thin space
		u8"\u200a", // hair space
		u8"\u2028", // line separator
		u8"\u2029", // paragraph separator
		u8"\u202f", // narrow no-break space
		u8"\u205f", // medium mathematical space
		u8"\u3000"  // ideographic space
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

	try
	{
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
	catch(...)
	{
		vsLog("Failed to trim whitespace from string %s", input);
	}
	return input;
}

vsString vsFormatString(const vsString& fmt)
{
	return fmt;
}

vsString vsFormatString(const char* fmt)
{
	return fmt;
}

