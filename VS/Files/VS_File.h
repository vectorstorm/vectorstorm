/*
 *  FS_File.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef FS_FILE_H
#define FS_FILE_H

struct PHYSFS_File;

class vsRecord;
class vsStore;

#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_String.h"
#include <cstdint>

struct zipdata;

class vsFile
{
public:
	enum Mode
	{
		MODE_Read,          // open an existing file and read from it.
		MODE_Write,         // overwrite an existing file.
		MODE_WriteDirectly, // actually, 'Write' writes into a temporary file and overwrites the existing file when you destroy the vsFile object.  If you *actually* want to write directly into a file, use this mode instead.

		MODE_ReadCompressed, // open an existing file and read from it, INFLATEd
		MODE_WriteCompressed, // overwrite an existing file, DEFLATEd

		MODE_ReadCompressed_Progressive, // open an existing file and read from it, decompressing into temporary buffers as we go.  We only support a limited set of read operations, since we won't have the whole thing in memory at once!

		MODE_MAX
	};

	// NB: If you add a new error code, add a string for it to _LogError() as well!
	enum Error
	{
		ERROR_Ok,
		ERROR_Serialiser,
		ERROR_Phys_Other, // unknown other error
		ERROR_Phys_Memory,
		ERROR_Phys_NotInitialised,
		ERROR_Phys_AlreadyInitialised,
		ERROR_Phys_ArgVZeroIsNull,
		ERROR_Phys_Unsupported,
		ERROR_Phys_PastEOF,
		ERROR_Phys_FilesStillOpen,
		ERROR_Phys_InvalidArgument,
		ERROR_Phys_NotMounted,
		ERROR_Phys_NoSuchPath,
		ERROR_Phys_SymlinkForbidden,
		ERROR_Phys_NoWriteDir,
		ERROR_Phys_OpenForReading,
		ERROR_Phys_OpenForWriting,
		ERROR_Phys_NotAFile,
		ERROR_Phys_ReadOnly,
		ERROR_Phys_Corrupt,
		ERROR_Phys_SymlinkLoop,
		ERROR_Phys_IO,
		ERROR_Phys_Permission,
		ERROR_Phys_NoSpace,
		ERROR_Phys_BadFilename,
		ERROR_Phys_Busy,
		ERROR_Phys_DirNotEmpty,
		ERROR_Phys_OS,
		ERROR_ZLib_Stream,
		ERROR_ZLib_Data,
		ERROR_ZLib_Memory,
		ERROR_ZLib_Version,
	};

private:
	vsString m_filename;
	vsString m_tempFilename;
	PHYSFS_File *	m_file;

	vsStore *m_compressedStore;
	vsStore *m_store;
	struct zipdata *m_zipData;
	bool m_ok;
	Error m_error;

	Mode		m_mode;

	size_t		m_length;
	bool m_moveOnDestruction;

	void _DoWriteLiteralBytes( const void* bytes, size_t byteCount );

	void _WriteBytes( const void* bytes, size_t byteCount );
	void _WriteFinalBytes_Buffered( const void* bytes, size_t byteCount );

	// do some processing of file compression.
	void _PumpCompression( const void* bytes, size_t byteCount, bool finish );

	bool _IsWrite() const;

	bool _ZLibIsOkay( const char* context, int retval );
	bool _PhysFSError( const char* context, int retval );

	void _LogError( const char* context ) const;

public:

			// In general, files should be opened by creating an vsFile;  the vsFile class automatically deals with finding where the file is
			// located on the particular platform and build.  However, if a file needs to be opened by a piece of middleware (such as SDL_Mixer,
			// for example), then you need to use vsFile::GetFullFilename() to convert from a filename to the correct path to reach that file on
			// our current platform.
	static vsString	GetFullFilename(const vsString &filename_in);

	vsFile( const vsString &filename, vsFile::Mode mode = MODE_Read );
	virtual ~vsFile();

	bool IsOK() { return m_ok; }
	void SetError( vsFile::Error err ) { m_ok = false; m_error = err; }
	Error GetError() { return m_error; } // if we're not ok, this is an english string explaining why not.  (TODO:  Error codes instead!)

	size_t GetLength() { return m_length; }
	const vsString& GetFilename() const { return m_filename; }

	struct DiskStats
	{
		uintmax_t freeBytes;      // bytes without things stored in them.
		uintmax_t availableBytes; // bytes which *we* can write into.  (may be lower than 'freeBytes'!)
		uintmax_t capacityBytes;  // total bytes on the disk.
	};

	static void LogDiskStats(); // logs information about our disk.
	static DiskStats GetDiskStats(); // returns how many bytes we can write into our write directory.

	static bool	Exists( const vsString &filename );	// returns true if the specified file exists.
	static bool	DirectoryExists( const vsString &dirName );	// returns true if the specified directory exists.
	// Delete functions return TRUE on success.
	static bool Delete( const vsString &filename ); // will delete a FILE.
	static bool Copy( const vsString &from, const vsString &to ); // will delete a FILE.
	static bool Move( const vsString &from, const vsString &to ); // will move a FILE.
	static bool DeleteEmptyDirectory( const vsString &filename ); // will delete a DIRECTORY, but only if it's empty.
	static bool DeleteDirectory( const vsString &filename ); // will delete a directory, even if it contains files or more directories.
	static bool MoveDirectory( const vsString& from, const vsString& to ); // will move a DIRECTORY from one point in the WRITE DIRECTORY to another.
	static bool MoveDirectoryContents( const vsString& from, const vsString& to ); // will copy files from a DIRECTORY from one point in the WRITE DIRECTORY to another.

	static vsString GetExtension( const vsString &filename ); // returns the 'extension' of the listed file or vsEmptyString if there is none.
	static vsString GetFileName( const vsString &filename );  // returns the full 'file name' of the listed file, including extension but excluding directories.
	static vsString GetBaseName( const vsString &filename );  // returns the 'base name' of the listed file (no directory, no extension).
	static vsString GetDirectory( const vsString &filename ); // returns the 'directory' component of the listed file, or "./" if none.
															  //
	static bool IsCompressedFileValid( const vsString& filename, vsString& outError );

	// DirectoryContents returns a list of FILES AND DIRECTORIES inside this
	// directory.  It is your responsibility to check for each one whether it
	// is the type of object you were looking for.
	static int DirectoryContents( vsArray<vsString> *result, const vsString &dirName );

	// DirectoryFiles returns a list of FILES inside this directory.
	static int DirectoryFiles( vsArray<vsString> *result, const vsString &dirName );

	// DirectoryFiles returns a list of DIRECTORIES inside this directory.
	static int DirectoryDirectories( vsArray<vsString> *result, const vsString &dirName );

	static void EnsureWriteDirectoryExists( const vsString &writeDirectoryName );

	bool		PeekRecord( vsRecord *record );	// ONLY FOR READ OPERATIONS.  Peeks at next record, without advancing.
	bool		Record( vsRecord *record );		// returns true if we found or successfully wrote another record
	bool		PeekLine( vsString *line );
	bool		ReadLine( vsString *line );

	bool		Record_Binary( vsRecord *record );		// returns true if we found or successfully wrote another record

	void		Rewind();

	void		Store( vsStore *store );		// read/write this raw data directly.  STORE IS REWOUND BEFORE READ/WRITE
	void		StoreBytes( vsStore *store, size_t bytes );	// how many bytes to read/write into/out of this store.  STORE IS NOT REWOUND BEFORE READ/WRITE
	void		PeekBytes( vsStore *store, size_t bytes );	// ONLY IN READ OPERATIONS.  Peek the requested number of bytes.
	void		ConsumeBytes( size_t bytes ); // ONLY IN READ OPERATIONS.  Count this many bytes as having been read.  (Usually used in combination with the above)

	int			ReadBytes( void* data, size_t bytes ); // this is a more direct version  of aa Read operation.  Will assert if we're not in a Read mode.
	void		WriteBytes( const void* data, size_t bytes ); // this is a more direct version of 'Store'.  Will assert if we're not in a Write mode.

	void		FlushBufferedWrites();
	/*  These functions are probably deprecated;  use vsRecord objects instead!
	 *
	vsString	ReadLabel();
	float		ReadFloat();
	vsVector2D	ReadVector();
	vsColor		ReadColor();
	 */

	bool		AtEnd();

	// the open failure handler should return 'true' if we should continue or 'false' if we should assert.
	typedef bool (*openFailureHandler)(const vsString& filename, const vsString& errorMessage);
	static void SetFileOpenFailureHandler( openFailureHandler handler );
};

#endif // FS_FILE_H
