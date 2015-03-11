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

//vsString vsEmptyString = "";

#ifdef _WIN32
#define vsprintf vsprintf_s
#define localtime localtime_s
#define gmtime gmtime_s
#endif

vsString vsFormatString( const char* format, ... )
{
	char sz[1024];
	va_list marker;

	va_start(marker, format);
	vsprintf(sz, format, marker);
	va_end(marker);

	return sz;
}

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


