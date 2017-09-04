/*
 *  VS_StringTable.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 26/08/2017
 *  Copyright 2017 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_STRINGTABLE_H
#define VS_STRINGTABLE_H

#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_HashTable.h"

// A string table is just an ordered array of strings which has an optimised way of
// finding a given string.  This is used by our binary record file format, for
// building the string table that sits at the front of the binary file (so that
// "string" data embedded in the file can store an integer index into the
// string table, instead of storing multiple copies of the strings)
//
// FUTURE ME:  If you're looking for something which will handle
// translation/localisation functionality, that's not this class!  You're
// looking for the vsLocalisationTable class!

class vsStringTable
{
	class Entry
	{
	public:
		Entry(): id(0) {}
		Entry(int i): id(i) {}
		int id;
	};
	vsArray<vsString> m_strings;
	vsHashTable<Entry> m_stringIndex;

public:
	vsStringTable();

	int AddString( const vsString& string );
	int FindString( const vsString& string );

	vsArray<vsString>& GetStrings() { return m_strings; }
	// const vsArray<vsString>& GetStrings() { return m_strings; }
};

#endif // VS_STRINGTABLE_H

