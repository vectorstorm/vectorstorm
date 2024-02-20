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
#include "VS_Mutex.h"
#include "Core.h"
#include "CORE_Game.h"

#include "VS_PhysFS.h"

#if defined(UNIX)
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/statvfs.h>
#endif // UNIX

#include <SDL2/SDL_filesystem.h>

#include <atomic>
#include <errno.h>
#include <zlib.h>

#include "VS_DisableDebugNew.h"
#include <vector>
#include <algorithm>
#include "VS_EnableDebugNew.h"

#include <filesystem>


#define UTF_CPP_CPLUSPLUS (201703L) // C++17 - __cplusplus isn't being filled in by Visual Studio by default for some reason??  Really we should be giving VS a /Zc:__cplusplus compiler option to make it do what the standard says it should always do.
#include "Utils/utfcpp/utf8.h"
#undef UTF_CPP_CPLUSPLUS

namespace
{
	vsMutex s_renameMutex;
	std::atomic<int> s_tempFileCount;

	int NextTmpFileCount()
	{
		return ++s_tempFileCount;
	}
}

// #define PROFILE_FILE_SYSTEM
#ifdef PROFILE_FILE_SYSTEM

#include "VS_TimerSystem.h"


namespace {

	// this mutex ensures that we don't try to rename multiple files
	// simultaneously, which prevents us from having two threads both
	// trying to overwrite the same file at the same time.

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



struct zipdata
{
	z_stream m_zipStream;
};

namespace
{
	vsString MakeWriteFilename( const vsString& in )
	{
		vsString out(in);
		if ( 0 == out.find("user/") )
		{
			out.erase(0,5);
		}
		if ( 0 == out.find("base/") )
		{
			out.erase(0,5);
		}
		return out;
	}
};

static vsFile::openFailureHandler s_openFailureHandler = nullptr;

vsFile::vsFile( const vsString &filename_in, vsFile::Mode mode ):
	m_filename(filename_in),
	m_tempFilename(),
	m_file(nullptr),
	m_compressedStore(nullptr),
	m_store(nullptr),
	m_zipData(nullptr),
	m_mode(mode),
	m_length(0),
	m_moveOnDestruction(false)
{
	vsString filename(filename_in);

	if ( mode == MODE_Write || mode == MODE_WriteDirectly || mode == MODE_WriteCompressed )
	{
		// Convert our 'user/' filename into a write directory-relative path.
		vsAssertF(0 == filename.find("user/"), "Code error: trying to write into a file '%s' which isn't in our user-writable directory!", filename_in);

		filename = MakeWriteFilename(filename_in);
	}

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
		if ( mode == MODE_Read || mode == MODE_ReadCompressed || mode == MODE_ReadCompressed_Progressive )
		{
			m_file = PHYSFS_openRead( filename.c_str() );
		}
		else if ( mode == MODE_Write || mode == MODE_WriteCompressed )
		{
			// in normal 'Write' mode, we actually write into a temporary file, and
			// then move it into position when the file is closed.  We do this so
			// that crashes in the middle of file writing don't obliterate the
			// original file (if any), or leave a half-written file.
			m_tempFilename = vsFormatString("user/tmp/%s.%d", filename.c_str(), NextTmpFileCount());
			vsString directoryOnly = m_tempFilename;
			size_t separator = directoryOnly.rfind("/");
			directoryOnly.erase(separator);
			EnsureWriteDirectoryExists(directoryOnly);
			m_file = PHYSFS_openWrite( MakeWriteFilename(m_tempFilename).c_str() );
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
			vsString errorMsg = PHYSFS_getLastErrorString();
			vsString errorFilename = filename;
			if ( vsFile::Exists(errorFilename) )
				errorFilename = GetFullFilename(filename);

			// Now let's try this thing:
			// try
			// {
			// 	std::filesystem::file_status fs = std::filesystem::status( errorFilename );
			// 	std::filesystem::perms perm = fs.permissions();
			// 	if ( (perm & std::filesystem::perms::owner_read) == std::filesystem::perms::none )
			// 		vsLog("Readable!");
			// }
			// catch ( std::filesystem::filesystem_error& err )
			// {
			// 	vsLog("what: %s", err.what());
			// 	vsLog("path1: %s", err.path1());
			// 	vsLog("path2: %s", err.path2());
			// 	vsLog("code().value(): %d", err.code().value());
			// 	vsLog("code().message(): %s", err.code().message());
			// 	// vsLog("code().category(): %s", err.code().category().message());
			// }

			if ( s_openFailureHandler )
				(*s_openFailureHandler)( errorFilename, errorMsg );
			vsAssert( m_file != nullptr, STR("Error opening file '%s' (trying '%s'):  %s", filename, errorFilename, errorMsg) );
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
			m_file = nullptr;

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
			m_file = nullptr;

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
			m_zipData->m_zipStream.avail_in = (uInt)compressedData->BytesLeftForReading();
			m_zipData->m_zipStream.next_in = (Bytef*)compressedData->GetReadHead();
			do
			{
				m_zipData->m_zipStream.avail_out = zipBufferSize;
				m_zipData->m_zipStream.next_out = (Bytef*)zipBuffer;
				int ret = inflate(&m_zipData->m_zipStream, Z_NO_FLUSH);
				vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered in destructor");
				vsAssertF(ret != Z_DATA_ERROR, "File '%s' is corrupt on disk (zlib reports Z_DATA_ERROR)", filename);
				vsAssertF(ret != Z_MEM_ERROR, "Out of memory loading file '%s' (zlib reports Z_MEM_ERROR)", filename);
				// [NOTE] Z_BUF_ERROR is not fatal, according to https://www.zlib.net/manual.html
				// vsAssert(ret != Z_BUF_ERROR, "File is corrupt on disk (zlib reports Z_BUF_ERROR)");
				vsAssertF(ret != Z_VERSION_ERROR, "File '%s' is incompatible (zlib reports Z_VERSION_ERROR)", filename);

				uint32_t decompressedBytes = zipBufferSize - m_zipData->m_zipStream.avail_out;
				decompressedSize += decompressedBytes;

			}while( m_zipData->m_zipStream.avail_out == 0 );
			inflateEnd(&m_zipData->m_zipStream);

			m_store = new vsStore( decompressedSize );

			// Now let's decompress it FOR REAL.
			ret = inflateInit(&m_zipData->m_zipStream);
			if ( ret != Z_OK )
			{
				vsAssertF( ret == Z_OK, "File '%s': inflateInit error: %d", filename, ret );
				return;
			}

			m_zipData->m_zipStream.avail_in = (uInt)compressedData->BytesLeftForReading();
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
		else if ( mode == MODE_ReadCompressed_Progressive )
		{
			// Okay.  We're going to load our compressed data into our 'm_store'
			// variable, and are then going to set up a decompression buffer to load
			// data into.
			//
			// [TODO] Maybe don't do a full load of the comrpessed data into memory
			// and instead load data into a temporary buffer when we need more to
			// finish some decompression.
			//
			const uint32_t compressedBufferSize = 1024 * 100;
			m_compressedStore = new vsStore( compressedBufferSize );
			// Store(m_compressedStore);
			// PHYSFS_close(m_file);
			// m_file = nullptr;

			m_zipData = new zipdata;
			m_zipData->m_zipStream.zalloc = Z_NULL;
			m_zipData->m_zipStream.zfree = Z_NULL;
			m_zipData->m_zipStream.opaque = Z_NULL;

			// Now, we need to set up our zip stream
			const uint32_t zipBufferSize = 1024 * 100;
			m_store = new vsStore( zipBufferSize );
			m_zipData->m_zipStream.avail_out = (uInt)m_store->BytesLeftForWriting();
			m_zipData->m_zipStream.next_out = (Bytef*)m_store->GetWriteHead();

			// Now let's get set to decompress it FOR REAL.
			int ret = inflateInit(&m_zipData->m_zipStream);
			if ( ret != Z_OK )
			{
				vsAssert( ret == Z_OK, vsFormatString("inflateInit error: %d", ret) );
				return;
			}

			m_zipData->m_zipStream.avail_in = m_compressedStore->BytesLeftForReading();
			m_zipData->m_zipStream.next_in = (Bytef*)m_compressedStore->GetReadHead();
		}
	}
}

vsFile::~vsFile()
{
	if ( m_mode == MODE_WriteCompressed )
	{
		_PumpCompression( nullptr, 0, true );
		deflateEnd(&m_zipData->m_zipStream);
	}
	else if ( m_mode == MODE_ReadCompressed_Progressive )
	{
		inflateEnd(&m_zipData->m_zipStream);
	}

	FlushBufferedWrites();

	vsDelete( m_compressedStore );
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
			_DoWriteLiteralBytes( m_store->GetReadHead(), m_store->BytesLeftForReading() );
			m_store->Clear();
		}
	}
}

