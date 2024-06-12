/*
 *  VS_RecordReader.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 06/08/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RECORDREADER_H
#define VS_RECORDREADER_H

#include "VS_Record.h"
class vsSerialiserReadStream;

// A vsRecordReader is designed for making it easy to read records from a
// streamed data file, most notably with its support for skipping whole
// branches of a file using 'Next()' to ignore children, and 'Child()' and
// 'Parent()' to help structure walks through the hierarchy.
//
// RecordStream is designed to be opinionated, and will throw asserts if you do
// anything it doesn't like (such as walking outside the bounds of the stream)

class vsRecordReader
{
	struct InternalData;
	InternalData *m_data;

	vsSerialiserReadStream *m_stream;

	int m_level;
	int m_remainingAtThisLevel;
	// we need some sort of stack here to know where we're at in the hierarchy.

	bool m_hasValidRecord;
	vsRecord m_record;

	void _Skip( int elements );

public:
	vsRecordReader( vsSerialiserReadStream *stream );
	~vsRecordReader();

	const vsRecord& Get() const;
	vsRecord& Get();

	void SetError();

	bool HasNext() const; // is there another record at this level?

	void Next(); // 'next' will skip over a record to the next one at the same level
	int BeginChildren(); // returns number of children, 'Next' will iterate through the children
	void EndChildren();   // Done reading children
};

#endif // VS_RECORDREADER_H

