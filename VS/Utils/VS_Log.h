/*
 *  VS_Log.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_LOG
#define VS_LOG

// vsLog_Start() creates a file named "log.txt" in our current output directory.
// All subsequent calls to vsLog() will write out text into that file, in addition
// to the console.  vsLog_End() closes that file, and stops writing vsLog() messages
// into it.  vsLog_Show() opens a finder/explorer/etc window in that directory.  We
// use this when an assert is thrown, so that end-users can more easily find the
// log file so they can e-mail it to us.  (TODO:  Consider whether we want to set
// up a system which will cause asserts to submit logs to us anonymously, instead?)
void vsLog_Start();
void vsLog_End();
void vsLog_Show();

#include "Utils/fmt/printf.h"
#include "Utils/fmt/format.h"

void vsLog_(const vsString& str);
void vsErrorLog_(const vsString &str);

template <typename S, typename... Args, typename Char = fmt::char_t<S> >
void vsLog(S format, Args&&... args)
{
	vsString str = fmt::sprintf(format,args...);
	vsLog_(str);
}

template <typename S, typename... Args, typename Char = fmt::char_t<S> >
void vsErrorLog(S format, Args&&... args)
{
	vsString str = fmt::sprintf(format,args...);
	vsErrorLog_(str);
}



// utility macro to trace out this message ONLY ONCE.
#define vsLogOnce(...)                \
{                                      \
	static bool ss_loggedOnce = false; \
	if ( !ss_loggedOnce )              \
	{                                  \
		vsLog(__VA_ARGS__);            \
		ss_loggedOnce = true;          \
	}                                  \
}

#endif // VS_LOG

