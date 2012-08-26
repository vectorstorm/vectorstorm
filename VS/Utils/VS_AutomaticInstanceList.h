/*
 *  MIX_AutomaticInstanceList.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 31/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef MIX_AUTOMATICINSTANCELIST_H
#define MIX_AUTOMATICINSTANCELIST_H

/** An automatically maintained list of instances for a type.
*
*  To add the ability to enumerate through all instances of a particular type,
*  derive that type from this template.
*
*  This class will work perfectly with multiple classes in a (single or multiple)
*  inheritance tree requiring an automatic instance list.
*  You will just need to provide the appropriate scope resolution to identiy
*  which automatic list you are referring to (as you would expect!).
*
*\code
*   class SomeClass : vsAutomaticInstanceList<SomeClass> {};
*   class DerivedClass : public SomeClass, public
vsAutomaticInstanceList<DerivedClass> {};
*
*   SomeClass a, b, c;
*   DerivedClass d, e, f;
*
*   for( SomeClass *instance = SomeClass::getFirstInstance();
		 *        instance != NULL;
		 *        instance = instance->getNextInstance() )
*   {
	*      // ...
	*   }
*
*   SomeClass *someClassInstance =
vsAutomaticInstanceList<SomeClass>::getFirstInstance;
*   DerivedClass *derivedClassInstance =
vsAutomaticInstanceList<DerivedClass>::getFirstInstance;
*\endcode
*
*\ingroup qcl_types_utility
*/
template <class T>
class vsAutomaticInstanceList
{
public:
    vsAutomaticInstanceList();
    vsAutomaticInstanceList( const vsAutomaticInstanceList<T> &otherObject );
    virtual ~vsAutomaticInstanceList();
	vsAutomaticInstanceList<T> &operator =( const vsAutomaticInstanceList<T> &otherObject );

    T *GetNextInstance() const;
    T *GetPreviousInstance() const;

	static T      *GetFirstInstance();
    static uint32_t  GetInstanceCount();

private:
		static T *s_firstInstance;
		static T *s_lastInstance;
    T *m_previousInstance;
    T *m_nextInstance;
};

template<class T> T *vsAutomaticInstanceList<T>::s_firstInstance = NULL;
/**< Static variable used to hold the head of the list.
*/

template<class T> T *vsAutomaticInstanceList<T>::s_lastInstance = NULL;


/** Adds this instance to the automatic instance list for this type.
*/
template<class T>
vsAutomaticInstanceList<T>::vsAutomaticInstanceList()
{
	//const size_t offset = (size_t)(T*)1 - (size_t)(vsAutomaticInstanceList<T>*)(T*)1;
	//T *thisT = (T*)((size_t)this + offset);
	T *thisT = (T*)this;

	m_nextInstance = NULL;
	m_previousInstance = s_lastInstance;
	if( m_previousInstance )
	{
		m_previousInstance->vsAutomaticInstanceList<T>::m_nextInstance = thisT;
	}

	s_lastInstance = thisT;

	if ( s_firstInstance == NULL )
	{
		s_firstInstance = thisT;
	}
}

/** Adds this instance to the automatic instance list for this type.
*/
template<class T>
vsAutomaticInstanceList<T>::vsAutomaticInstanceList( const vsAutomaticInstanceList<T>
												   &otherObject )
{
	const size_t offset = (size_t)(T*)1 - (size_t)(vsAutomaticInstanceList<T>*)(T*)1;
	T *thisT = (T*)((size_t)this + offset);

	m_nextInstance = NULL;
	m_previousInstance = s_lastInstance;
	if( m_previousInstance )
	{
		m_previousInstance->vsAutomaticInstanceList<T>::m_nextInstance = thisT;
	}

	s_lastInstance = thisT;

	if ( s_firstInstance == NULL )
	{
		s_firstInstance = thisT;
	}
}

/** Prevent the compiler-generated assignment operator from trashing the instance
links.
*/
template<class T>
vsAutomaticInstanceList<T> &
vsAutomaticInstanceList<T>::operator =( const vsAutomaticInstanceList<T> &otherObject )
{
	// nothing to do
	return *this;
}


/** Remove this instance to the automatic instance list for this type.
*/
template<class T>
vsAutomaticInstanceList<T>::~vsAutomaticInstanceList()
{
	const size_t offset = (size_t)(T*)1 - (size_t)(vsAutomaticInstanceList<T>*)(T*)1;
	T *thisT = (T*)((size_t)this + offset);

	// must be at least one instance if *we're* destructing!
	vsAssert( s_firstInstance != NULL, "No instance??" );

	//
	// unhook ourselves from the list
	//
	if( m_previousInstance )
	{
		vsAssert( thisT == m_previousInstance->vsAutomaticInstanceList<T>::m_nextInstance, "List corruption" );
		m_previousInstance->vsAutomaticInstanceList<T>::m_nextInstance = m_nextInstance;
	}
	else
	{
		vsAssert( thisT == vsAutomaticInstanceList<T>::s_firstInstance, "List corruption" );
		vsAutomaticInstanceList<T>::s_firstInstance = m_nextInstance;
	}
	if( m_nextInstance )
	{
		vsAssert( m_nextInstance->vsAutomaticInstanceList<T>::m_previousInstance == thisT, "List corruption" );
		m_nextInstance->vsAutomaticInstanceList<T>::m_previousInstance = m_previousInstance;
	}
	else
	{
		vsAssert( thisT == vsAutomaticInstanceList<T>::s_lastInstance, "List corruption" );
		vsAutomaticInstanceList<T>::s_lastInstance = m_previousInstance;
	}
}


/** Returns the first instance of type T, or NULL if no instances exist.
*
*  Note that the "first" instance only corresponds to the instance's
*  position in the instance list, and bears no guaranteed relation to the age
*  of the instance in relation to other instances.
*/
template<class T> T*
vsAutomaticInstanceList<T>::GetFirstInstance()
{
	return s_firstInstance;
}

/** Returns the next instance of type T, or NULL if this is the last instance in the
list.
*/
template<class T> T*
vsAutomaticInstanceList<T>::GetNextInstance() const
{
	return m_nextInstance;
}



/** Returns the previous instance of type T, or NULL if this is the first instance
in the list.
*/
template<class T> T*
vsAutomaticInstanceList<T>::GetPreviousInstance() const
{
	return m_previousInstance;
}

/** Returns how many instances are in the list.
*
*\note this function iterates through the entire list,
*      so don't call it every time through your loop!!!
*/
template<class T> uint32_t
vsAutomaticInstanceList<T>::GetInstanceCount()
{
	uint32_t count = 0;
	T *instance = vsAutomaticInstanceList<T>::GetFirstInstance();
	while( instance )
	{
		count++;
		instance = instance->GetNextInstance();
	}
	return count;
}

#endif // MIX_AUTOMATICINSTANCELIST_H
