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
#elif defined(__APPLE_CC__)
	#include <execinfo.h>
	#include <signal.h>
	#define DEBUG_BREAK raise(SIGTRAP)
#else
	#include <signal.h>
	#define DEBUG_BREAK raise(SIGTRAP)
#endif

//#if defined(_DEBUG)

static bool bAsserted = false;

void vsFailedAssert( const vsString &conditionStr, const vsString &msg, const char *file, int line )
{
	if ( !bAsserted )
	{
		// failed assertion..  render and go into an infinite loop.
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

		{

#if defined(_DEBUG)
			DEBUG_BREAK;
#else
			vsString mbString = vsFormatString("Failed assertion:  %s\nFailed condition: (%s)\nat %s:%d", msg.c_str(), conditionStr.c_str(), trimmedFile.c_str(), line);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed assertion", mbString.c_str(), NULL);
// #if defined(__APPLE_CC__)
// 			void* callstack[128];
// 			int i, frames = backtrace(callstack, 128);
// 			char** strs = backtrace_symbols(callstack, frames);
// 			for (i = 0; i < frames; ++i) {
// 				vsLog("%s", strs[i]);
// 			}
// 			free(strs);
// #endif
			vsLog_End();
			vsLog_Show();
#endif
			// raise a segfault, to force exception handling to kick in.
			*(char*)(NULL) = 1;
		}
	}
	else
	{
		vsLog("Error:  Asserted while handling assertion!");
	}
}

//#endif

