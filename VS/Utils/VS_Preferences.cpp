/*
 *  VS_Preferences.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Preferences.h"

#include "VS_File.h"
#include "VS_Record.h"

vsPreferences::vsPreferences(const vsString &filename_in)
{
	vsString filename = vsFormatString("%s.prefs",filename_in.c_str());
	m_filename = filename;

	m_preferenceList = new vsPreferenceObject;

	if ( vsFile::Exists(filename) )	// if the file exists, then read the current pref values out of it.
	{
		vsRecord record;
		vsFile prevsFile(filename);

		while( prevsFile.Record(&record) )
		{
			if ( record.GetTokenCount() < 1 )
				vsLog("%s preference has wrong number of arguments!", record.GetLabel().AsString().c_str() );
			else
				AddPreference( record.GetLabel().AsString(), record.GetToken(0).AsInteger() );
		}
	}
}

vsPreferences::~vsPreferences()
{
	vsPreferenceObject *s = m_preferenceList->m_next;
	vsPreferenceObject *next;
	while ( s != m_preferenceList )
	{
		next = s->m_next;
		s->Extract();
		delete s;
		s = next;
	}

	delete m_preferenceList;
}

vsPreferenceObject *
vsPreferences::FindPreference(const vsString &label)
{
	vsPreferenceObject *s = m_preferenceList->m_next;

	while ( s != m_preferenceList )
	{
		if ( s->m_label == label )
			return s;
		s = s->m_next;
	}

	return nullptr;
}

vsPreferenceObject *
vsPreferences::AddPreference(const vsString &label, int value)
{
	vsPreferenceObject *newObj = new vsPreferenceObject;
	newObj->m_label = label;
	newObj->m_value = value;

	m_preferenceList->Insert(newObj);

	return newObj;
}

void
vsPreferences::Save()
{
	vsRecord record;
	vsFile prevsFile(m_filename, vsFile::MODE_Write);

	vsPreferenceObject *s = m_preferenceList->m_next;

	while( s != m_preferenceList )
	{
		record.Init();

		record.SetLabel( s->m_label );
		record.SetTokenCount(1);
		record.GetToken(0).SetInteger(s->m_value);

		prevsFile.Record(&record);

		s = s->m_next;
	}
}

vsPreferenceObject *
vsPreferences::GetPreference(const vsString &label, int defaultValue, int minValue, int maxValue)
{
	// first, let's find the relevant preference.

	vsPreferenceObject *o = FindPreference(label);
	if ( o )
	{
		o->m_value = vsClamp( minValue, o->m_value, maxValue );
	}
	else
	{
		o = AddPreference(label, defaultValue);
	}

	return o;
}


vsPreferenceObject::vsPreferenceObject():
	m_label("nullptr"),
	m_value(0)
{
	m_next = this;
	m_prev = this;
}

void
vsPreferenceObject::Insert( vsPreferenceObject *next )
{
	next->m_next = this;
	next->m_prev = m_prev;

	m_prev->m_next = next;
	m_prev = next;
}

void
vsPreferenceObject::Extract()
{
	m_next->m_prev = m_prev;
	m_prev->m_next = m_next;

	m_next = m_prev = this;
}

