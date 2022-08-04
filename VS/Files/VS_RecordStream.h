/*
 *  VS_RecordStream.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 04/08/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RECORDSTREAM_H
#define VS_RECORDSTREAM_H

#include "VS_Record.h"
class vsSerialiserReadStream;

// A vsRecordStream is designed for making it easy to read records from a
// streamed data file, most notably with its support for skipping whole
// branches of a file using 'Next()' to ignore children, and 'Child()' and
// 'Parent()' to help structure walks through the hierarchy.
//
// RecordStream is designed to be opinionated, and will throw asserts if you do
// anything it doesn't like (such as walking outside the bounds of the stream)

class vsRecordStream
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
	vsRecordStream( vsSerialiserReadStream *stream );
	~vsRecordStream();

	const vsRecord& Get() const;
	vsRecord& Get();

	bool HasNext() const; // is there another record at this level?
	void Next(); // 'next' will skip over a record to the next one at the same level
	void Child(); // 'child' goes to before the first child of the current record.
	void Parent();   // ''parent' goes up one level and invalidates the record.
};

/*
 *
 *  a { b { c { } d { } } e { } } f {}
 *
 * Next() -> a
 * Next() -> f
 *
 *   OR
 *
 * Next() -> a
 * Down() -> b
 * Down() -> c
 * Up() -> e
 * Up()
 */

#endif // VS_RECORDSTREAM_H

