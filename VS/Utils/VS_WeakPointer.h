#ifndef VS_WEAKPOINTER_H
#define VS_WEAKPOINTER_H

#include "VS_WeakPointerTarget.h"
#include "VS/Utils/VS_Demangle.h"

template< class T >
class vsWeakPointer : protected vsWeakPointerBase
{
	T* m_ptr;
public:
	vsWeakPointer( T *object = nullptr );
	vsWeakPointer( const vsWeakPointer<T> &sp );
	virtual ~vsWeakPointer();

	virtual void Clear() { m_ptr = nullptr; }

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
	m_ptr(object)
{
	if( m_ptr )
	{
		m_ptr->AddReference( this );
	}
}


template< class T >
vsWeakPointer<T>::vsWeakPointer( const vsWeakPointer<T> &sp ):
	m_ptr( sp.m_ptr )
{
	if( m_ptr )
	{
		m_ptr->AddReference( this );
	}
}

template< class T >
vsWeakPointer<T>::~vsWeakPointer()
{
	if( Target() )
	{
		Target()->ReleaseReference( this );
	}
}


template< class T >
vsWeakPointer<T> &
vsWeakPointer<T>::operator=( const vsWeakPointer &sp )
{
	if( this == &sp )
	{
		// self-assignment, nothing to do here!
		return *this;
	}

	// release the old reference
	if( Target() )
	{
		Target()->ReleaseReference( this );
	}

	m_ptr = sp.m_ptr;

	// add the new reference
	if( m_ptr )
	{
		m_ptr->AddReference( this );
	}

	return *this;
}


template< class T >
vsWeakPointer<T> &
vsWeakPointer<T>::operator=( T *object )
{
	// release the old reference
	if( Target() )
	{
		Target()->ReleaseReference( this );
	}

	m_ptr = object;

	// add the new reference
	if( object )
	{
		object->AddReference( this );
	}

	return *this;
}


//  Conversion to a const C pointer.
template< class T >
vsWeakPointer<T>::operator const T *() const
{
	return Target();
}

//  Conversion to a non-const C pointer.
template< class T >
vsWeakPointer<T>::operator T *()
{
	return Target();
}


//  Arrow operator - access to const members of the object pointed at.
template< class T >
const T *
vsWeakPointer<T>::operator->() const
{
	if ( m_ptr == nullptr )
	{
		vsAssertF( m_ptr != nullptr, "dereferenced weak pointer of type '%s' which is nullptr", Demangle(typeid(T).name()) );
	}
	return Target();
}


//  Arrow operator - access to the non-const members of the object pointed at.
template< class T >
T *
vsWeakPointer<T>::operator->()
{
	if ( m_ptr == nullptr )
	{
		vsAssertF( m_ptr != nullptr, "dereferenced weak pointer of type '%s' which is nullptr", Demangle(typeid(T).name()) );
	}
	return Target();
}


//  Direct pointer access
template< class T >
T *
vsWeakPointer<T>::Target()
{
	return m_ptr;
}

//  Direct pointer access
template< class T >
const T *
vsWeakPointer<T>::Target() const
{
	return m_ptr;
}

#endif // VS_WEAKPOINTER_H
