/*
 *  FS_File.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_File.h"
#include "VS_FileCache.h"
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
#include <zlib.h>

#include "VS_DisableDebugNew.h"
#include <vector>
#include <algorithm>
#include "VS_EnableDebugNew.h"

// #define PROFILE_FILE_SYSTEM
#ifdef PROFILE_FILE_SYSTEM

#include "VS_TimerSystem.h"


namespace {

	class vsFileProfiler
	{
		vsString m_name;
		unsigned long m_start;
		bool m_cached;
	public:
		vsFileProfiler(const vsString& name, bool cached):
			m_name(name),
			m_start( vsTimerSystem::Instance()->GetMicroseconds() ),
			m_cached(cached)
		{
		}

		~vsFileProfiler()
		{
			unsigned long now = vsTimerSystem::Instance()->GetMicroseconds();
			if ( m_cached )
				vsLog("FileProfiler [CACHED] '%s':  %f milliseconds", m_name.c_str(), (now - m_start) / 1000.f);
			else
				vsLog("FileProfiler '%s':  %f milliseconds", m_name.c_str(), (now - m_start) / 1000.f);
		}
	};
};

#define PROFILE(x) vsFileProfiler fp(x,false)
#define PROFILE_CACHED(x) vsFileProfiler fp(x,true)

#else

#define PROFILE(x)
#define PROFILE_CACHED(x)

#endif


// PhysFS made a bunch of changes to its interface in version 2.1.0.  Let's
// check what version of PhysFS we're compiling against and create the necessary
// glue code to make everything work.
//
// Our goal here is for the code in this file to exclusively be using the
// MODERN interfaces (i.e.: v2.1+), but to provide glue code so that those
// running older versions of the library (i.e.: me, in all my Steam builds) can
// still compile and run.

#if PHYSFS_VER_MAJOR < 2 || (PHYSFS_VER_MAJOR == 2 && PHYSFS_VER_MINOR < 1)

	// We're in a PhysFS version before 2.1.0.  This means that
	// PHYSFS_writeBytes() and PHYSFS_readBytes() don't exist yet, so let's
	// make them!

#define PHYSFS_writeBytes(file, bytes, count) PHYSFS_write(file, bytes, 1, count)
#define PHYSFS_readBytes(file, bytes, count) PHYSFS_read(file, bytes, 1, count)

	// Additionally, in PhysFS 2.1.0 a whole bunch of file query functions got
	// removed and replaced by a single "Stat" function that fetches a whole
	// lot of file status data in a single go.  Stat doesn't exist yet in
	// versions before 2.1.0, so let's make one!

	// in 2.1.0 there are more filetypes than these, but for now this is all I
	// need for compatibility purposes.
	enum PHYSFS_FileType
	{
		PHYSFS_FILETYPE_REGULAR,
		PHYSFS_FILETYPE_DIRECTORY
	};
	struct PHYSFS_Stat
	{
		PHYSFS_sint64 filesize;
		PHYSFS_sint64 modtime;
		// PHYSFS_sint64 createtime; // no way to get these values in <2.1.0
		// PHYSFS_sint64 accesstime;
		PHYSFS_FileType filetype;
		// int readonly;
	};

	int PHYSFS_stat( const char* filename, PHYSFS_Stat* stat )
	{
		if ( !PHYSFS_exists(filename) )
			return 0;
		if ( PHYSFS_isDirectory(filename) )
		{
			stat->filetype = PHYSFS_FILETYPE_DIRECTORY;
		}
		else
		{
			PHYSFS_File *file = PHYSFS_openRead( filename );
			if ( file != NULL )
			{
				stat->filetype = PHYSFS_FILETYPE_REGULAR;
				stat->filesize = PHYSFS_fileLength(file);
				PHYSFS_close(file);
			}
			else
			{
				vsAssert(false, vsFormatString("No clue what file type this is: %s", filename));
				return 0;
			}
		}
		stat->modtime = PHYSFS_getLastModTime(filename);
		return 1;
	}

	// Okay.  This is ugly.  In v2.1.0, PhysFS went from "getLastError()" which
	// returned a string, to "getLastErrorCode()" which returned an integer,
	// and "getErrorByCode()" which would convert the integer into a string.
	//
	// This is a really good change because it means that errors can be
	// localised into other languages.  But it's really inconvenient because
	// those functions don't exist before v2.1.0, and so we can't create them
	// ourselves, the way that we did for those other new functions.

	// So we're going to be absurd and evil in our glue code.  Hold my drink.

	enum PHYSFS_ErrorCode
	{
		PHYSFS_ERR_OK,
		PHYSFS_ERR_VERYNOTOK
	};
	PHYSFS_ErrorCode PHYSFS_getLastErrorCode()
	{
		if ( PHYSFS_getLastError() == NULL )
			return PHYSFS_ERR_OK; // no error.

		return PHYSFS_ERR_VERYNOTOK;
		// So if there's an error string, we're going to return non-0, which
		// code will correctly interpret as being an error code.  Nothing here
		// actually understands these codes right now, so it's safe for us
		// to just return an arbitrary non-0 amount.
	}

	const char* PHYSFS_getErrorByCode(int code)
	{
		return PHYSFS_getLastError();
		//
		// Hey look, they've asked us to translate the "error code" we gave them
		// in getLastErrorCode().  That code was meaningless, but let's just give
		// them the actual error string that PhysFS has given us.
		//
		// So normal code will go:
		//
		//     if ( getLastErrorCode() )
		//       Report( getErrorByCode( getLastErrorCode() ) );
		//
		// Or something like that.  And this approach works for that!
		//
		// Yeah this will break if anybody's doing something clever, such as
		// saving off an error code and looking it up later, but for all the
		// usage in this file right now, it'll be fine.
		//
		// But really, people should upgrade to 2.1.0 or later, so they don't need
		// to use this slightly-fragile hack.
		//
		// And by 'people', I mean 'me'.
		//
	}

#endif

// utility function, mimicking the behaviour of deprecated PHYSFS_getLastError().
const char* PHYSFS_getLastErrorString()
{
	PHYSFS_ErrorCode code = PHYSFS_getLastErrorCode();
	if ( code != PHYSFS_ERR_OK )
		return PHYSFS_getErrorByCode(code);
	return NULL;
}

struct zipdata
{
	z_stream m_zipStream;
};

static vsFile::openFailureHandler s_openFailureHandler = NULL;

vsFile::vsFile( const vsString &filename, vsFile::Mode mode ):
	m_filename(filename),
	m_tempFilename(),
	m_file(NULL),
	m_store(NULL),
	m_zipData(NULL),
	m_mode(mode),
	m_length(0),
	m_moveOnDestruction(false)
{
	// vsAssert( !DirectoryExists(filename), vsFormatString("Attempted to open directory '%s' as a plain file", filename.c_str()) );

	if ( (mode == MODE_Read || mode == MODE_ReadCompressed) &&
			vsFileCache::IsFileInCache( filename ) )
	{
		PROFILE_CACHED(filename);
		vsStore *c = vsFileCache::GetFileContents( filename );
		m_store = new vsStore(*c);
		m_mode = MODE_Read;
		m_length = m_store->BufferLength();
	}
	else
	{
		PROFILE(filename);
		if ( mode == MODE_Read || mode == MODE_ReadCompressed )
		{
			m_file = PHYSFS_openRead( filename.c_str() );
		}
		else if ( mode == MODE_Write || mode == MODE_WriteCompressed )
		{
			// in normal 'Write' mode, we actually write into a temporary file, and
			// then move it into position when the file is closed.  We do this so
			// that crashes in the middle of file writing don't obliterate the
			// original file (if any), or leave a half-written file.
			m_tempFilename = vsFormatString("tmp/%s", m_filename.c_str());
			vsString directoryOnly = m_tempFilename;
			size_t separator = directoryOnly.rfind("/");
			directoryOnly.erase(separator);
			EnsureWriteDirectoryExists(directoryOnly);
			m_file = PHYSFS_openWrite( m_tempFilename.c_str() );
			m_moveOnDestruction = true;

			m_store = new vsStore( 1024 * 1024 );

			if ( mode == MODE_WriteCompressed )
			{
				// we're going to write compressed data!
				m_zipData = new zipdata;
				m_zipData->m_zipStream.zalloc = Z_NULL;
				m_zipData->m_zipStream.zfree = Z_NULL;
				m_zipData->m_zipStream.opaque = Z_NULL;
				int ret = deflateInit(&m_zipData->m_zipStream, Z_DEFAULT_COMPRESSION);
				if ( ret != Z_OK )
				{
					vsLog("deflateInit error: %d", ret);
					mode = MODE_Write;
				}

				// What's more, compressed writing doesn't give us nice uniform
				// blobs of data to write.  Well-behaved clients might ask us to
				// write a megabyte at a time, and zlib turns around and tells us
				// that we have 2 bytes of data to write because it's found some
				// amazing compression scheme, or it simply isn't sure yet how it
				// wants to compress that data we've given it so far.
				//
				// As a result, we want to buffer our writes!
			}
		}
		else if ( mode == MODE_WriteDirectly )
		{
			m_file = PHYSFS_openWrite( filename.c_str() );
			mode = MODE_Write;
			m_mode = MODE_Write;
		}

		if ( m_file )
		{
			m_length = (size_t)PHYSFS_fileLength(m_file);
		}
		else
		{
			if ( s_openFailureHandler )
				(*s_openFailureHandler)( filename );
			vsAssert( m_file != NULL, STR("Error opening file '%s':  %s", filename.c_str(), PHYSFS_getLastErrorString()) );
		}

		bool shouldCache = (filename.find(".win") != vsString::npos) ||
			(filename.find(".glsl") != vsString::npos);

		if ( mode == MODE_Read )
		{
			// in read mode, let's just read out all the data right now in a single
			// big read, and close the file.  It's much (MUCH) faster to read if
			// the data's already in memory.  Particularly on Windows, where the
			// speed improvement can be several orders of magnitude, compared against
			// making lots of calls to read small numbers of bytes from the file.
			m_store = new vsStore( m_length );
			Store(m_store);

			PHYSFS_close(m_file);
			m_file = NULL;

			if ( shouldCache )
				vsFileCache::SetFileContents( filename, *m_store );
		}
		else if ( mode == MODE_ReadCompressed )
		{
			// in COMPRESSED read mode, we can't do the clever thing we did above
			// where we just made a single store and loaded the whole file into it
			// all at once, because we don't know how big the file is going to wind
			// up being.
			//
			// So here's what we're going to do;  we're going to load all the data
			// into a store (as above), decompress it once just to figure out how
			// big it's going to be, then decompress it AGAIN, into a store that's
			// the right size to hold it.

			vsStore *compressedData = new vsStore( m_length );
			Store(compressedData);
			PHYSFS_close(m_file);
			m_file = NULL;

			// we're going to read the compressed data TWICE.
			// First, we're just going to count how many bytes we inflate into.
			m_zipData = new zipdata;
			m_zipData->m_zipStream.zalloc = Z_NULL;
			m_zipData->m_zipStream.zfree = Z_NULL;
			m_zipData->m_zipStream.opaque = Z_NULL;
			int ret = inflateInit(&m_zipData->m_zipStream);
			if ( ret != Z_OK )
			{
				vsAssert( ret == Z_OK, vsFormatString("inflateInit error: %d", ret) );
				return;
			}

			uint32_t decompressedSize = 0;
			const uint32_t zipBufferSize = 1024 * 100;
			char zipBuffer[zipBufferSize];
			m_zipData->m_zipStream.avail_in = compressedData->BytesLeftForReading();
			m_zipData->m_zipStream.next_in = (Bytef*)compressedData->GetReadHead();
			do
			{
				m_zipData->m_zipStream.avail_out = zipBufferSize;
				m_zipData->m_zipStream.next_out = (Bytef*)zipBuffer;
				int ret = inflate(&m_zipData->m_zipStream, Z_NO_FLUSH);
				vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered in destructor");
				vsAssert(ret != Z_DATA_ERROR, "File is corrupt on disk (zlib reports Z_DATA_ERROR)");
				vsAssert(ret != Z_MEM_ERROR, "Out of memory loading file (zlib reports Z_MEM_ERROR)");
				// [NOTE] Z_BUF_ERROR is not fatal, according to https://www.zlib.net/manual.html
				// vsAssert(ret != Z_BUF_ERROR, "File is corrupt on disk (zlib reports Z_BUF_ERROR)");
				vsAssert(ret != Z_VERSION_ERROR, "File is incompatible (zlib reports Z_VERSION_ERROR)");

				uint32_t decompressedBytes = zipBufferSize - m_zipData->m_zipStream.avail_out;
				decompressedSize += decompressedBytes;

			}while( m_zipData->m_zipStream.avail_out == 0 );
			inflateEnd(&m_zipData->m_zipStream);

			m_store = new vsStore( decompressedSize );

			// Now let's decompress it FOR REAL.
			ret = inflateInit(&m_zipData->m_zipStream);
			if ( ret != Z_OK )
			{
				vsAssert( ret == Z_OK, vsFormatString("inflateInit error: %d", ret) );
				return;
			}

			m_zipData->m_zipStream.avail_in = compressedData->BytesLeftForReading();
			m_zipData->m_zipStream.next_in = (Bytef*)compressedData->GetReadHead();
			do
			{
				m_zipData->m_zipStream.avail_out = zipBufferSize;
				m_zipData->m_zipStream.next_out = (Bytef*)zipBuffer;
				int ret = inflate(&m_zipData->m_zipStream, Z_NO_FLUSH);
				vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered in destructor");

				int decompressedBytes = zipBufferSize - m_zipData->m_zipStream.avail_out;
				m_store->WriteBuffer( zipBuffer, decompressedBytes );

			}while( m_zipData->m_zipStream.avail_out == 0 );
			inflateEnd(&m_zipData->m_zipStream);

			// and now that we've decompressed all the data, we can drop into
			// regular 'Read' mode to serve the data to our clients.
			m_mode = MODE_Read;
			m_length = decompressedSize;
			vsDelete( compressedData );

			if ( shouldCache )
				vsFileCache::SetFileContents( filename, *m_store );
		}
	}
}

vsFile::~vsFile()
{
	if ( m_mode == MODE_WriteCompressed )
	{
		_PumpCompression( NULL, 0, true );
		deflateEnd(&m_zipData->m_zipStream);
	}
	FlushBufferedWrites();

	vsDelete( m_zipData );
	vsDelete( m_store );
	if ( m_file )
		PHYSFS_close(m_file);
	if ( m_moveOnDestruction )
	{
		// now we need to move the file we just wrote into its final position.
		Move( m_tempFilename, m_filename );
	}
}

void
vsFile::FlushBufferedWrites()
{
	if ( m_mode == MODE_Write || m_mode == MODE_WriteCompressed )
	{
		if ( m_store && m_store->BytesLeftForReading() )
		{
			PHYSFS_writeBytes( m_file, m_store->GetReadHead(), m_store->BytesLeftForReading() );
			m_store->Clear();
		}
	}
}

bool
vsFile::Exists( const vsString &filename ) // static method
{
	PHYSFS_Stat stat;
	if ( PHYSFS_stat(filename.c_str(), &stat) )
	{
		return true; // exists!
	}
	// PHYSFS_ErrorCode code = PHYSFS_getLastErrorCode();
	// const char* str = PHYSFS_getErrorByCode(code);
	// vsLog( "vsFile::Exists(%s) failed: (%d) %s", filename, code, str );
	// char** searchPath = PHYSFS_getSearchPath();
	// int pathId = 0;
	// while ( searchPath[pathId] )
	// {
	// 	vsLog("Search path: %s",searchPath[pathId]);
	// 	pathId++;
	// }
	return false;
}

bool
vsFile::DirectoryExists( const vsString &filename ) // static method
{
	// Caution:  PHYSFS_stat is *super* slow on Windows!  Often multiple
	// milliseconds, even off an SSD.
	//
	PHYSFS_Stat stat;
	if ( PHYSFS_stat(filename.c_str(), &stat) )
		return (stat.filetype == PHYSFS_FILETYPE_DIRECTORY);
	return false;
}

bool
vsFile::Delete( const vsString &filename ) // static method
{
	if ( DirectoryExists(filename) ) // This file is a directory, don't delete it!
		return false;

	return PHYSFS_delete(filename.c_str()) != 0;
}

bool
vsFile::Copy( const vsString &from, const vsString &to )
{
	if ( !vsFile::Exists(from) )
		return false;

	vsFile f(from);
	vsFile t(to, vsFile::MODE_WriteDirectly);

	vsStore s( f.GetLength() );
	f.Store(&s);
	t.Store(&s);

	return true;
}

bool
vsFile::Move( const vsString &from, const vsString &to )
{
	return ( Copy(from, to) && Delete(from) );
}

bool
vsFile::DeleteEmptyDirectory( const vsString &filename )
{
	// If it's not a directory, don't delete it!
	//
	// Note that PHYSFS_delete will return an error if we
	// try to delete a non-empty directory.
	//
	if ( DirectoryExists(filename) )
		return PHYSFS_delete(filename.c_str()) != 0;
	return false;
}

bool
vsFile::DeleteDirectory( const vsString &filename )
{
	if ( DirectoryExists(filename) )
	{
		vsArray<vsString> files;
        DirectoryContents(&files, filename);
		for ( int i = 0; i < files.ItemCount(); i++ )
		{
			vsString ff = vsFormatString("%s/%s", filename.c_str(), files[i].c_str());
			if ( vsFile::DirectoryExists( ff ) )
			{
				// it's a directory;  remove it!
				DeleteDirectory( ff );
			}
			else
			{
				// it's a file, delete it.
				Delete( ff );
			}
		}

		// I should now be empty, so delete me.
		return DeleteEmptyDirectory( filename );
	}
	return false;
}

bool
vsFile::MoveDirectory( const vsString& from_in, const vsString& to_in )
{
	vsString filename(from_in);
	vsString from, to;
	const char* physDir = PHYSFS_getRealDir( filename.c_str() );
	if ( physDir )
	{
		vsString dir(physDir);

#if defined(_WIN32)
		from = dir + "\\" + filename;
		to = dir + "\\" + to_in;
#else
		from = dir + "/" + filename;
		to = dir + "/" + to_in;
#endif
		return (0 == rename( from.c_str(), to.c_str() ));
	}

	return false;
}

namespace
{
	class sortFilesByModificationDate
	{
		vsString m_dirName;
	public:
		sortFilesByModificationDate( const vsString& dirName ):
			m_dirName(dirName + PHYSFS_getDirSeparator())
		{
		}

		bool operator()(char* a,char* b)
		{
			PHYSFS_Stat astat, bstat;
			if ( PHYSFS_stat((m_dirName + a).c_str(), &astat) &&
					PHYSFS_stat((m_dirName + b).c_str(), &bstat) )
			{
				PHYSFS_sint64 atime = astat.modtime;
				PHYSFS_sint64 btime = bstat.modtime;
				return ( atime > btime );
			}
			return 0;
		}
	};

	class sortFilesByName
	{
	public:
		sortFilesByName()
		{
		}

		bool operator()(char* a,char* b)
		{
			vsString as(a);
			vsString bs(b);
			return as < bs;
		}
	};
}

int
vsFile::DirectoryContents( vsArray<vsString>* result, const vsString &dirName ) // static method
{
    result->Clear();
	char **files = PHYSFS_enumerateFiles(dirName.c_str());
	if ( !files ) // error!
	{
		PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
		const char* errorMsg = PHYSFS_getErrorByCode( error );
		vsAssertF( files != NULL ,"PhysFS reports an error reading contents of directory '%s':  %s", dirName, errorMsg);
		return 0;
	}
	char **i;
	std::vector<char*> s;
	for (i = files; *i != NULL; i++)
		s.push_back(*i);

	std::sort(s.begin(), s.end(), sortFilesByName());
	// std::sort(s.begin(), s.end(), sortFilesByModificationDate(dirName));

	for (size_t i = 0; i < s.size(); i++)
		result->AddItem( s[i] );

	PHYSFS_freeList(files);

    return result->ItemCount();
}

int
vsFile::DirectoryFiles( vsArray<vsString>* result, const vsString &dirName ) // static method
{
	result->Clear();
	vsArray<vsString> all;
	DirectoryContents(&all, dirName);

	for ( int i = 0; i < all.ItemCount(); i++ )
	{
		vsString fn = all[i];
		if ( !vsFile::DirectoryExists( dirName + "/" + fn ) )
			result->AddItem(fn);
	}

    return result->ItemCount();
}

int
vsFile::DirectoryDirectories( vsArray<vsString>* result, const vsString &dirName ) // static method
{
    result->Clear();
	vsArray<vsString> all;
	DirectoryContents(&all, dirName);

	for ( int i = 0; i < all.ItemCount(); i++ )
	{
		vsString fn = all[i];
		if ( vsFile::DirectoryExists( dirName + "/" + fn ) )
			result->AddItem(fn);
	}

    return result->ItemCount();
}

void
vsFile::EnsureWriteDirectoryExists( const vsString &writeDirectoryName ) // static method
{
	if ( !DirectoryExists(writeDirectoryName) )
	{
		int mkdirResult = PHYSFS_mkdir( writeDirectoryName.c_str() );
		vsAssert( mkdirResult != 0, vsFormatString("Failed to create directory '%s%s%s': %s",
				PHYSFS_getWriteDir(), PHYSFS_getDirSeparator(), writeDirectoryName.c_str(), PHYSFS_getLastErrorString()) );
	}
}

bool
vsFile::PeekRecord( vsRecord *r )
{
	vsAssert(r != NULL, "Called vsFile::Record with a NULL vsRecord!");

	if ( m_mode == MODE_Write || m_mode == MODE_WriteCompressed )
	{
		vsAssert( m_mode != MODE_Write, "Error:  Not legal to PeekRecord() when writing a file!" );
		return false;
	}
	else
	{
		bool succeeded = false;
		vsString line;

		r->Init();

		size_t filePos = m_store->GetReadHeadPosition();
		if ( r->Parse(this) )
			succeeded = true;

		m_store->SeekReadHeadTo(filePos);
		// PHYSFS_seek(m_file, filePos);

		return succeeded;
	}

	return false;
}

bool
vsFile::Record( vsRecord *r )
{
	vsAssert(r != NULL, "Called vsFile::Record with a NULL vsRecord!");

	if ( m_mode == MODE_Write || m_mode == MODE_WriteCompressed )
	{
		vsString recordString = r->ToString();

		PHYSFS_writeBytes( m_file, recordString.c_str(), recordString.size() );

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
vsFile::Record_Binary( vsRecord *r )
{
	// utility function;  actually, we do this from the Record side.
	if ( m_mode == MODE_Write || m_mode == MODE_WriteCompressed )
	{
		r->SaveBinary(this);
		return true;
	}
	else
	{
		if ( AtEnd() )
			return false;
		return r->LoadBinary(this);
	}
}

bool
vsFile::ReadLine( vsString *line )
{
	// const int c_bufSize = 1024;
	// char buf[c_bufSize];

	size_t filePos = m_store->GetReadHeadPosition();
	char peekChar = 'a';
	bool done = false;

	while ( !done && !AtEnd() && peekChar != '\n' && peekChar != 0 )
	{
		peekChar = m_store->ReadInt8();
	}
	size_t afterFilePos = m_store->GetReadHeadPosition();
	size_t bytes = afterFilePos - filePos;
	m_store->SeekReadHeadTo(filePos);
	if ( bytes > 0 )
	{
		char* buffer = (char*)malloc(bytes+1);
		m_store->ReadBuffer(buffer, bytes);

		// if we read a newline, let's just ignore it.
		if ( peekChar == '\n' )
		{
			bytes--;
		}

		buffer[bytes] = 0;

		*line = buffer;
		size_t i;
		while ( (i = line->find('\r')) != vsString::npos)
		{
			line->erase(i,1);
		}

		free(buffer);

		return true;
	}
	return false;
}

bool
vsFile::PeekLine( vsString *line )
{
	size_t filePos = m_store->GetReadHeadPosition();
	bool result = ReadLine(line);
	m_store->SeekReadHeadTo(filePos);
	return result;
	// PHYSFS_sint64 filePos = PHYSFS_tell(m_file);
	// bool result = ReadLine(line);
	// PHYSFS_seek(m_file, filePos);
	// return result;
}

void
vsFile::Rewind()
{
	if ( m_store )
		m_store->SeekReadHeadTo(0);
	if ( m_file )
		PHYSFS_seek(m_file, 0);
}

void
vsFile::Store( vsStore *s )
{
	s->Rewind();
	if ( m_mode == MODE_Write ||
			m_mode == MODE_WriteCompressed ||
			m_mode == MODE_WriteDirectly )
	{
		_WriteBytes(s->GetReadHead(), s->BytesLeftForReading());
	}
	else
	{
		if ( m_file )
		{
			PHYSFS_sint64 n;
			n = PHYSFS_readBytes( m_file, s->GetWriteHead(), s->BufferLength() );

			if ( s->BufferLength() < (size_t)n )
			{
				// fatal error:  Let's trace out deets!
				vsLog("Ut-oh, major file loading error has happened;  somehow we've read more bytes than expected and we have nowhere to store them?");
				vsLog("We're about to die, probably, so here's some deets!");

				vsLog("File:  %s", m_filename);
				vsLog("Bytes read: %d", n);
				vsLog("Space to store them: %d", s->BufferLength());
			}
			s->SetLength((size_t)n);
		}
		else
		{
			s->Clear();
			s->Append(m_store);
		}
	}
}

void
vsFile::WriteBytes( const void* data, size_t bytes )
{
	vsAssert( m_mode == MODE_Write ||
			m_mode == MODE_WriteCompressed ||
			m_mode == MODE_WriteDirectly, "Tried to write into a file which is in read mode?" );
	{
		_WriteBytes(data, bytes);
	}
}

int
vsFile::ReadBytes( void* data, size_t bytes )
{
	vsAssert( m_mode == MODE_Read, "Tried to read bytes when not in read mode?" );

	int bytesToRead = vsMin(bytes, m_store->BytesLeftForReading());
	memcpy( data, m_store->GetReadHead(), bytesToRead );
	m_store->AdvanceReadHead(bytesToRead);
	return bytesToRead;
}

void
vsFile::_WriteFinalBytes_Buffered( const void* bytes, size_t byteCount )
{
	vsAssert( m_mode == MODE_Write || m_mode == MODE_WriteCompressed,
			"Trying to write but we're not in write mode??" );

	if ( m_store )
	{
		if ( byteCount < m_store->BytesLeftForWriting() )
		{
			m_store->WriteBuffer(bytes, byteCount);
		}
		else
		{
			while( byteCount > 0 )
			{
				size_t bytesWeCanWrite = vsMin( byteCount, m_store->BytesLeftForWriting() );
				m_store->WriteBuffer(bytes, bytesWeCanWrite);
				bytes = (char*)(bytes) + bytesWeCanWrite;

				PHYSFS_writeBytes( m_file, m_store->GetReadHead(), m_store->BytesLeftForReading() );
				m_store->Clear();
				byteCount -= bytesWeCanWrite;
			}
		}
	}
	else
	{
		// we're not writing in a buffered context.  Just write the bytes directly.
		PHYSFS_writeBytes( m_file, bytes, byteCount );
	}
}

void
vsFile::_WriteBytes( const void* bytes, size_t byteCount )
{
	if ( m_mode == MODE_Write || m_mode == MODE_WriteDirectly )
	{
		_WriteFinalBytes_Buffered(bytes, byteCount);
	}
	else if ( m_mode == MODE_WriteCompressed )
	{
		_PumpCompression( bytes, byteCount, false );
	}
	else
	{
		vsAssert(0, "Tried to write bytes when we're not in a 'write' mode??");
	}
}

void
vsFile::_PumpCompression( const void* bytes, size_t byteCount, bool finish )
{
	vsAssert( m_mode == MODE_WriteCompressed, "Trying to pump compression when we're not in WriteCompressed mode??" );

	const int zipBufferSize = 1024 * 100;
	char zipBuffer[zipBufferSize];
	m_zipData->m_zipStream.avail_in = byteCount;
	m_zipData->m_zipStream.next_in = (Bytef*)bytes;
	do
	{
		m_zipData->m_zipStream.avail_out = zipBufferSize;
		m_zipData->m_zipStream.next_out = (Bytef*)zipBuffer;
		int ret = deflate(&m_zipData->m_zipStream, finish ? Z_FINISH : Z_NO_FLUSH);
		vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered by deflate()");

		int compressedBytes = zipBufferSize - m_zipData->m_zipStream.avail_out;
		if ( compressedBytes > 0 )
			_WriteFinalBytes_Buffered(zipBuffer, compressedBytes);

	}while( m_zipData->m_zipStream.avail_out == 0 );
	vsAssert( m_zipData->m_zipStream.avail_in == 0, "Didn't compress all the available input data?" );
}

void
vsFile::StoreBytes( vsStore *s, size_t bytes )
{
	if ( m_mode == MODE_Write ||
			m_mode == MODE_WriteCompressed ||
			m_mode == MODE_WriteDirectly )
	{
		_WriteBytes(s->GetReadHead(), vsMin(bytes,s->BytesLeftForReading()));
	}
	else
	{
		size_t actualBytes = vsMin(bytes, m_store->BytesLeftForReading());
		s->WriteBuffer( m_store->GetReadHead(), actualBytes );
		m_store->AdvanceReadHead(actualBytes);
	}
}

// ONLY IN READ OPERATIONS.  Peek the requested number of bytes.
void
vsFile::PeekBytes( vsStore *s, size_t bytes )
{
	vsAssert( m_mode != MODE_Write &&
			m_mode != MODE_WriteCompressed &&
			m_mode != MODE_WriteDirectly, "PeekBytes() called in Write mode?" );

	size_t actualBytes = vsMin(bytes, m_store->BytesLeftForReading());
	s->WriteBuffer( m_store->GetReadHead(), actualBytes );
}

void
vsFile::ConsumeBytes( size_t bytes )
{
	m_store->AdvanceReadHead(bytes);
}

vsString
vsFile::GetFullFilename(const vsString &filename_in)
{
#if defined(VS_TOOL)
	return filename_in;
#endif

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
	vsString filename(filename_in);
	const char* physDir = PHYSFS_getRealDir( filename.c_str() );
	if ( physDir )
	{
		vsString dir(physDir);

#if defined(_WIN32)
		return dir + "\\" + filename;
#else
		return dir + "/" + filename;
#endif
	}
	vsAssert(0, vsFormatString( "No such file: %s", filename_in.c_str() ) );
	return filename;
#endif
}


bool
vsFile::AtEnd()
{
	return m_store->BytesLeftForReading() == 0; // !m_file || PHYSFS_eof( m_file );
}

void
vsFile::SetFileOpenFailureHandler( openFailureHandler handler )
{
	s_openFailureHandler = handler;
}

vsString
vsFile::GetExtension( const vsString &filename )
{
	size_t i = filename.rfind('.');
	if ( i != vsString::npos )
		return filename.substr(i+1);
	return vsEmptyString;
}

vsString
vsFile::GetBaseName( const vsString &filename )
{
	vsString result = filename;
	size_t ext = result.rfind('.');
	if ( ext != vsString::npos )
		result.erase(ext);
	size_t dir = result.rfind('/');
	if ( dir != vsString::npos )
		result.erase(0,dir+1);
	return result;
}

vsString
vsFile::GetFileName( const vsString &filename )
{
	// [TODO] figure out what I want to do about directory separators on Windows.
	// I *think* we're using Linux-style directories everywhere, even on
	// Windows.  But.. as written, this code won't work with Windows-style
	// backslash-delimited directory paths.
	//
	size_t i = filename.rfind('/');
	if ( i != vsString::npos )
		return filename.substr(i+1);
	return filename;
}

vsString
vsFile::GetDirectory( const vsString &filename )
{
	// [TODO] figure out what I want to do about directory separators on Windows.
	// I *think* we're using Linux-style directories everywhere, even on
	// Windows.  But.. as written, this code won't work with Windows-style
	// backslash-delimited directory paths.
	//
	size_t i = filename.rfind('/');
	if ( i != vsString::npos )
		return filename.substr(0,i);
	return vsString("./");
}

