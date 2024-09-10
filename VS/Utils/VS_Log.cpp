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
#include <SDL3/SDL_filesystem.h>
#include <sys/stat.h>
#include <filesystem>
#include "VS_File.h"
#include "VS_Thread.h"
#include "VS_TimerSystem.h"
#include "VS_Mutex.h"

#ifdef MSVC
#define vsprintf vsprintf_s
#endif

static std::FILE* s_logFile = nullptr;
static vsMutex s_mutex;
// static PHYSFS_File* s_log = nullptr;
// static vsFile *s_log = nullptr;

void vsLog_Start(const char* companyName, const char* title, const char* profile)
{
	// std::filesystem::path is neat, converting to whatever the native
	// OS wants for its filesystem functions when you call c_str() on it,
	// which means Windows automatically gets its UTF-16 strings that it wants.
	//
	// I like everything about it except for its silly silly / operator
	// for concatonating directory components.

	int sdlInitResult = SDL_Init(0);
	vsAssertF( sdlInitResult == 1, "SDL_Init returned %d: %s", sdlInitResult, SDL_GetError());

	char* prefPathChar = SDL_GetPrefPath(companyName, title);
	vsAssertF(prefPathChar, "vsLogStart: Unable to figure out where to save files: %s", SDL_GetError());
	vsString prefPath(prefPathChar);
	SDL_free(prefPathChar);
	SDL_Quit();
	std::filesystem::path directory( prefPath );
	directory /= profile;
	std::filesystem::path logFile = directory / "log.txt";
	std::filesystem::path prevLogFile = directory / "log_previous.txt";

	if ( !std::filesystem::is_directory(directory) )
		std::filesystem::create_directory(directory);
	if ( std::filesystem::exists( logFile ) )
		std::filesystem::rename( logFile, prevLogFile );

#if defined(_WIN32)
	s_logFile = _wfopen(logFile.c_str(), L"w");
#else
	s_logFile = std::fopen(logFile.c_str(), "w");
#endif
}

void vsLog_End()
{
	if ( s_logFile )
	{
		std::fclose( s_logFile );
		s_logFile = nullptr;
	}
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

	const vsString& threadName = vsThread::GetCurrentThreadName();
	vsString msg( vsFormatString( "%s: %fs - %25s:%4d -- %s\n", threadName, time, file, line, str ) );
	// int threadId = vsThread::GetCurrentThreadId();
	// vsString msg( vsFormatString( "%d: %fs - %25s:%4d -- %s\n", threadId, time, file, line, str ) );

	{
		vsScopedLock lock(s_mutex);

		std::fprintf(stdout, "%s", msg.c_str());
		if ( s_logFile )
		{
			std::fprintf( s_logFile, "%s", msg.c_str() );
			std::fflush( s_logFile );
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

	const vsString& threadName = vsThread::GetCurrentThreadName();
	vsString msg( vsFormatString( "ERR: %s: %fs - %25s:%4d -- %s\n", threadName, time, file, line, str ) );
	// int threadId = vsThread::GetCurrentThreadId();
	// vsString msg( vsFormatString( "ERR: %d: %fs - %*s:%*d -- %s\n", threadId, time, 25, file, 4,line, str ) );

	{
		vsScopedLock lock(s_mutex);

		std::fprintf(stderr, "%s\n", str.c_str() );
		if ( s_logFile )
		{
			std::fprintf( s_logFile, "%s", msg.c_str() );
			std::fflush( s_logFile );
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

