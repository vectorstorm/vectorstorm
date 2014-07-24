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
 			vsAssert( record.GetTokenCount() == 3, vsFormatString("%s preference has wrong number of arguments!", record.GetLabel().AsString().c_str() ));

			AddPreference( record.GetLabel().AsString(), record.GetToken(0).AsInteger(), record.GetToken(1).AsInteger(), record.GetToken(2).AsInteger());
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

	return NULL;
}

vsPreferenceObject *
vsPreferences::AddPreference(const vsString &label, int value, int minValue, int maxValue)
{
	vsPreferenceObject *newObj = new vsPreferenceObject;
	newObj->m_label = label;
	newObj->m_value = value;
	newObj->m_min = minValue;
	newObj->m_max = maxValue;

	newObj->Validate();

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
		record.SetTokenCount(3);
		record.GetToken(0).SetInteger(s->m_value);
		record.GetToken(1).SetInteger(s->m_min);
		record.GetToken(2).SetInteger(s->m_max);

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
		o->m_min = minValue;
		o->m_max = maxValue;

		if ( o->m_value < minValue || o->m_value > maxValue )
			o->m_value = defaultValue;		// we seem to have changed the limits for this value, such that the saved value is no longer legal.  Switch it to the new default value.

		o->Validate();
	}
	else
	{
		o = AddPreference(label, defaultValue, minValue, maxValue);
	}

	return o;
}


vsPreferenceObject::vsPreferenceObject():
	m_label("NULL"),
	m_value(0),
	m_min(0),
	m_max(0)
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

void
vsPreferenceObject::Validate()
{
	vsAssert(m_value >= m_min, "Preference value too low!");
	vsAssert(m_value <= m_max, "Preference value too high!");
}

