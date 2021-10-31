/*
 *  VS_SingletonManager.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_SingletonManager.h"

vsSingletonManager *	vsSingletonManager::s_instance = nullptr;

vsSingletonManager::vsSingletonManager():
	m_table(16)
{
	s_instance = this;
}

vsSingletonManager::~vsSingletonManager()
{
	s_instance = nullptr;
}

void
vsSingletonManager::RegisterSingleton(void *singleton, const vsString &name)
{
	m_table.AddItemWithKey(singleton, name);
}

void
vsSingletonManager::UnregisterSingleton(void *singleton, const vsString &name)
{
	m_table.RemoveItemWithKey(singleton, name);
}

void *
vsSingletonManager::GetSingleton(const vsString &name)
{
	void **singletonAddressPointer = (void **)m_table.FindItem(name);
	if ( singletonAddressPointer )
	{
		return *singletonAddressPointer;
	}
	
	return nullptr;
}

