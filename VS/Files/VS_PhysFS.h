/*
 *  VS_PhysFS.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/02/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_PHYSFS_H
#define VS_PHYSFS_H

#include <physfs.h>

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
#define PHYSFS_unmount(x) PHYSFS_removeFromSearchPath(x)

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

	int PHYSFS_stat( const char* filename, PHYSFS_Stat* stat );

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
	PHYSFS_ErrorCode PHYSFS_getLastErrorCode();

	const char* PHYSFS_getErrorByCode(int code);

#endif

// utility function, mimicking the behaviour of deprecated PHYSFS_getLastError().
const char* PHYSFS_getLastErrorString();

#endif // VS_PHYSFS_H

