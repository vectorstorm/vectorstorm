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

class vsRecord;
class vsStore;

class vsFile
{
public:
	enum Mode
	{
		MODE_Read,
		MODE_Write,

		MODE_MAX
	};

private:
	FILE *		m_file;

	Mode		m_mode;

	size_t		m_length;

public:

			// In general, files should be opened by creating an vsFile;  the vsFile class automatically deals with finding where the file is
			// located on the particular platform and build.  However, if a file needs to be opened by a piece of middleware (such as SDL_Mixer,
			// for example), then you need to use vsFile::GetFullFilename() to convert from a filename to the correct path to reach that file on
			// our current platform.
	static vsString	GetFullFilename(const vsString &filename_in);

				vsFile( const vsString &filename, vsFile::Mode mode = MODE_Read );
	virtual		~vsFile();

	size_t		GetLength() { return m_length; }

	static bool	Exists( const vsString &filename );	// returns true if the requested file exists.  (Useful for prefs files, which may or may not exist)
	static bool Delete( const vsString &filename );

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
