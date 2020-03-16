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

namespace {
	void LoadTranslationsIntoHash( vsHashTable<vsString> *ht, const vsString& language )
	{
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

					ht->AddItemWithKey(string, label);
				}
			}
		}
	}
};

static vsHashTable<vsString>	*s_localisationTable = NULL;
static vsHashTable<vsString>	*s_fallbackLocalisationTable = NULL;

vsLocalisationTable::vsLocalisationTable()
{
}

vsLocalisationTable::~vsLocalisationTable()
{
	vsDelete( s_localisationTable );
	vsDelete( s_fallbackLocalisationTable );
}

void
vsLocalisationTable::Init()
{
	Init("English");
}

void
vsLocalisationTable::Init(const vsString &language)
{
	s_localisationTable = new vsHashTable<vsString>( 512 );
	LoadTranslationsIntoHash( s_localisationTable, language );

	if ( !s_fallbackLocalisationTable )
	{
		if ( vsFile::Exists("i18n/english.vrt") )
		{
			s_fallbackLocalisationTable = new vsHashTable<vsString>( 512 );
			LoadTranslationsIntoHash( s_fallbackLocalisationTable, "english" );
		}
	}
}

void
vsLocalisationTable::Deinit()
{
	vsDelete( s_localisationTable );
}

void
vsLocalisationTable::AddKey( const vsString& key, const vsString& translation )
{
	s_localisationTable->AddItemWithKey(translation, key);
}

vsString
vsLocalisationTable::GetTranslation( const vsString &key )
{
	vsString *str = s_localisationTable->FindItem(key);

	if ( str )
	{
		return *str;
	}
	else if ( s_fallbackLocalisationTable )
	{
		str = s_fallbackLocalisationTable->FindItem(key);
		if ( str )
			return vsFormatString("$%s$", *str);
	}
	return vsFormatString("<<%s>>", key.c_str());
}

