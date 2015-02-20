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
#include <physfs.h>

#include "VS_DisableDebugNew.h"
#include <vector>
#include <algorithm>
#include "VS_EnableDebugNew.h"

vsFile::vsFile( const vsString &filename, vsFile::Mode mode )
{
	m_mode = mode;

	if ( mode == MODE_Read )
		m_file = PHYSFS_openRead( filename.c_str() );
	else
		m_file = PHYSFS_openWrite( filename.c_str() );

	if ( m_file )
	{
		m_length = PHYSFS_fileLength(m_file);
	}

	vsAssert( m_file != NULL, STR("Error opening file '%s':  %s", filename.c_str(), PHYSFS_getLastError()) );
}

vsFile::~vsFile()
{
	if ( m_file )
		PHYSFS_close(m_file);
}

bool
vsFile::Exists( const vsString &filename ) // static method
{
	return PHYSFS_exists(filename.c_str()) && !PHYSFS_isDirectory(filename.c_str());
}

bool
vsFile::DirectoryExists( const vsString &filename ) // static method
{
	return PHYSFS_exists(filename.c_str()) && PHYSFS_isDirectory(filename.c_str());
}

bool
vsFile::Delete( const vsString &filename ) // static method
{
	return PHYSFS_delete(filename.c_str()) != 0;
}

class sortFilesByModificationDate
{
	vsString m_dirName;
	public:
	sortFilesByModificationDate( const vsString& dirName ):
		m_dirName(dirName)
	{
	}

	bool operator()(char* a,char* b)
	{
		int atime = PHYSFS_getLastModTime((m_dirName + a).c_str());
		int btime = PHYSFS_getLastModTime((m_dirName + b).c_str());
		return ( atime > btime );
	}
};

vsArray<vsString>
vsFile::DirectoryContents( const vsString &dirName ) // static method
{
	char **files = PHYSFS_enumerateFiles(dirName.c_str());
	char **i;
	std::vector<char*> s;
	for (i = files; *i != NULL; i++)
		s.push_back(*i);


	std::sort(s.begin(), s.end(), sortFilesByModificationDate(dirName));

	vsArray<vsString> result;

	for (size_t i = 0; i < s.size(); i++)
		result.AddItem( s[i] );

	PHYSFS_freeList(files);

	return result;
}

void
vsFile::EnsureWriteDirectoryExists( const vsString &writeDirectoryName ) // static method
{
	if ( !DirectoryExists(writeDirectoryName) )
	{
		int mkdirResult = PHYSFS_mkdir( writeDirectoryName.c_str() );
		vsAssert( mkdirResult != 0, vsFormatString("Failed to create directory '%s%s%s': %s",
				PHYSFS_getWriteDir(), PHYSFS_getDirSeparator(), writeDirectoryName.c_str(), PHYSFS_getLastError()) );
	}
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

		long filePos = PHYSFS_tell(m_file);
		if ( r->Parse(this) )
			succeeded = true;

		PHYSFS_seek(m_file, filePos);

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

		PHYSFS_write( m_file, recordString.c_str(), 1, recordString.size() );

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
	// const int c_bufSize = 1024;
	// char buf[c_bufSize];

	long filePos = PHYSFS_tell(m_file);
	char peekChar = 'a';

	while ( !AtEnd() && peekChar != '\n' && peekChar != 0 )
	{
		PHYSFS_read(m_file, &peekChar, 1, 1);
	}
	long afterFilePos = PHYSFS_tell(m_file);
	long bytes = afterFilePos - filePos;
	PHYSFS_seek(m_file, filePos);
	if ( bytes > 0 )
	{
		char* buffer = (char*)malloc(bytes+1);
		PHYSFS_read(m_file, buffer, 1, bytes);
		buffer[bytes-1] = 0;

		*line = buffer;
		while (line->find('\r') != vsString::npos)
		{
			line->erase( line->find('\r') );
		}
		free(buffer);

		return true;
	}
	return false;
}

bool
vsFile::PeekLine( vsString *line )
{
	long filePos = PHYSFS_tell(m_file);
	bool result = ReadLine(line);
	PHYSFS_seek(m_file, filePos);
	return result;
}

void
vsFile::Rewind()
{
	PHYSFS_seek(m_file, 0);
}

void
vsFile::Store( vsStore *s )
{
	if ( m_mode == MODE_Write )
	{
		s->Rewind();
		PHYSFS_write( m_file, s->GetReadHead(), 1, s->Length() );
	}
	else
	{
		s->Rewind();
		size_t n = PHYSFS_read( m_file, s->GetWriteHead(), 1, s->BufferLength() );
		s->SetLength(n);
	}
}

void
vsFile::StoreBytes( vsStore *s, size_t bytes )
{
	if ( m_mode == MODE_Write )
	{
		PHYSFS_write( m_file, s->GetReadHead(), 1, vsMin(bytes, s->BytesLeftForReading()) );
	}
	else
	{
		size_t n = PHYSFS_read( m_file, s->GetWriteHead(), 1, vsMin(bytes, s->BytesLeftForWriting()) );
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

	vsString dir = PHYSFS_getRealDir( filename_in.c_str() );
	return dir + "/" + filename_in;
#endif
}


bool
vsFile::AtEnd()
{
	return !m_file || PHYSFS_eof( m_file );
}

