#ifndef QCL_TYPES_POINTERS_WEAKPOINTER_H
#define QCL_TYPES_POINTERS_WEAKPOINTER_H

#include "VS_WeakPointerTarget.h"



/** A smart pointer to class T with weak semantics.
 *
 *  The T type must have been derived from vsWeakPointerTarget.
 *
 *  This type of pointer:
 *    - keeps track reference count the objects they point at
 *    - automatically NULLs itself when the target object is deleted.
 *
 *  Apart from that, they behave just like an ordinary pointer. Enjoy!
 *
 *	Be wary of using a vsWeakPointer in conjunction with a qCopyOnWritePointer!
 *
 *  See also qDerivedWeakPointer< T, ParentTargetT >
 *
 *\ingroup qcl_types_pointers
 */
template< class T >
class vsWeakPointer : protected vsWeakPointerBase
{
public:
				vsWeakPointer( T *object = NULL );
				vsWeakPointer( const vsWeakPointer<T> &sp );
	virtual    ~vsWeakPointer();

	vsWeakPointer<T> &	operator = ( const vsWeakPointer<T> &sp );
	vsWeakPointer<T> &	operator = ( T *object );

						operator const T *() const;
						operator T *();

			 const T *	operator->() const;
				   T *	operator->();

					 T *	Target();
			   const T *	Target() const;
private:
	friend class vsWeakPointerTarget;
};





/** Construct a new vsWeakPointer from a dumb pointer.
 *  The object being attached do won't necessarily have a 0 reference count!
 */
template< class T >
vsWeakPointer<T>::vsWeakPointer( T *object )
{
	if( object )
	{
		object->AddReference( this );
	}
}


/** Copy constructor.
 */
template< class T >
vsWeakPointer<T>::vsWeakPointer( const vsWeakPointer<T> &sp )
{
	if( sp.Target() )
	{
		sp.Target()->AddReference( this );
	}
}

/** Destructor. Will release the object if this is the last pointer!
 */
template< class T >
vsWeakPointer<T>::~vsWeakPointer()
{
	if( Target() )
	{
		Target()->ReleaseReference( this );
	}
}


/** Standard assignment.
 *   If this pointer was the last pointing to its object,
 *   that object will be deleted when the new pointer is assigned.
 */
template< class T >
vsWeakPointer<T> &
vsWeakPointer<T>::operator=( const vsWeakPointer &sp )
{
	if( this == &sp )
	{
		// self assignment, sod off!
		return *this;
	}

	// release the old reference
	if( Target() )
	{
		Target()->ReleaseReference( this );
	}

	// get the new reference
	if( sp.Target() )
	{
		sp.Target()->AddReference( this );
	}

	return *this;
}


/**  Standard assignment.
 *   If this pointer was the last pointing to its object,
 *   that object will be deleted when the new pointer is assigned.
 */
template< class T >
vsWeakPointer<T> &
vsWeakPointer<T>::operator=( T *object )
{
	// release the old reference
	if( Target() )
	{
		Target()->ReleaseReference( this );
	}

	// get the new reference
	if( object )
	{
		object->AddReference( this );
	}

	return *this;
}


/**  Conversion to a const dumb pointer.
 */
template< class T >
vsWeakPointer<T>::operator const T *() const
{
	return Target();
}

/**  Conversion to a dumb pointer.
 */
template< class T >
vsWeakPointer<T>::operator T *()
{
	return Target();
}


/**  Arrow operator - access to the members of the object pointed at.
 */
template< class T >
const T *
vsWeakPointer<T>::operator->() const
{
	assert( m_target != NULL );
	return Target();
}


/**  Arrow operator - access to the members of the object pointed at.
 */
template< class T >
T *
vsWeakPointer<T>::operator->()
{
	assert( m_target != NULL );
	return Target();
}


/**  Write access to the members of the object pointed at.
 */
template< class T >
T *
vsWeakPointer<T>::Target()
{
	return (T*)m_target;
}

/**  Read access to the members of the object pointed at.
 */
template< class T >
const T *
vsWeakPointer<T>::Target() const
{
	return (const T *)m_target;
}

#endif // QCL_TYPES_POINTERS_WEAKPOINTER_H
