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

#include "VS_String.h"
#include "VS_Log.h"

#if ENABLE_ASSERTS

// A "Check" will output a warning message to the log if the condition fails.
#define vsCheck(x,y) {if(!(x)){ vsFailedCheck(#x, y,__FILE__, __LINE__); }}
#define vsCheckF(x,y,...) {if(!(x)){ vsFailedCheckF(#x, __FILE__, __LINE__, y, __VA_ARGS__); }}
void vsFailedCheck(const char* conditionStr, const char* msg, const char *file, int line);


template <typename S, typename... Args, typename Char = fmt::char_t<S> >
void vsFailedCheckF(const char* conditionStr, const char* file, int line, S format, Args&&... args)
{
	vsString msgFormatted = fmt::sprintf(format,args...);
	vsFailedCheck( conditionStr, msgFormatted.c_str(), file, line );
}


// An "Assert" will stop the program immediately, and output a message to the log
// if the condition fails.
//
#define vsAssert(x,y) {if(!(x)){ vsFailedAssert(#x, y,__FILE__, __LINE__); }}
#define vsAssertF(x,y,...) {if(!(x)) { vsFailedAssertF(#x, __FILE__, __LINE__, y, __VA_ARGS__); }}
void vsFailedAssert( const char* conditionStr, const char* msg, const char *file, int line );
inline void vsFailedAssert( const char* conditionStr, const vsString &msg, const char *file, int line )
{
	return vsFailedAssert( conditionStr, msg.c_str(), file, line );
}

template <typename S, typename... Args, typename Char = fmt::char_t<S> >
void vsFailedAssertF(const char* conditionStr, const char* file, int line, S msg, Args&&... args)
{
	vsString msgFormatted = fmt::sprintf(msg,args...);
	vsFailedAssert( conditionStr, msgFormatted.c_str(), file, line );
}

#else
#define vsAssert(x,y) {}
#define vsAssertF(x,y,...) {}
#endif


#endif // VS_DEBUG_H

