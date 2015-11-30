/*
 *  VS_Log.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Log.h"

#include "VS_System.h"
#include <cstdarg>
#include <cstdio>
#include <cstddef>
#include <physfs.h>

#ifdef MSVC
#define vsprintf vsprintf_s
#endif

static PHYSFS_File* s_log = NULL;

void vsLog_Start()
{
	s_log = PHYSFS_openWrite( "log.txt" );
}

void vsLog_End()
{
	PHYSFS_close(s_log);
}

void vsLog_Show()
{
	const char*writeDir = PHYSFS_getWriteDir();
	if ( writeDir )
	{
		vsSystem::Launch(writeDir);
	}
}

char sz[1024*10];
void vsLog(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	vsnprintf(sz, 1024*10, format, va);
	va_end(va);

	vsString str = sz;

	vsLog(str);
}

void vsLog(const vsString &str)
{
	fprintf(stdout, "%s\n", str.c_str());
	if ( s_log )
	{
		PHYSFS_write( s_log, str.c_str(), 1, str.size() );
#ifdef _WIN32
		PHYSFS_write( s_log, "\r\n", 1, 2 );
#else
		PHYSFS_write( s_log, "\n", 1, 1 );
#endif
	}
}

void vsErrorLog(const char *format, ...)
{
	va_list marker;

	va_start(marker, format);
	vsnprintf(sz, 1024*10, format, marker);
	va_end(marker);

	vsString str = sz;

	vsErrorLog(str);
}

void vsErrorLog(const vsString &str)
{
	fprintf(stderr, "%s\n", str.c_str() );
	if ( s_log )
	{
		PHYSFS_write( s_log, str.c_str(), 1, str.size() );
#ifdef _WIN32
		PHYSFS_write( s_log, "\r\n", 1, 2 );
#else
		PHYSFS_write( s_log, "\n", 1, 1 );
#endif
	}
}

