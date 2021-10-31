/*
 *  MIX_Singleton.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 31/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef MIX_SINGLETON_H
#define MIX_SINGLETON_H

#include <assert.h>
#include <typeinfo>
#include "VS/Utils/VS_SingletonManager.h"
#include "VS/Utils/VS_Demangle.h"

/** Base mix-in class for singleton types.
*
*  A singleton is a type that only allows a single instance.
*
*  YourType must derive from vsSingleton<YourType>.
*
*\ingroup vs_types_mixins
*/
template <typename T>
class vsSingleton
{
public:
	/** Default constructor. Asserts if not the only instance. */
    vsSingleton()
{
	vsAssert( !s_instance, "More than one instance?" );
	s_instance = (T*)(this);

	if ( vsSingletonManager::Instance() )
	{
		vsSingletonManager::Instance()->RegisterSingleton(s_instance, typeid(s_instance).name());
	}
}

/** Default destructor. Asserts if not destructing the expected only instance. */
~vsSingleton()
{
	vsAssert( s_instance == (T*)(this), "More than one instance?" );

	if ( vsSingletonManager::Instance() )
	{
		vsSingletonManager::Instance()->UnregisterSingleton(s_instance, typeid(s_instance).name());
	}
	s_instance = 0;
}

/** Returns true if the instance exists. */
static bool Exists()
{
	return (s_instance != nullptr);
}

/** Returns a reference to the instance.
*  It is not legal to call this function if the instance does not exist.
*/
static T& Singleton()
{
	return *Instance();
}

/** Returns a pointer to the instance.
*  Returns nullptr if the instance does not exist.
*  This is legal to do, as opposed to GetSingleton(), which is not legal to call
*  when the instance does not exist.
*/
static T* Instance()
{
	if ( !s_instance && vsSingletonManager::Instance() )
	{
		s_instance = reinterpret_cast<T*>(vsSingletonManager::Instance()->GetSingleton(typeid(s_instance).name()));
	}
	vsAssertF( s_instance, "No instance of %s?", Demangle(typeid(T).name()).c_str() );
	return s_instance;
}

private:
static T *s_instance;
};

template <typename T> T* vsSingleton <T>::s_instance = 0;

#endif // MIX_SINGLETON_H
