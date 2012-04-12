#ifndef MIX_TYPES_POINTERS_STRONGPOINTER_H
#define MIX_TYPES_POINTERS_STRONGPOINTER_H

#include "VS_StrongPointerTarget.h"

/** A smart pointer to class T with strong semantics.
 *
 *  The T type must have been derived from vsStrongPointerTarget.
 *
 *  This type of pointer:
 *    - reference count the objects they point at
 *    - automatically delete objects when the last strong pointer disappears
 *
 *  Apart from that, they behave just like an ordinary pointer. Enjoy!
 *
 *	Be wary of using a vsStrongPointer in conjunction with a qCopyOnWritePointer!
 *
 *\ingroup qcl_types_pointers
 */
template< class T >
class vsStrongPointer
{
public:
				vsStrongPointer( T *object = NULL );
				vsStrongPointer( const vsStrongPointer<T> &sp );
	virtual    ~vsStrongPointer();

	vsStrongPointer<T> &	operator = ( const vsStrongPointer<T> &sp );
	vsStrongPointer<T> &	operator = ( T *object );

				operator const T *() const;
				operator       T *();

			T *	operator->();
	  const T *	operator->() const;


private:
	T *m_target;
};




/** Construct a new vsStrongPointer from a dumb pointer.
 *  The object being attached do won't necessarily have a 0 reference count!
 */
template< class T >
vsStrongPointer<T>::vsStrongPointer( T *object )
 :	m_target( object )
{
	if( m_target )
	{
		m_target->vsStrongPointerTarget::addReference();
	}
}


/** Copy constructor.
 */
template< class T >
vsStrongPointer<T>::vsStrongPointer( const vsStrongPointer<T> &sp )
:	m_target( sp.m_target )
{
	if( m_target )
	{
		m_target->vsStrongPointerTarget::addReference();
	}
}

/** Destructor. Will release the object if this is the last pointer!
 */
template< class T >
vsStrongPointer<T>::~vsStrongPointer()
{
	if( m_target )
	{
		if( m_target->vsStrongPointerTarget::releaseReference() == 0 )
		{
			// last object, bye!
			delete m_target;
		}
	}
}


/** Standard assignment.
 *   If this pointer was the last pointing to its object,
 *   that object will be deleted when the new pointer is assigned.
 */
template< class T >
vsStrongPointer<T> &
vsStrongPointer<T>::operator=( const vsStrongPointer &sp )
{
	if( this == &sp )
	{
		// self assignment, sod off!
		return *this;
	}

	// release the old reference
	if( m_target )
	{
		if( m_target->vsStrongPointerTarget::releaseReference() == 0 )
		{
			// last pointer, bye!
			delete m_target;
		}
	}

	// get the new reference
	m_target = sp.m_target;
	if( m_target )
	{
		m_target->vsStrongPointerTarget::addReference();
	}

	return *this;
}


/**  Standard assignment.
 *   If this pointer was the last pointing to its object,
 *   that object will be deleted when the new pointer is assigned.
 */
template< class T >
vsStrongPointer<T> &
vsStrongPointer<T>::operator=( T *object )
{
	// release the old reference
	if( m_target )
	{
		if( m_target->vsStrongPointerTarget::releaseReference() == 0 )
		{
			// last object, bye!
			delete m_target;
		}
	}

	// get the new reference
	m_target = object;
	if( m_target )
	{
		m_target->vsStrongPointerTarget::addReference();
	}

	return *this;
}


/**  Writable access to the object.
 *   Requires a non-const vsStrongPointer.
 */
template< class T >
vsStrongPointer<T>::operator T *()
{
	return m_target;
}


/**  Read-only access to the object.
 */
template< class T >
vsStrongPointer<T>::operator const T *() const
{
	return m_target;
}


/**  Arrow operator - write access to the members of the object pointed at.
 *   Only safe to use on non-NULL pointers.
 *   Requires a non-const vsStrongPointer.
 */
template< class T >
T *
vsStrongPointer<T>::operator->()
{
	assert( m_target != NULL );
	return m_target;
}

/**  Const arrow operator - read access to the members of the object pointed at.
 *   Only safe to use on non-NULL pointers.
 */
template< class T >
const T *
vsStrongPointer<T>::operator->() const
{
	assert( m_target != NULL );
	return m_target;
}


#endif // MIX_TYPES_POINTERS_STRONGPOINTER_H
