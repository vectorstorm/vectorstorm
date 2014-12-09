/*
 *  FS_File.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_File.h"
#include "VS_Record.h"
#include "VS_Store.h"
#include "Core.h"
#include "CORE_Game.h"

#if defined(UNIX)
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#endif // UNIX

#include <SDL2/SDL_filesystem.h>

#include <errno.h>


vsFile::vsFile( const vsString &filename_in, vsFile::Mode mode )
{
	m_mode = mode;

	vsString filename = GetFullFilename( filename_in.c_str() );

	vsString modeStr[vsFile::MODE_MAX] =
	{
		"rb",	// MODE_Read
		"wb"		// MODE_Write
	};

	m_file = fopen(filename.c_str(), modeStr[m_mode].c_str());

	if ( m_file )
	{
		fseek(m_file, 0, SEEK_END);
		m_length = ftell(m_file);
		rewind(m_file);
	}
	else
	{
#if defined(__APPLE_CC__)
		vsString dirName = filename;
		size_t slashIndex = dirName.rfind('/');
		if ( slashIndex != dirName.npos )
		{
			dirName.erase(slashIndex,dirName.npos);
		}

		DIR * dir = opendir(dirName.c_str());
		if ( !dir )
			vsLog("No such directory: %s", dirName.c_str());
		else
		{
			dirent *dent = readdir(dir);

			while( dent )
			{
				vsLog("File: %s", dent->d_name);

				dent = readdir(dir);
			}

			closedir(dir);
		}
#endif // __APPLE_CC__
	}

	int e = errno;

	vsString errStr;

	switch( e )
	{
		case EACCES:
			errStr = "EACCES: Required permissions are denied";
			break;
		case EAGAIN:
			errStr = "EAGAIN: Specified slave side of a locked pseudo-terminal device";
			break;
		case EINTR:
			errStr = "EINTR: Operation interrupted";
			break;
		case EINVAL:
			errStr = "EINVAL: Invalid value";
			break;
		case EIO:
			errStr = "EIO: IO Error";
			break;
		case EISDIR:
			errStr = "EISDIR:  Requested file is a directory";
			break;
#ifndef _WIN32		// no symbolic links in Win32, so this error doesn't exist.
		case ELOOP:
			errStr = "ELOOP:  Too many symbolic links in path";
			break;
#endif // _WIN32
		case EMFILE:
			errStr = "EMFILE:  File is already open";
			break;
		case ENAMETOOLONG:
			errStr = "ENAMETOOLONG:  Filename is too long";
			break;
		case ENFILE:
			errStr = "ENFILE:  System file table is full";
			break;
		case ENOENT:
			errStr = "ENOENT:  A component of the path name that must exist does not exist.";
			break;
		case ENOTDIR:
			errStr = "ENOTDIR:  A component of the path prefix is not a directory.";
			break;
		case EROFS:
			errStr = "EROFS:  Read-only file system.";
			break;
		default:
			errStr = vsFormatString("%d", e);
	}

	vsAssert( m_file != NULL, STR("fopen() returned %s, opening file '%s'", errStr.c_str(), filename.c_str()) );
}

vsFile::~vsFile()
{
	if ( m_file )
		fclose(m_file);
}

bool
vsFile::Exists( const vsString &filename_in )		// static member!
{
	bool exists = false;
	vsString filename = GetFullFilename( filename_in.c_str() );
	FILE *file = fopen(filename.c_str(), "r");
	if ( file )
	{
		fclose(file);
		exists = true;
	}

	return exists;
}

bool
vsFile::Delete( const vsString &filename_in )		// static member!
{
	vsString filename = GetFullFilename( filename_in.c_str() );
	int retval = remove(filename.c_str());

	return (retval == 0);
}

bool
vsFile::PeekRecord( vsRecord *r )
{
	vsAssert(r != NULL, "Called vsFile::Record with a NULL vsRecord!");

	if ( m_mode == MODE_Write )
	{
		vsAssert( m_mode != MODE_Write, "Error:  Not legal to PeekRecord() when writing a file!" );
		return false;
	}
	else
	{
		bool succeeded = false;
		vsString line;

		r->Init();

		long filePos = ftell(m_file);
		if ( r->Parse(this) )
			succeeded = true;

		fseek(m_file, filePos, SEEK_SET);

		return succeeded;
	}

	return false;
}

bool
vsFile::Record( vsRecord *r )
{
	vsAssert(r != NULL, "Called vsFile::Record with a NULL vsRecord!");

	if ( m_mode == MODE_Write )
	{
		vsString recordString = r->ToString();

		fprintf( m_file, "%s", recordString.c_str() );

		return true;
	}
	else
	{
		// we want to read the next line into this vsRecord class, so initialise
		// it before we start.
		r->Init();

		return r->Parse(this);
	}

	return false;
}

bool
vsFile::ReadLine( vsString *line )
{
	const int c_bufSize = 1024;
	char buf[c_bufSize];

	if ( !AtEnd() && fgets(buf, c_bufSize-1, m_file) )
	{
		buf[c_bufSize-1] = 0;	// force a terminator on the end, just in case we receive a line of >= 1024 characters.

		for ( int i = 0; i < c_bufSize; i++ )
		{
			if ( buf[i] == 0 ||
				buf[i] == '\n' ||
				buf[i] == '\r' )
			{
				buf[i] = 0;
				break;
			}
		}

		*line = buf;

		return true;
	}
	return false;
}

bool
vsFile::PeekLine( vsString *line )
{
	const int c_bufSize = 1024;
	char buf[c_bufSize];

	long filePos = ftell(m_file);

	if ( !AtEnd() && fgets(buf, c_bufSize-1, m_file) )
	{
		fseek(m_file, filePos, SEEK_SET);

		buf[c_bufSize-1] = 0;	// force a terminator on the end, just in case we receive a line of >= 1024 characters.

		for ( int i = 0; i < c_bufSize; i++ )
		{
			if ( buf[i] == 0 ||
				buf[i] == '\n' ||
				buf[i] == '\r' )
			{
				buf[i] = 0;
				break;
			}
		}

		*line = buf;

		return true;
	}
	return false;
}

void
vsFile::Rewind()
{
	fseek(m_file, 0, SEEK_SET);
}

void
vsFile::Store( vsStore *s )
{
	if ( m_mode == MODE_Write )
	{
		s->Rewind();
		fwrite(s->GetReadHead(), 1, s->Length(), m_file );
	}
	else
	{
		s->Rewind();
		size_t n = fread(s->GetWriteHead(), 1, s->BufferLength(), m_file );
		s->SetLength(n);
	}
}

void
vsFile::StoreBytes( vsStore *s, size_t bytes )
{
	if ( m_mode == MODE_Write )
	{
		fwrite(s->GetReadHead(), 1, vsMin(bytes, s->BytesLeftForReading()), m_file );
	}
	else
	{
		size_t n = fread(s->GetWriteHead(), 1, vsMin(bytes, s->BytesLeftForWriting()), m_file );
		s->AdvanceWriteHead(n);
	}
}

vsString
vsFile::GetFullFilename(const vsString &filename_in)
{
#if TARGET_OS_IPHONE
	vsString filename = filename_in;

	// find the slash, if any.
	int pos = filename.rfind("/");
	if ( pos != vsString::npos )
	{
		filename.erase(0,pos+1);
	}

	vsString result = vsFormatString("./%s",filename.c_str());
	return result;
#else

	vsString fileName = filename_in;
	vsString dirName;

	bool	fileIsInternal = true;
	// check if filename_in is a preferences file, and if so, then look in the preferences directory, instead of in the usual directory.
	vsString prefsSuffix(".prefs");
	size_t prefsIndex = filename_in.find(prefsSuffix);

	// check if filename_in is a save file, and if so, then look in the save directory, instead of in the usual directory.
	vsString savSuffix(".sav");
	size_t savIndex = filename_in.find(savSuffix);

	if ( prefsIndex != vsString::npos && prefsIndex == filename_in.length() - prefsSuffix.length() )
	{
		fileIsInternal = false;
		dirName = SDL_GetPrefPath("VectorStorm", core::GetGameName().c_str());
	}
	else if ( savIndex != vsString::npos && savIndex == filename_in.length() - savSuffix.length() )
	{
		fileIsInternal = false;
		dirName = SDL_GetPrefPath("VectorStorm", core::GetGameName().c_str());
	}
	else if ( filename_in == "Version.txt" )
	{
		dirName = "Version/";
	}
	else
	{
		dirName = core::GetGameName();
		if ( !dirName.empty() )
			dirName += "/";

	}

	vsString result;

	if ( fileIsInternal )
	{
#if defined(__APPLE_CC__)
		result = vsFormatString("Contents/Resources/Data/%s%s", dirName.c_str(), fileName.c_str());
#else
		result = vsFormatString("Data/%s/%s", dirName.c_str(), fileName.c_str());
#endif
	}
	else
	{
		result = vsFormatString("%s/%s",dirName.c_str(),fileName.c_str());
	}

	return result;
#endif
}


bool
vsFile::AtEnd()
{
	return !m_file || feof( m_file );
}
