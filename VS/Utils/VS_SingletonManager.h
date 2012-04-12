/*
 *  VS_SingletonManager.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SINGLETON_MANAGER_H
#define VS_SINGLETON_MANAGER_H

#include "VS_HashTable.h"

// The behaviour of static members inside templated classes inside libraries is undefined by
// the C standard, and some compilers (most notably GCC compilers before the 3.0 series)
// give each library its own static members for these templated classes which are referenced
// both by library code and by game code.  To get around this problem, we have the
// vsSingletonManager, which (although using the singleton pattern), is not templated via
// the vsSingleton template, and so doesn't have this static data problem.
//
// The vsSingletonManager stores a hash table of currently existing singletons.  vsSingleton
// talks to it when people try to access an apparently non-existant singleton, to check whether
// perhaps this build has been compiled by a compiler which has provided separate static variable
// space for the library than for the game code.. and will retrieve the singleton pointers so that
// the same pointer can be set in both sets of static data, if that turns out to be the case.
//
// Note that this requires RTTI to be enabled, in order to work, as we use the requested singleton's
// RTTI type name as our key into the hash table.

class vsSingletonManager
{
	static vsSingletonManager *		s_instance;
	
	vsHashTable<void *>				m_table;
public:
	
	vsSingletonManager();
	~vsSingletonManager();
	
	void		RegisterSingleton(void *singleton, const vsString &name);
	void		UnregisterSingleton(void *singleton, const vsString &name);
	void *		GetSingleton(const vsString &name);
	
	static vsSingletonManager *	Instance() { return s_instance; }
};

#endif // VS_SINGLETON_MANAGER_H

