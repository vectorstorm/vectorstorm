#ifndef VS_WEAKPOINTER_H
#define VS_WEAKPOINTER_H

#include "VS_WeakPointerTarget.h"
#include "VS/Utils/VS_Demangle.h"

template< class T >
class vsWeakPointer
{
	mutable T* m_ptr;
	mutable vsWeakPointerTarget::Proxy* m_proxy;
public:
	vsWeakPointer( T *object = nullptr );
	vsWeakPointer( const vsWeakPointer<T> &sp );
	virtual ~vsWeakPointer();
	void Clear();

	vsWeakPointer<T> & operator=( const vsWeakPointer<T> &sp );
	vsWeakPointer<T> & operator=( T *object );

	operator const T *() const;
	operator T *();

	const T * operator->() const;
	T * operator->();

	T * Target();
	const T * Target() const;
};

template< class T >
vsWeakPointer<T>::vsWeakPointer( T *object ):
	m_ptr( object ),
	m_proxy( object ? object->GetProxy() : nullptr )
{
	if ( m_proxy )
		m_proxy->IncRef();
}


template< class T >
vsWeakPointer<T>::vsWeakPointer( const vsWeakPointer<T> &sp ):
	m_ptr( sp.m_ptr ),
	m_proxy( sp.m_proxy )
{
	if ( m_proxy )
		m_proxy->IncRef();
}

template< class T >
vsWeakPointer<T>::~vsWeakPointer()
{
	if ( m_proxy )
		m_proxy->DecRef();
	m_proxy = nullptr;
	m_ptr = nullptr;
}

template< class T >
void
vsWeakPointer<T>::Clear()
{
	if ( m_proxy )
		m_proxy->DecRef();
	m_proxy = nullptr;
	m_ptr = nullptr;
}

template< class T >
vsWeakPointer<T> &
vsWeakPointer<T>::operator=( const vsWeakPointer<T> &sp )
{
	if( this == &sp )
	{
		// self-assignment, nothing to do here!
		return *this;
	}

	// release the old reference
	if( m_proxy )
		m_proxy->DecRef();

	m_ptr = sp.m_ptr;
	m_proxy = sp.m_proxy;

	// add the new reference
	if( m_proxy )
		m_proxy->IncRef();

	return *this;
}


template< class T >
vsWeakPointer<T> &
vsWeakPointer<T>::operator=( T *object )
{
	// release the old reference
	if( m_proxy )
		m_proxy->DecRef();

	m_ptr = object;
	m_proxy = object ? object->GetProxy() : nullptr;

	// add the new reference
	if( m_proxy )
		m_proxy->IncRef();

	return *this;
}


//  Conversion to a const C pointer.
template< class T >
vsWeakPointer<T>::operator const T*() const
{
	if ( !m_proxy )
		return nullptr;
	if ( m_proxy->Get() == nullptr )
	{
		m_proxy->DecRef();
		m_proxy = nullptr;
		m_ptr = nullptr;
	}
	return m_ptr;
}

//  Conversion to a non-const C pointer.
template< class T >
vsWeakPointer<T>::operator T*()
{
	if ( !m_proxy )
		return nullptr;
	if ( m_proxy->Get() == nullptr )
	{
		m_proxy->DecRef();
		m_proxy = nullptr;
		m_ptr = nullptr;
	}
	return m_ptr;
}


//  Arrow operator - access to const members of the object pointed at.
template< class T >
const T *
vsWeakPointer<T>::operator->() const
{
	if ( m_proxy == nullptr )
	{
		vsAssertF( m_proxy != nullptr, "dereferenced weak pointer of type '%s' which is nullptr", Demangle(typeid(T).name()) );
	}
	return Target();
}


//  Arrow operator - access to the non-const members of the object pointed at.
template< class T >
T *
vsWeakPointer<T>::operator->()
{
	if ( m_proxy == nullptr )
	{
		vsAssertF( m_proxy != nullptr, "dereferenced weak pointer of type '%s' which is nullptr", Demangle(typeid(T).name()) );
	}
	return Target();
}


//  Direct pointer access
template< class T >
T *
vsWeakPointer<T>::Target()
{
	if ( !m_proxy )
		return nullptr;
	if ( m_proxy->Get() == nullptr )
	{
		m_proxy->DecRef();
		m_proxy = nullptr;
		m_ptr = nullptr;
	}
	return m_ptr;
}

//  Direct pointer access
template< class T >
const T *
vsWeakPointer<T>::Target() const
{
	if ( !m_proxy )
		return nullptr;
	if ( m_proxy->Get() == nullptr )
	{
		m_proxy->DecRef();
		m_proxy = nullptr;
		m_ptr = nullptr;
	}
	return m_ptr;
}

#endif // VS_WEAKPOINTER_H
