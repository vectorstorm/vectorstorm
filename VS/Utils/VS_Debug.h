/*
 *  VS_Debug.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_DEBUG_H
#define VS_DEBUG_H


#if defined(ENABLE_ASSERTS)
#include "VS_String.h"
#include "VS_Log.h"

#define vsAssert(x,y) if(!(x)){ vsFailedAssert(#x, y,__FILE__, __LINE__); }

#define vsAssertF(x,y,...) if(!(x)) { vsFailedAssertF(#x, __FILE__, __LINE__, y, __VA_ARGS__); }

void vsFailedAssert( const char* conditionStr, const char* msg, const char *file, int line );
inline void vsFailedAssert( const char* conditionStr, const vsString &msg, const char *file, int line )
{
	return vsFailedAssert( conditionStr, msg.c_str(), file, line );
}

void vsFailedAssertF(const char* conditionStr, const char* file, int line, fmt::CStringRef msg, fmt::ArgList args);
FMT_VARIADIC(void, vsFailedAssertF, const char*, const char*, int, fmt::CStringRef )

#else
#define vsAssert(x,y) {}
#endif


#endif // VS_DEBUG_H

