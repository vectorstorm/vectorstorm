/*
 *  VS_Archive.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/2023
 *  Copyright 2023 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_ARCHIVE_H
#define VS_ARCHIVE_H

class vsArchive
{
	// initial header.
	struct Header
	{
		uint32_t headerSize;
		uint32_t version;
	};
	// Table of contents starts immediately after the header.
	struct Toc
	{
		uint32_t fileTableStartByte; // relative to start of file
		uint32_t fileDataStartByte;  // relative to start of file

		uint32_t fileTableCount;
	}
	// File table entries start at the position specified in the Toc.
	struct FileTableEntry
	{
		vsString filename;
		uint32_t fileStartOffset; // relative to the fileDataStartByte
		uint32_t fileSize;        // in bytes
		uint8_t flags;
	};

	FILE* m_file;
	Header m_header;
	vsArray<FileTableEntry> m_fileTable;

public:

	vsArchive( const vsString& filename );
	~vsArchive();


};

#endif // VS_ARCHIVE_H

