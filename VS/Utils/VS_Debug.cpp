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
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#define __thread 
#endif
#if defined(__APPLE_CC__)
#include <execinfo.h>
#endif

__thread std::vector<const char *> _error_context_name;
__thread std::vector<const char *> _error_context_data;

vsErrorContext::vsErrorContext(const char *name, const char *data)
{
	_error_context_name.push_back(name);
	_error_context_data.push_back(data);
}

vsErrorContext::~vsErrorContext()
{
	_error_context_name.pop_back();
	_error_context_data.pop_back();
}

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
		std::vector<const char *>::iterator it;
		for ( size_t i = 0; i < _error_context_name.size(); i++ )
		{
			vsLog("When %s: %s", _error_context_name[i], _error_context_data[i]);
		}

		vsLog("Failed assertion:  %s", msg.c_str());
		vsLog("Failed condition: (%s)", conditionStr.c_str());
		vsLog("at %s:%d", trimmedFile.c_str(), line);

		{
#if defined(_WIN32)
			vsString mbString = vsFormatString("Failed assertion:  %s\nFailed condition: (%s)\nat %s:%d", msg.c_str(), conditionStr.c_str(), trimmedFile.c_str(), line);
			MessageBoxA(NULL, mbString.c_str(), NULL, MB_OK);
#endif
#if defined(__APPLE_CC__)
			void* callstack[128];
			int i, frames = backtrace(callstack, 128);
			char** strs = backtrace_symbols(callstack, frames);
			for (i = 0; i < frames; ++i) {
				vsLog("%s\n", strs[i]);
			}
			free(strs);
#endif

		// Now we intentionally try to write to NULL, to trigger debuggers to stop,
		// so we can perhaps try to debug whatever went wrong.
		//
		// If there's no debugger running, then this will probably yield a segfault.
		// If we're running on something primitive enough that it doesn't even segfault,
		// then just explicitly exit.
			char *ptr = NULL;
			*ptr = 0;

			exit(1);
		}
	}
	else
	{
		vsLog("Error:  Asserted while handling assertion!");
	}
}

//#endif

