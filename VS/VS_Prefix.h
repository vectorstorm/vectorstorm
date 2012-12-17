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


#if defined __APPLE__
#include "TargetConditionals.h"
#endif


#ifndef NULL
#define NULL (0L)
#endif //

#define BIT(x) (1<<x)

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
#include <WinSock2.h>
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

#define ENABLE_ASSERTS

#if defined(__cplusplus)

#include <string>
#include <stdlib.h>
#ifdef UNIX
#include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>
#include <exception> // for std::bad_alloc
#include "VS/Math/VS_Math.h"
#include "VS/Utils/VS_Debug.h"
#include "VS/Utils/VS_String.h"
#include "VS/Utils/VS_Log.h"

void * MyMalloc(size_t size, const char*fileName, int lineNumber, int allocType = 1);	// 1 == Malloc type.  We can ignore this.  :)
void MyFree(void *p, int allocType = 1);

void* operator new (size_t size, const char *file, int line);
void* operator new[] (size_t size, const char *file, int line);
void operator delete (void *ptr, const char *file, int line) throw();
void operator delete[] (void *ptr, const char *file, int line) throw();
void operator delete (void *ptr) throw();
void operator delete[] (void *ptr) throw();

#define vsDelete(x) { if ( x ) { delete x; x = NULL; } }

#define vsDeleteArray(x) { if ( x ) { delete [] x; x = NULL; } }

#define INGAME_NEW new(__FILE__, __LINE__)
#define new INGAME_NEW

#define malloc(x) MyMalloc(x, __FILE__, __LINE__)
#define free(p) MyFree(p)


#if defined(_WIN32)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;

#else // __GNUC__, __APPLE_CC__, and probably everything else?

#include <stdint.h>

#endif	// normal Linux/BSD/etc section

#endif // __cplusplus

#endif // VS_PREFIX_H

