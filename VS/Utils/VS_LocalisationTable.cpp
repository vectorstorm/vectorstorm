/*
 *  VS_LocalisationTable.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_LocalisationTable.h"

#include "VS/Utils/VS_HashTable.h"


#include "VS_File.h"
#include "VS_Record.h"

static vsHashTable<vsString>	*s_localisationTable = NULL;

vsLocalisationTable::vsLocalisationTable()
{
}

vsLocalisationTable::~vsLocalisationTable()
{
}

void
vsLocalisationTable::Init()
{
	Init("English");
}

void
vsLocalisationTable::Init(const vsString &language)
{
	s_localisationTable = new vsHashTable<vsString>( 128 );

	vsString filename = vsFormatString("i18n/%s.vrt", language.c_str());

	if ( vsFile::Exists(filename) )
	{
		vsFile table(filename);

		vsString str;
		vsRecord r;

		while( table.Record(&r) )
		{
			if ( r.GetTokenCount() > 0 )
			{
				vsString label = r.GetLabel().AsString();
				vsString string = r.GetToken(0).AsString();

				s_localisationTable->AddItemWithKey(string, label);
			}
		}
	}
}

void
vsLocalisationTable::Deinit()
{
	vsDelete( s_localisationTable );
}

vsString
vsLocalisationTable::GetTranslation( const vsString &key )
{
	vsString *str = s_localisationTable->FindItem(key);

	if ( str )
	{
		return *str;
	}
	else
	{
		return vsFormatString("<<%s>>", key.c_str());
	}
}

