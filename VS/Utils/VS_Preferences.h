/*
 *  VS_Preferences.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_PREFERENCES_H
#define VS_PREFERENCES_H

class vsPreferenceObject;

class vsPreferences
{
protected:
	
	vsPreferenceObject *	m_preferenceList;
	
	vsString				m_filename;
	
	
	vsPreferenceObject *	AddPreference(const vsString &label, int defaultValue, int minValue, int maxValue);
	vsPreferenceObject *	FindPreference(const vsString &label);
	
public:
	
							vsPreferences(const vsString &filename);
							~vsPreferences();
	
	vsPreferenceObject *	GetPreference(const vsString &label, int defaultValue, int minValue, int maxValue);
	
	void					Save();			// save the currently registered preference objects
};

class vsPreferenceObject
{
public:
	
	vsString				m_label;
	int						m_value;
	
	int						m_min;
	int						m_max;
	
	vsPreferenceObject *	m_next;
	vsPreferenceObject *	m_prev;
	
	vsPreferenceObject();
	
	void Insert( vsPreferenceObject *other );
	void Extract();
	
	int  GetValue() { return m_value; }
	void SetValue(int value) { m_value = value; }
	
	void Validate();
};


#endif // VS_PREFERENCES_H
