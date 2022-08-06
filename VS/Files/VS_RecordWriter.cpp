/*
 *  VS_RecordWriter.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 06/08/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RecordWriter.h"
#include <stack>

struct vsRecordWriter::InternalData
{
	std::stack<int> cursor;
};


vsRecordWriter::vsRecordWriter( vsSerialiserWriteStream *stream ):
	m_stream(stream),
	m_childCount(0),
	m_level(0),
	m_remainingToWriteAtThisLevel(-1),
	m_hasValidRecord(false),
	m_first(true)
{
	m_data = new InternalData;
}

vsRecordWriter::~vsRecordWriter()
{
	if ( m_hasValidRecord )
		_WriteRecord();

	vsDelete( m_data );
}

void
vsRecordWriter::_WriteRecord()
{
	vsAssert( m_hasValidRecord, "Tried to write without a valid record??" );
	if ( m_first )
		m_record.WriteBinary_Stream_Init( m_stream, m_childCount );
	else
		m_record.WriteBinary_Stream( m_stream, m_childCount );
	m_first = false;
	m_childCount = 0;
	m_record.Init();
}

void
vsRecordWriter::Next()
{
	if ( m_hasValidRecord )
		_WriteRecord();
	m_childCount = 0;

	if ( m_level > 0 )
	{
		vsAssert( m_remainingToWriteAtThisLevel >= 0, "Trying to add a record where we didn't make space for one earlier??");
		m_remainingToWriteAtThisLevel--;

		// We need to detect whether
	}

	m_hasValidRecord = true;
}

	// As a utility, you can call 'AddWithChildren' and we'll add the whole
	// hierarchy of the vsRecord recursively, instead of only this specific
	// record itself.  Useful for when you just have a simple structure and
	// don't want to have to mess with the 'Child()' and 'Parent()' interfaces.
	// void AddWithChildren( vsRecord& r );

void
vsRecordWriter::BeginChildren(int childCount)
{
	m_childCount = childCount;
	_WriteRecord();

	m_level++;
	m_data->cursor.push( m_remainingToWriteAtThisLevel );
}
//
// void
// vsRecordWriter::EndChildren()
// {
// 	vsAssert( m_remainingToWriteAtThisLevel == 0, "Called Parent() before filling in all declared Child records??" );
// 	m_remainingChildrenToWrite = m_remainingToWriteAtThisLevel;
// 	m_remainingToWriteAtThisLevel = m_data->cursor.top();
// 	m_data->cursor.pop();
// 	m_level--;
// }
//
