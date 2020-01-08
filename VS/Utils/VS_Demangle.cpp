/*
 *  VS_Demangle.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 09/01/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Demangle.h"
#ifdef GCC
#include <cxxabi.h>
#endif

vsString Demangle( const vsString &input )
{
#ifdef GCC
	char buf[1024];
    size_t size=1024;
    int status;
    char* res = abi::__cxa_demangle (input.c_str(),
									 buf,
									 &size,
									 &status);

	return vsString(res);
#else // really should just be visual studio, here.
	vsString result = input;
	if ( result.find("class ") == 0 )
	{
		result.erase(0, 6);
	}
	return result;
#endif
}

