/*
 *  VS_RecordReader.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 06/08/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RecordReader.h"

#include <stack>

struct vsRecordReader::InternalData
{
	std::stack<int> cursor;
};

vsRecordReader::vsRecordReader( vsSerialiserReadStream *stream ):
	m_stream(stream),
	m_level(0),
	m_remainingAtThisLevel(-1)
{
	m_data = new InternalData;
	m_record.LoadBinary_Stream_Init(m_stream);
	m_hasValidRecord = true;
}

vsRecordReader::~vsRecordReader()
{
	vsDelete( m_data );
}

const vsRecord&
vsRecordReader::Get() const
{
	vsAssert( m_hasValidRecord, "Tried to get record while in invalid state??" );
	return m_record;
}

vsRecord&
vsRecordReader::Get()
{
	vsAssert( m_hasValidRecord, "Tried to get record while in invalid state??" );
	return m_record;
}

bool
vsRecordReader::HasNext() const
{
	return (m_remainingAtThisLevel > 0);
}

void
vsRecordReader::Next()
{
	if ( m_level != 0 )
	{
		vsAssert( m_remainingAtThisLevel > 0, "Overflow siblings" );
		m_remainingAtThisLevel--;
	}

	// if we're going 'next' we're skipping over any of this element's children
	if ( m_hasValidRecord && m_record.GetChildCount() )
		_Skip( m_record.GetChildCount() );

	m_record.LoadBinary_Stream(m_stream);
	m_hasValidRecord = true;
}

int
vsRecordReader::BeginChildren()
{
	vsAssert( m_hasValidRecord, "Trying to go down in an invalid record??" );
	// vsAssert( m_record.GetChildCount() > 0, "Trying to go down in an empty record??" );

	m_data->cursor.push( m_remainingAtThisLevel );
	m_remainingAtThisLevel = m_record.GetChildCount();
	m_level++;
	m_hasValidRecord = false;

	return m_remainingAtThisLevel;
}

void
vsRecordReader::EndChildren()
{
	vsAssert( m_level > 0, "Trying to go up from the top of a record??" );

	if ( m_remainingAtThisLevel )
		vsLog("Skipped %d records at this level", m_remainingAtThisLevel);

	_Skip( m_remainingAtThisLevel );

	m_remainingAtThisLevel = m_data->cursor.top();
	m_data->cursor.pop();
	m_level--;
	m_hasValidRecord = false;
}

void
vsRecordReader::_Skip( int elements )
{
	int elementsToSkip = m_remainingAtThisLevel;
	for ( int i = 0; i < elementsToSkip; i++ )
	{
		m_record.LoadBinary_Stream(m_stream);
		elementsToSkip += m_record.GetChildCount();
	}
}

