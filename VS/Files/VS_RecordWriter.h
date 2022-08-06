/*
 *  VS_RecordWriter.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 06/08/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RECORDWRITER_H
#define VS_RECORDWRITER_H

#include "VS_Record.h"
class vsSerialiserReadStream;
class vsSerialiserWriteStream;

// A vsRecordWriter is designed for making it easy to write records into a
// streamed data file.
//
// vsRecordWriter is designed to be opinionated, and will throw asserts if you
// do anything it doesn't like

class vsRecordWriter
{
	struct InternalData;
	InternalData *m_data;

	vsSerialiserWriteStream *m_stream;

	vsRecord m_record;
	int m_childCount;

	int m_level;
	int m_remainingToWriteAtThisLevel;

	bool m_hasValidRecord;

	bool m_first;

	void _WriteRecord();

public:

	vsRecordWriter( vsSerialiserWriteStream *stream );
	~vsRecordWriter();

	// Move to the next record at the current level.
	void Next();

	// gets the currently in-construction record.
	vsRecord& Get();
	// utility functions
	void SetLabel( const vsString& label );

	// As a utility, you can call 'AddWithChildren' and we'll add the whole
	// hierarchy of the vsRecord recursively, instead of only this specific
	// record itself.  Useful for when you just have a simple structure and
	// don't want to have to mess with the 'Child()' and 'Parent()' interfaces.
	// void AddWithChildren( vsRecord& r );

	void BeginChildren( int childCount ); // 'child' makes it so our next added record will be a child of the most recently added record.
	void EndChildren();   // ''parent' goes up one level and invalidates the record.

};


#endif // VS_RECORDWRITER_H

