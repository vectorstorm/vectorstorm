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

class vsFile
{
public:
	enum Mode
	{
		MODE_Read,          // open an existing file and read from it.
		MODE_Write,         // overwrite an existing file.
		MODE_WriteDirectly, // actually, 'Write' writes into a temporary file and overwrites the existing file when you destroy the vsFile object.  If you *actually* want to write directly into a file, use this mode instead.

		MODE_MAX
	};

private:
	vsString m_filename;
	vsString m_tempFilename;
	PHYSFS_File *	m_file;
	vsStore *m_store;

	Mode		m_mode;

	size_t		m_length;
	bool m_moveOnDestruction;

public:

			// In general, files should be opened by creating an vsFile;  the vsFile class automatically deals with finding where the file is
			// located on the particular platform and build.  However, if a file needs to be opened by a piece of middleware (such as SDL_Mixer,
			// for example), then you need to use vsFile::GetFullFilename() to convert from a filename to the correct path to reach that file on
			// our current platform.
	static vsString	GetFullFilename(const vsString &filename_in);

				vsFile( const vsString &filename, vsFile::Mode mode = MODE_Read );
	virtual		~vsFile();

	size_t		GetLength() { return m_length; }

	static bool	Exists( const vsString &filename );	// returns true if the specified file exists.
	static bool	DirectoryExists( const vsString &dirName );	// returns true if the specified directory exists.
	// Delete functions return TRUE on success.
	static bool Delete( const vsString &filename ); // will delete a FILE.
	static bool Copy( const vsString &from, const vsString &to ); // will delete a FILE.
	static bool Move( const vsString &from, const vsString &to ); // will delete a FILE.
	static bool DeleteEmptyDirectory( const vsString &filename ); // will delete a DIRECTORY, but only if it's empty.
	static bool DeleteDirectory( const vsString &filename ); // will delete a directory, even if it contains files or more directories.

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

	void		Rewind();

	void		Store( vsStore *store );		// read/write this raw data directly.
	void		StoreBytes( vsStore *store, size_t bytes );	// how many bytes to read/write into/out of this store

	/*  These functions are probably deprecated;  use vsRecord objects instead!
	 *
	vsString	ReadLabel();
	float		ReadFloat();
	vsVector2D	ReadVector();
	vsColor		ReadColor();
	 */

	bool		AtEnd();
};

#endif // FS_FILE_H
