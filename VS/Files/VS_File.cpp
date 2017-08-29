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
#include <zlib.h>

#include "VS_DisableDebugNew.h"
#include <vector>
#include <algorithm>
#include "VS_EnableDebugNew.h"

struct zipdata
{
	z_stream m_zipStream;
};

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
	vsAssert( !DirectoryExists(filename), vsFormatString("Attempted to open directory '%s' as a plain file", filename.c_str()) );

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

	vsAssert( m_file != NULL, STR("Error opening file '%s':  %s", filename.c_str(), PHYSFS_getLastError()) );

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

		int decompressedSize = 0;
		int zipBufferSize = 1024 * 100;
		char zipBuffer[zipBufferSize];
		m_zipData->m_zipStream.avail_in = compressedData->BytesLeftForReading();
		m_zipData->m_zipStream.next_in = (Bytef*)compressedData->GetReadHead();
		do
		{
			m_zipData->m_zipStream.avail_out = zipBufferSize;
			m_zipData->m_zipStream.next_out = (Bytef*)zipBuffer;
			int ret = inflate(&m_zipData->m_zipStream, Z_NO_FLUSH);
			vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered in destructor");

			int decompressedBytes = zipBufferSize - m_zipData->m_zipStream.avail_out;
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
	}
}

vsFile::~vsFile()
{
	if ( m_mode == MODE_WriteCompressed )
	{
		int zipBufferSize = 1024 * 100;
		char zipBuffer[zipBufferSize];
		m_zipData->m_zipStream.avail_in = 0;
		m_zipData->m_zipStream.next_in = NULL;
		do
		{
			m_zipData->m_zipStream.avail_out = zipBufferSize;
			m_zipData->m_zipStream.next_out = (Bytef*)zipBuffer;
			int ret = deflate(&m_zipData->m_zipStream, Z_FINISH);
			vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered in destructor");

			int compressedBytes = zipBufferSize - m_zipData->m_zipStream.avail_out;
			if ( compressedBytes > 0 )
				_WriteBytes(zipBuffer, compressedBytes);
				// PHYSFS_write( m_file, zipBuffer, 1, compressedBytes );

		}while( m_zipData->m_zipStream.avail_out == 0 );
		vsAssert( m_zipData->m_zipStream.avail_in == 0, "Didn't compress all the available input data?" );
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
			PHYSFS_write( m_file, m_store->GetReadHead(), 1, m_store->BytesLeftForReading() );
			m_store->Clear();
		}
	}
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
		PHYSFS_sint64 atime = PHYSFS_getLastModTime((m_dirName + a).c_str());
		PHYSFS_sint64 btime = PHYSFS_getLastModTime((m_dirName + b).c_str());
		return ( atime > btime );
	}
};

int
vsFile::DirectoryContents( vsArray<vsString>* result, const vsString &dirName ) // static method
{
    result->Clear();
	char **files = PHYSFS_enumerateFiles(dirName.c_str());
	char **i;
	std::vector<char*> s;
	for (i = files; *i != NULL; i++)
		s.push_back(*i);

	std::sort(s.begin(), s.end(), sortFilesByModificationDate(dirName));

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
				PHYSFS_getWriteDir(), PHYSFS_getDirSeparator(), writeDirectoryName.c_str(), PHYSFS_getLastError()) );
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
vsFile::Record_Binary( vsRecord *r )
{
	// utility function;  actually, we do this from the Record side.
	if ( m_mode == MODE_Write || m_mode == MODE_WriteCompressed )
		r->SaveBinary(this);
	else
		r->LoadBinary(this);
	return false;
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
	if ( m_mode == MODE_Write || m_mode == MODE_WriteCompressed )
	{
		s->Rewind();
		PHYSFS_write( m_file, s->GetReadHead(), 1, s->Length() );
	}
	else
	{
		s->Rewind();
		if ( m_file )
		{
			PHYSFS_sint64 n;
			n = PHYSFS_read( m_file, s->GetWriteHead(), 1, s->BufferLength() );
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
vsFile::_WriteBytes( void* bytes, size_t byteCount )
{
	vsAssert( m_mode == MODE_Write || m_mode == MODE_WriteCompressed,
			"Trying to write but we're not in write mode??" );

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

			PHYSFS_write( m_file, m_store->GetReadHead(), 1, m_store->BytesLeftForReading() );
			m_store->Clear();
			byteCount -= bytesWeCanWrite;
		}
	}
}

void
vsFile::StoreBytes( vsStore *s, size_t bytes )
{
	if ( m_mode == MODE_Write )
	{
		_WriteBytes(s->GetReadHead(), s->BytesLeftForReading());
		// PHYSFS_write( m_file, s->GetReadHead(), 1, vsMin(bytes, s->BytesLeftForReading()) );
	}
	else if ( m_mode == MODE_WriteCompressed )
	{
		int zipBufferSize = 1024 * 100;
		char zipBuffer[zipBufferSize];
		m_zipData->m_zipStream.avail_in = bytes;
		m_zipData->m_zipStream.next_in = (Bytef*)s->GetReadHead();
		do
		{
			m_zipData->m_zipStream.avail_out = zipBufferSize;
			m_zipData->m_zipStream.next_out = (Bytef*)zipBuffer;
			int ret = deflate(&m_zipData->m_zipStream, Z_NO_FLUSH);
			vsAssert(ret != Z_STREAM_ERROR, "Zip State not clobbered");

			int compressedBytes = zipBufferSize - m_zipData->m_zipStream.avail_out;
			if ( compressedBytes > 0 )
				_WriteBytes(zipBuffer, compressedBytes);
				// PHYSFS_write( m_file, zipBuffer, 1, compressedBytes );

		}while( m_zipData->m_zipStream.avail_out == 0 );
		vsAssert( m_zipData->m_zipStream.avail_in == 0, "Didn't compress all the available input data?" );
	}
	else
	{
		size_t actualBytes = vsMin(bytes, m_store->BytesLeftForReading());
		s->WriteBuffer( m_store->GetReadHead(), actualBytes );
		m_store->AdvanceReadHead(actualBytes);
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
	vsString filename(filename_in);
	vsString dir = PHYSFS_getRealDir( filename.c_str() );
	return dir + "/" + filename;
#endif
}


bool
vsFile::AtEnd()
{
	return m_store->BytesLeftForReading() == 0; // !m_file || PHYSFS_eof( m_file );
}

