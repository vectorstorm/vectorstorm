/*
 *  VS_StringTable.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 26/08/2017
 *  Copyright 2017 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_StringTable.h"

vsStringTable::vsStringTable():
	m_stringIndex(256)
{
}

int
vsStringTable::AddString( const vsString& string )
{
	// already in the table?
	Entry* indexPtr = m_stringIndex.FindItem(string);
	if ( indexPtr )
		return indexPtr->id;

	// add to the table!
	int index = m_strings.ItemCount();
	m_stringIndex[string] = index;
	m_strings.AddItem(string);
	return index;
}

int
vsStringTable::FindString( const vsString& string )
{
	// already in the table?
	Entry* indexPtr = m_stringIndex.FindItem(string);
	if ( indexPtr )
		return indexPtr->id;

	vsAssert(indexPtr, vsFormatString("Requested string '%s' not in string table", string.c_str()));
	return 0;
}

