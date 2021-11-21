/*
 *  VS_Prefix.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 8/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_PREFIX_H
#define VS_PREFIX_H

#include "VS_Config.h"

#if defined __APPLE__
#include <TargetConditionals.h>
#endif


#define BIT(x) (1<<(x))

#ifdef UNUSED
//#elif defined(__GNUC__)
#elif defined(__cplusplus)
#define UNUSED (void)
//#define UNUSED(x) x __attribute__((unused))
//# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#ifdef _WIN32
// WinSock2 must be #included before Windows.h.  So let's just #include it here,
// so I don't have to go searching for all possible include path orders.
#include <winsock2.h>
#undef min
#undef max
#endif

// Define our 'vsTuneable' type, which lets us change tuning values at a breakpoint,
// while still compiling down to 'const' in release builds.
#if defined(_DEBUG)
#define vsTuneable static
#else
#define vsTuneable const
#endif

#define ENABLE_ASSERTS 1

#if defined(__cplusplus)

#include <string>
#include <cstdlib>
#ifdef UNIX
#include <stdint.h>
#include <stddef.h>
#endif
#include <cstdio>
#include <string.h>
#include <exception> // for std::bad_alloc

#include "VS/Utils/VS_Debug.h"
#include "VS/Utils/VS_String.h"
#include "VS/Utils/VS_Log.h"

#define vsDelete(x) { if ( x ) { delete x; x = nullptr; } }
#define vsDeleteArray(x) { if ( x ) { delete [] x; x = nullptr; } }

#ifdef VS_OVERLOAD_ALLOCATORS
void * MyMalloc(size_t size, const char*fileName, int lineNumber, int allocType = 1);	// 1 == Malloc type.  We can ignore this.  :)
void MyFree(void *p, int allocType = 1);

void* operator new(std::size_t n, const char* file, size_t line);
void* operator new[](std::size_t n, const char* file, size_t line);
void operator delete(void* pointer, const char* file, size_t line);
void operator delete[](void* pointer, const char* file, size_t line);

#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW

#define malloc(x) MyMalloc(x, __FILE__, __LINE__)
#define free(p) MyFree(p)

#else // !VS_OVERLOAD_ALLOCATORS

void* operator new(std::size_t n);
void* operator new[](std::size_t n);

#endif // !VS_OVERLOAD_ALLOCATORS

#if defined(_WIN32)

typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else // __GNUC__, __APPLE_CC__, and probably everything else?

#include <stdint.h>

#endif	// normal Linux/BSD/etc section

#include "VS/Math/VS_Math.h"

#endif // __cplusplus

#endif // VS_PREFIX_H