void
vsFile::_DoWriteLiteralBytes( const void* bytes, size_t byteCount )
{
	PHYSFS_uint64 bytesToWrite = byteCount;
	PHYSFS_sint64 bytesWritten = PHYSFS_writeBytes( m_file, bytes, bytesToWrite );
	if ( bytesWritten != (PHYSFS_sint64)bytesToWrite )
	{
		vsLog("Tried to write %ld bytes to file, actually wrote %ld bytes",
				(PHYSFS_sint64)bytesToWrite, bytesWritten);

		PHYSFS_ErrorCode code = PHYSFS_getLastErrorCode();
		const char* errStr = PHYSFS_getErrorByCode(code);

		// this file is truncated;  let's try to delete it rather than let it
		// live in a partially-written state!
		// bool successfullyDeleted = false;

		PHYSFS_close(m_file);
		if ( vsFile::Exists(m_tempFilename) )
			vsFile::Delete( m_tempFilename );
		if ( vsFile::Exists(m_filename) )
			vsFile::Delete( m_filename );

		vsAssertF( bytesWritten == (PHYSFS_sint64)bytesToWrite,
				"Unable to write file(%s): (%ld) %s",
				m_filename,
				code,
				errStr)
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

	return PHYSFS_delete( MakeWriteFilename(filename).c_str() ) != 0;
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

namespace {

	// this is our fallback "rename" functionality which just loads
	// the file and writes it out into a new place, then deletes the
	// original.  Works everywhere with no special OS support required.
	bool FallbackRename( const vsString& from, const vsString& to )
	{
		vsFile *f = new vsFile(from);
		vsStore s( f->GetLength() );
		f->Store(&s);
		vsDelete(f);

		vsFile::Delete(from);

		// need to use the raw 'to_in' here so we still have the 'user' bit
		// out in front
		vsFile t(to, vsFile::MODE_WriteDirectly);
		t.Store(&s);

		return true;
	}
}

bool
vsFile::Move( const vsString &user_from, const vsString &user_to )
{
	vsScopedLock lock(s_renameMutex);

	if ( !vsFile::Exists(user_from) )
		return false;

	std::filesystem::path f = GetFullFilename(user_from);
	std::filesystem::path t = PHYSFS_getWriteDir() + MakeWriteFilename( user_to ); // make sure we pull out the virtual 'user' folder if any!

#if defined(__APPLE_CC__)
	// For whatever reason, apple has tied C++ features to OS versions, and
	// we've declared ourselves as only requiring OSX 10.9, which means we
	// don't have access to std::filesystem.  (We can get it in 10.15, but as
	// that's only two years old (at time of writing), it feels like a big
	// ask).  Let's have Mac builds just do our fallback rename for now, rather
	// than requiring an updated OS just so we can use std::filesystem.  It's
	// not worth requiring the OS upgrade!
	//
	// Let's use POSIX ::rename() and then our fallback, here.  That should be
	// available everywhere/everywhen on OSX!

	if ( 0 != ::rename(f.c_str(),t.c_str()) )
	{
		vsLog("While attempting rename: %s -> %s", f.c_str(), t.c_str());
		vsLog("::rename error: %d;  doing remove+rename instead", errno);
		return FallbackRename(from, to_in);
	}
	return true;
#else // !defined(__APPLE_CC__)


	std::filesystem::path fp(f);
	std::filesystem::path tp(t);
	fp.make_preferred();
	tp.make_preferred();

	// std::string fps = fp.string();

	// vsLog("From: %s", GetFullFilename(from));
	// vsLog("FromPath: %s", fp.string());
	// vsLog("To: %s", PHYSFS_getWriteDir() + to);
	// vsLog("ToPath: %s", tp.string());

	try
	{
		std::filesystem::rename( fp, tp );
	}
	catch( std::exception &e )
	{
		// Okay, std::filesystem::rename() has failed for some reason.  Fall back
		// to trying to delete and move.
		vsLog("While attempting rename: %s -> %s", fp.string(), tp.string());
		vsLog("filesystem::rename error: \"%s\";  doing remove+rename instead", e.what());
		try
		{
			std::filesystem::remove( tp );
			std::filesystem::rename( fp, tp );
		}
		catch( std::exception &ee )
		{
			// okay, that failed too for some reason.  Just do a manual copy.
			vsLog("Remove and rename error: \"%s\"", ee.what());
			vsLog("Trying manual copy instead...");
			return FallbackRename(user_from, user_to);
		}
	}
	return true;
#endif // !defined(__APPLE_CC__)
}

bool
vsFile::DeleteEmptyDirectory( const vsString &filename )
{
	// If it's not a directory, don't delete it!
	//
	// Note that PHYSFS_delete will return an error if we
	// try to delete a non-empty directory.
	//
	if ( DirectoryExists(filename) ) // This directory exists?
		return PHYSFS_delete(MakeWriteFilename(filename).c_str()) != 0;
	return false;
}

bool
vsFile::DeleteDirectory( const vsString &filename )
{
	if ( DirectoryExists(filename) ) // This directory exists?
	{
		vsArray<vsString> files;
		DirectoryContents(&files, filename);
		for ( int i = 0; i < files.ItemCount(); i++ )
		{
			vsString ff = vsFormatString("%s/%s", filename.c_str(), files[i].c_str());
			if ( vsFile::DirectoryExists(ff) )
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
vsFile::MoveDirectory( const vsString& from, const vsString& to )
{
	vsString f = GetFullFilename(from);
	vsString t = PHYSFS_getWriteDir() + MakeWriteFilename( to ); // make sure we pull out the virtual 'user' folder if any!

	std::filesystem::path fp(f);
	std::filesystem::path tp(t);

	fp.make_preferred();
	tp.make_preferred();

	if ( !vsFile::Exists(from) )
		return false;

	if ( !std::filesystem::is_directory(tp) )
	{
		// destination directory doesn't exist, so we should be able to just
		// rename ourselves over there and be done!
		try
		{
			std::filesystem::rename(fp,tp);
			return true;
		}
		catch( std::exception &e )
		{
			vsLog("Attempt to rename directory into place failed.  Falling back on manual file move");
		}
	}

	if ( MoveDirectoryContents(from, to) )
	{
		DeleteDirectory(from);
		return true;
	}

	return false;
}

bool
vsFile::MoveDirectoryContents( const vsString& from, const vsString& to )
{
	// We're going to try to move the files inside 'from' to the specified
	// other directory.  This function is recursive, and will be called on
	// directories inside the 'from' directory to the 'to' directory,
	// which we'll do using the ::Move() function above.

	// vsLog("From: %s", from);
	// vsLog("To: %s", to);

	EnsureWriteDirectoryExists(to);

	{
		vsArray<vsString> files, directories;
		DirectoryFiles(&files, from);
		DirectoryDirectories(&directories, from);

		for ( int i = 0; i < files.ItemCount(); i++ )
		{
			// don't move these autocloud files if we find any
			if ( files[i] != "steam_autocloud.vdf" )
			{
				vsString fileFrom = vsFormatString("%s/%s", from, files[i] );
				vsString fileTo = vsFormatString("%s/%s", to, files[i] );
				Move( fileFrom, fileTo );
			}
		}
		for ( int i = 0; i < directories.ItemCount(); i++ )
		{
			vsString directoryFrom = vsFormatString("%s/%s", from, directories[i] );
			vsString directoryTo = vsFormatString("%s/%s", to, directories[i] );
			MoveDirectoryContents( directoryFrom, directoryTo );
		}
	}
	return true;
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
		vsAssertF( files != nullptr ,"PhysFS reports an error reading contents of directory '%s':  %s", dirName, errorMsg);
		return 0;
	}
	char **i;
	std::vector<char*> s;
	for (i = files; *i != nullptr; i++)
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
vsFile::EnsureWriteDirectoryExists( const vsString &directoryName ) // static method
{
	if ( !DirectoryExists(directoryName) )
	{
		int mkdirResult = PHYSFS_mkdir( MakeWriteFilename(directoryName).c_str() );
		vsAssert( mkdirResult != 0, vsFormatString("Failed to create directory '%s%s%s': %s",
				PHYSFS_getWriteDir(), PHYSFS_getDirSeparator(), directoryName.c_str(), PHYSFS_getLastErrorString()) );
	}
}

bool
vsFile::PeekRecord( vsRecord *r )
{
	vsAssert(r != nullptr, "Called vsFile::Record with a nullptr vsRecord!");

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
	vsAssert(r != nullptr, "Called vsFile::Record with a nullptr vsRecord!");

	if ( m_mode == MODE_Write || m_mode == MODE_WriteCompressed )
	{
		vsString recordString = r->ToString();

		_DoWriteLiteralBytes( recordString.c_str(), recordString.size() );

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
		if ( m_mode == MODE_Read )
		{
			if ( AtEnd() )
				return false;
		}
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

	while ( !AtEnd() && peekChar != '\n' && peekChar != 0 )
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

			if ( n < 0 )
			{
				// read error occurred.
				vsLog( "Ut-oh, file reading error has been returned;  PHYSFS_readBytes() has returned %d", n  );;
				vsLog("File:  %s", m_filename);
				vsLog("Bytes read: %d", n);
				vsLog("Space to store them: %d", s->BufferLength());

				PHYSFS_ErrorCode errorCode = PHYSFS_getLastErrorCode();
				vsLog("Error code: %d", errorCode);
				vsLog("Error string: %s", PHYSFS_getErrorByCode(errorCode));

			}
			else if ( s->BufferLength() < (size_t)n )
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
	// Return number of bytes read

	if ( m_mode == MODE_Read )
	{
		int bytesToRead = vsMin(bytes, m_store->BytesLeftForReading());
		memcpy( data, m_store->GetReadHead(), bytesToRead );
		m_store->AdvanceReadHead(bytesToRead);
		return bytesToRead;
	}
	else if ( m_mode == MODE_ReadCompressed_Progressive )
	{
		bool atEnd = false;
		// out of bytes to decompress;  load more!
		while ( m_store->BytesLeftForReading() < bytes && !atEnd )
		{
			// decompress data from our compressed buffer!
			if ( m_compressedStore->BytesLeftForReading() )
			{
				m_store->EraseReadBytes();

				m_zipData->m_zipStream.avail_in = m_compressedStore->BytesLeftForReading();
				m_zipData->m_zipStream.next_in = (Bytef*)m_compressedStore->GetReadHead();

				m_zipData->m_zipStream.avail_out = m_store->BytesLeftForWriting();
				m_zipData->m_zipStream.next_out = (Bytef*)m_store->GetWriteHead();

				int ret = inflate(&m_zipData->m_zipStream, Z_NO_FLUSH);
				vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered in destructor");
				vsAssert(ret != Z_DATA_ERROR, "File is corrupt on disk (zlib reports Z_DATA_ERROR)");
				vsAssert(ret != Z_MEM_ERROR, "Out of memory loading file (zlib reports Z_MEM_ERROR)");
				// [NOTE] Z_BUF_ERROR is not fatal, according to https://www.zlib.net/manual.html
				// vsAssert(ret != Z_BUF_ERROR, "File is corrupt on disk (zlib reports Z_BUF_ERROR)");
				vsAssert(ret != Z_VERSION_ERROR, "File is incompatible (zlib reports Z_VERSION_ERROR)");

				uint32_t compressedBytesDecompressed = m_compressedStore->BytesLeftForReading() - m_zipData->m_zipStream.avail_in;
				uint32_t decompressedBytes = m_store->BytesLeftForWriting() - m_zipData->m_zipStream.avail_out;

				m_compressedStore->AdvanceReadHead(compressedBytesDecompressed);
				m_store->AdvanceWriteHead( decompressedBytes );
			}

			if ( m_store->BytesLeftForReading() < bytes )
			{
				// okay, we've exhausted our compressed store!  Let's load more
				// data from disk into the compressed store, if there's any more
				// data to read!
				if ( m_file )
				{
					m_compressedStore->EraseReadBytes();
					int n = PHYSFS_readBytes( m_file,
							m_compressedStore->GetWriteHead(),
							m_compressedStore->BytesLeftForWriting() );

					// did we hit the end?
					if ( n < (int)m_compressedStore->BytesLeftForWriting() )
					{
						PHYSFS_close( m_file );
						m_file = nullptr;
					}
					m_compressedStore->AdvanceWriteHead(n);
				}
				else
					atEnd = true;
			}
		}

		bytes = vsMin(bytes, m_store->BytesLeftForReading());
		m_store->ReadBuffer(data, bytes);
		return bytes;
	}
	else
	{
		vsAssertF( 0, "Tried to read bytes when file '%s' is not in read mode?", m_filename );
	}
	return 0;
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

				_DoWriteLiteralBytes( m_store->GetReadHead(), m_store->BytesLeftForReading() );
				m_store->Clear();
				byteCount -= bytesWeCanWrite;
			}
		}
	}
	else
	{
		// we're not writing in a buffered context.  Just write the bytes directly.
		_DoWriteLiteralBytes( bytes, byteCount );
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

	vsString filename(filename_in);
	const char* physDir = PHYSFS_getRealDir( filename.c_str() );
	filename = MakeWriteFilename( filename ); // make sure we pull out the virtual 'user' folder if any!

	if ( physDir )
	{
		vsString dir(physDir);

		if ( dir.back() == '/' )
			return dir + filename;
		else
			return dir + "/" + filename;
	}
	vsAssert(0, vsFormatString( "No such file: %s", filename_in.c_str() ) );
	return filename;
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

size_t
vsFile::AvailableWriteBytes()
{
	vsString directory( PHYSFS_getWriteDir() );
#if defined(__APPLE_CC__)
	struct statvfs info;
	if ( ::statvfs( directory.c_str(), &info ) != 0 )
	{
		vsLog("fstatvfs for directory %s failed", directory);
	}
	else
	{
		vsLog("Directory: %s", directory);
		vsLog("  Block size:   %d", info.f_frsize);
		vsLog("  Total blocks: %d", info.f_blocks);
		vsLog("  Free blocks:  %d", info.f_bfree);

		return info.f_frsize * info.f_bfree;
	}
	return std::numeric_limits<size_t>::max();
#else
	try
	{
		std::filesystem::space_info si = std::filesystem::space(directory);
		vsLog("Directory: %s", directory);
		vsLog("  capacity:   %d", si.capacity);
		vsLog("  free: %d", si.free);
		vsLog("  available:  %d", si.available);
		return si.available;
	}
	catch(const std::exception& e)
	{
		vsLog("Failed to call std::filesystem::space on '%s': %s", directory, e.what());
	}
	return std::numeric_limits<size_t>::max();
#endif
}

