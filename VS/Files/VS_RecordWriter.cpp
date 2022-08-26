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
	m_hasValidRecord(false)
{
	m_record.WriteBinary_Stream_Init( m_stream );

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

	m_record.WriteBinary_Stream( m_stream, m_childCount );
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

vsRecord&
vsRecordWriter::Get()
{
	vsAssert( m_hasValidRecord, "Tried to call Get() on vsRecordWriter which doesn't have a record??" );
	return m_record;
}

	// As a utility, you can call 'AddWithChildren' and we'll add the whole
	// hierarchy of the vsRecord recursively, instead of only this specific
	// record itself.  Useful for when you just have a simple structure and
	// don't want to have to mess with the 'Child()' and 'Parent()' interfaces.
	// void AddWithChildren( vsRecord& r );

void
vsRecordWriter::BeginChildren(int childCount)
{
	vsAssert( m_hasValidRecord, "Beginning children when we don't have a valid record??" );

	m_childCount = childCount;
	_WriteRecord();
	m_childCount = 0;
	m_hasValidRecord = false;

	m_level++;
	m_data->cursor.push( m_remainingToWriteAtThisLevel );
	m_remainingToWriteAtThisLevel = childCount;
}

void
vsRecordWriter::EndChildren()
{
	if ( m_hasValidRecord )
		_WriteRecord(); // write out the last child.

	vsAssert( m_level > 0, "Called EndChildren() past top of record stack" );
	vsAssert( m_remainingToWriteAtThisLevel == 0, "Called EndChildren() before filling in all declared Child records??" );
	m_remainingToWriteAtThisLevel = m_data->cursor.top();
	m_data->cursor.pop();
	m_level--;
	m_hasValidRecord = false;
}

void
vsRecordWriter::SetLabel( const vsString& label )
{
	vsAssert( m_hasValidRecord, "SetLabel() called on non-valid record?" );
	m_record.SetLabel(label);
}

void
vsRecordWriter::SetInt( int value )
{
	vsAssert( m_hasValidRecord, "SetInt() called on non-valid record?" );
	m_record.SetInt(value);
}

void
vsRecordWriter::SetFloat(float value)
{
	vsAssert( m_hasValidRecord, "SetFloat() called on non-valid record?" );
	m_record.SetFloat(value);
}

void
vsRecordWriter::SetTokenCount( int count )
{
	vsAssert( m_hasValidRecord, "SetTokenCount() called on non-valid record?" );
	m_record.SetTokenCount(count);
}

vsToken&
vsRecordWriter::GetToken( int i )
{
	vsAssert( m_hasValidRecord, "GetToken() called on non-valid record?" );
	return m_record.GetToken(i);
}

