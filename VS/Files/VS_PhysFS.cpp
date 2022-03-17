/*
 *  VS_PhysFS.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/02/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_PhysFS.h"

// PhysFS made a bunch of changes to its interface in version 2.1.0.  Let's
// check what version of PhysFS we're compiling against and create the necessary
// glue code to make everything work.
//
// Our goal here is for the code in this file to exclusively be using the
// MODERN interfaces (i.e.: v2.1+), but to provide glue code so that those
// running older versions of the library (i.e.: me, in all my Steam builds) can
// still compile and run.

#if PHYSFS_VER_MAJOR < 2 || (PHYSFS_VER_MAJOR == 2 && PHYSFS_VER_MINOR < 1)

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
			if ( file != nullptr )
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

	PHYSFS_ErrorCode PHYSFS_getLastErrorCode()
	{
		if ( PHYSFS_getLastError() == nullptr )
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
	return nullptr;
}

