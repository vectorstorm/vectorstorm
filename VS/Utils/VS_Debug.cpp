/*
 *  VS_Debug.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Debug.h"
#include "VS_DisplayList.h"
#include "VS_Font.h"
#include "VS_Scene.h"
#include "VS_Screen.h"
#include "VS_Sprite.h"
#include "VS_System.h"

#include <assert.h>

#include <SDL2/SDL_messagebox.h>

#if defined(_WIN32)
	#include <windows.h>
	#define __thread
	#define DEBUG_BREAK __debugbreak()
	// no elegant way to crash on Windows, so let's just
	// write into an illegal address space to force the
	// kernel to kill us.
	//
	// Interesting note:  writing into illegal memory like
	// this officially triggers "undefined behaviour".  It
	// won't necessarily crash.  In fact, recent versions
	// of clang have chosen to simply ignore writes to NULL;
	// behaviour is undefined, so they're well within their
	// rights to not crash when you do that.  But here on
	// Windows, our nefarious deeds will actually (at time
	// of writing) crash our program successfully.
	#define CRASH *((volatile int*)NULL) = 0xDEADBEEF

#elif defined(__APPLE_CC__)
	#include <execinfo.h>
	#include <signal.h>
	#define DEBUG_BREAK __builtin_trap()
	// we don't need to do something illegal to crash;  we
	// can simply send a "we've crashed" signal ourselves.
	// So much simpler, this way!
	#define CRASH raise(SIGSEGV)
#else
	#include <signal.h>
	#define DEBUG_BREAK __builtin_trap()
	#define CRASH raise(SIGSEGV)
#endif

static bool bAsserted = false;

void vsFailedAssert( const vsString &conditionStr, const vsString &msg, const char *file, int line )
{
	if ( !bAsserted )
	{
		// failed assertion..  trace out some information and then crash.
		bAsserted = true;
		vsString trimmedFile(file);
		size_t pos = trimmedFile.rfind('/');
		if ( pos )
		{
			trimmedFile = trimmedFile.substr(pos+1);
		}

		vsLog("Failed assertion:  %s", msg.c_str());
		vsLog("Failed condition: (%s)", conditionStr.c_str());
		vsLog("at %s:%d", trimmedFile.c_str(), line);
		vsLog_End();

		{
#if defined(_DEBUG)
			DEBUG_BREAK;
#else
			vsString mbString = vsFormatString("Failed assertion:  %s\nFailed condition: (%s)\nat %s:%d", msg.c_str(), conditionStr.c_str(), trimmedFile.c_str(), line);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed assertion", mbString.c_str(), NULL);
#endif
			CRASH;
		}
	}
	else
	{
		vsLog("Error:  Asserted while handling assertion!");
	}
}

