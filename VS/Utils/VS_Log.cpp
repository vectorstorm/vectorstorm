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
#include <SDL2/SDL_filesystem.h>
#include "VS_File.h"
#include "VS_Task.h"
#include "VS_TimerSystem.h"
#include "VS_Mutex.h"

#ifdef MSVC
#define vsprintf vsprintf_s
#endif

static FILE* s_logFile = nullptr;
static vsString prefPath;
static vsMutex s_mutex;
// static PHYSFS_File* s_log = nullptr;
// static vsFile *s_log = nullptr;

void vsLog_Start(const char* companyName, const char* title)
{
	prefPath = SDL_GetPrefPath(companyName, title);
	vsString logFile = prefPath + "log.txt";
	vsString prevLogFile = prefPath + "log_previous.txt";

	// Now we need to copy this log to a different spot.
	FILE *logRead = fopen(logFile.c_str(), "rb");

	if ( logRead )
	{
		FILE *logWrite = fopen(prevLogFile.c_str(), "wb");
		if ( logWrite )
		{
			fseek( logRead, 0L, SEEK_END );
			long bytes = ftell( logRead );
			fseek( logRead, 0L, SEEK_SET );

			// copy 20kb at a time.
			const long c_bufferSize = 1024 * 24;

			char buffer[c_bufferSize];
			while ( bytes > 0 )
			{
				long bytesToReadThisTime = vsMin( bytes, c_bufferSize );
				size_t bb = fread( buffer, 1, bytesToReadThisTime, logRead );
				vsAssert( bb == (size_t)bytesToReadThisTime, "copying log file failed??" );
				fwrite( buffer, 1, bytesToReadThisTime, logWrite );
				bytes -= bytesToReadThisTime;
			}

			fclose( logWrite );
		}
		fclose( logRead );
	}

	s_logFile = fopen(logFile.c_str(), "w");
}

void vsLog_End()
{
	fclose( s_logFile );
}

void vsLog_Show()
{
	// const char*writeDir = PHYSFS_getWriteDir();
	// if ( writeDir )
	// {
	// 	vsSystem::Launch(writeDir);
	// }
}

void vsLog_(const char* file, int line, const vsString &str)
{
	float time = 0.f;
	if ( vsTimerSystem::Instance() )
		time = vsTimerSystem::Instance()->GetSecondsSinceLaunch();

	for ( const char* ptr = file; *ptr; ++ptr )
		if ( *ptr == '/' || *ptr == '\\' )
		{
			file = ptr+1;
		}

	int threadId = vsTask::GetCurrentThreadId();
	vsString msg( vsFormatString( "%d: %fs - %25s:%4d -- %s\n", threadId, time, file, line, str ) );

	{
		vsScopedLock lock(s_mutex);

		fprintf(stdout, "%s", msg.c_str());
		if ( s_logFile )
		{
			fprintf( s_logFile, "%s", msg.c_str() );
			fflush( s_logFile );
		}
	}
}


void vsErrorLog_(const char* file, int line, const vsString &str)
{
	float time = 0.f;
	if ( vsTimerSystem::Instance() )
		time = vsTimerSystem::Instance()->GetSecondsSinceLaunch();

	for ( const char* ptr = file; *file; ++ptr )
		if ( *ptr == '/' || *ptr == '\\' )
		{
			file = ptr+1;
		}

	int threadId = vsTask::GetCurrentThreadId();
	vsString msg( vsFormatString( "ERR: %d: %fs - %*s:%*d -- %s\n", threadId, time, 25, file, 4,line, str ) );

	{
		vsScopedLock lock(s_mutex);

		fprintf(stderr, "%s\n", str.c_str() );
		if ( s_logFile )
		{
			fprintf( s_logFile, "%s", msg.c_str() );
			fflush( s_logFile );
		}
	}
}

void vsStripLogLine(vsString& line)
{
	const vsString separator(" -- ");
	size_t pos = line.find(separator);
	if ( pos != vsString::npos )
		line.erase(0,pos+separator.size());
}

