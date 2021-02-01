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
#include "VS_File.h"

#ifdef MSVC
#define vsprintf vsprintf_s
#endif

// static PHYSFS_File* s_log = NULL;
static vsFile *s_log = NULL;

void vsLog_Start()
{
	s_log = new vsFile("log.txt", vsFile::MODE_WriteDirectly);
	// s_log = PHYSFS_openWrite( "log.txt" );
}

void vsLog_End()
{
	delete s_log;
	// PHYSFS_close(s_log);
}

void vsLog_Show()
{
	// const char*writeDir = PHYSFS_getWriteDir();
	// if ( writeDir )
	// {
	// 	vsSystem::Launch(writeDir);
	// }
}

void vsLog_(const vsString &str)
{
	fprintf(stdout, "%s\n", str.c_str());
	if ( s_log )
	{
#ifdef _WIN32
		vsString fullLine = str + "\r\n";
#else
		vsString fullLine = str + "\n";
#endif
		s_log->WriteBytes( (const void*)fullLine.c_str(), fullLine.size() );
	}
}


void vsErrorLog_(const vsString &str)
{
	fprintf(stderr, "%s\n", str.c_str() );
	if ( s_log )
	{
#ifdef _WIN32
		vsString fullLine = str + "\r\n";
#else
		vsString fullLine = str + "\n";
#endif
		s_log->WriteBytes( (const void*)fullLine.c_str(), fullLine.size() );
	}
}

