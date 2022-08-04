/*
 *  VS_RecordStream.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 04/08/2022
 *  Copyright 2022 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RecordStream.h"
#include <stack>

struct vsRecordStream::InternalData
{
	std::stack<int> cursor;
};

vsRecordStream::vsRecordStream( vsSerialiserReadStream *stream ):
	m_stream(stream),
	m_level(0),
	m_remainingAtThisLevel(-1)
{
	m_data = new InternalData;
	m_record.LoadBinary_Stream_Init(m_stream);
	m_hasValidRecord = true;
}

vsRecordStream::~vsRecordStream()
{
	vsDelete( m_data );
}

const vsRecord&
vsRecordStream::Get() const
{
	vsAssert( m_hasValidRecord, "Tried to get record while in invalid state??" );
	return m_record;
}

vsRecord&
vsRecordStream::Get()
{
	vsAssert( m_hasValidRecord, "Tried to get record while in invalid state??" );
	return m_record;
}

bool
vsRecordStream::HasNext() const
{
	return (m_remainingAtThisLevel > 0);
}

void
vsRecordStream::Next()
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

void
vsRecordStream::Child()
{
	vsAssert( m_hasValidRecord, "Trying to go down in an invalid record??" );
	// vsAssert( m_record.GetChildCount() > 0, "Trying to go down in an empty record??" );

	m_data->cursor.push( m_remainingAtThisLevel );
	m_remainingAtThisLevel = m_record.GetChildCount();
	m_level++;
	m_hasValidRecord = false;
}

void
vsRecordStream::Parent()
{
	vsAssert( m_level > 0, "Trying to go up from the top of a record??" );

	_Skip( m_remainingAtThisLevel );

	m_remainingAtThisLevel = m_data->cursor.top();
	m_data->cursor.pop();
	m_level--;
	m_hasValidRecord = false;
}

void
vsRecordStream::_Skip( int elements )
{
	int elementsToSkip = m_remainingAtThisLevel;
	for ( int i = 0; i < elementsToSkip; i++ )
	{
		m_record.LoadBinary_Stream(m_stream);
		elementsToSkip += m_record.GetChildCount();
	}
}

