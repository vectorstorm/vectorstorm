#ifndef MIX_TYPES_POINTERS_WEAKPOINTERTARGET_H
#define MIX_TYPES_POINTERS_WEAKPOINTERTARGET_H

class vsWeakPointerTarget;

/** Base class for weak pointer implementations.
 *
 *\ingroup qcl_types_pointers
 */
class vsWeakPointerBase
{
public:
	vsWeakPointerBase();
	virtual void Clear() = 0;
	vsWeakPointerBase   *m_previousWeakPointer;
	vsWeakPointerBase   *m_nextWeakPointer;
};

/** Parent class for objects that want to be pointed at by a weak pointer.
 *
 *  Weak pointers do not keep an instance alive like strong pointers do.
 *
 *  They notice when the object they are pointing at is destroyed, and
 *  will take on a different value when that happens.
 *  For example, a standard vsWeakPointer becomes nullptr when the object being pointed at is destroyed.
 *
 *\ingroup qcl_types_pointers
 */

class vsWeakPointerTarget
{
public:
	vsWeakPointerTarget();
			///< Initialize the reference count to 0.

	vsWeakPointerTarget( const vsWeakPointerTarget &otherCount ) { UNUSED(otherCount); m_referenceCount = 0; m_firstWeakPointer = nullptr; }
			///< Copy constructor. Note that this initialize the reference count to 0!

	vsWeakPointerTarget &operator=( const vsWeakPointerTarget &otherCount ) { UNUSED(otherCount); return *this; }
			///< Assignment operator.

	virtual ~vsWeakPointerTarget();

	int AddReference( vsWeakPointerBase *newReference ) const;		// todo make this private
	int ReleaseReference( vsWeakPointerBase *oldReference ) const;	// todo make this private
	int GetReferenceCount() const { assert( m_referenceCount >= 0 ); return m_referenceCount; }
			///< Returns the reference count (obscure!)

	void	BreakAllReferences();
			///< All existing weak pointers to this object will magically become nullptr.
			///  The object itself is unaffected.

private:
	mutable int m_referenceCount;
	mutable vsWeakPointerBase *m_firstWeakPointer;
};


#include "VS_WeakPointer.h"

inline
vsWeakPointerBase::vsWeakPointerBase()
{
	m_previousWeakPointer = nullptr;
	m_nextWeakPointer = nullptr;
}


inline
vsWeakPointerTarget::vsWeakPointerTarget()
{
	m_referenceCount = 0;
	m_firstWeakPointer = nullptr;
}


/** Destructor -- detaches all existing weak pointers by setting their object pointer to nullptr.
 */
inline
vsWeakPointerTarget::~vsWeakPointerTarget()
{
	BreakAllReferences();
}


/** Force all existing weak pointers to this object to become nullptr
 */

inline
void
vsWeakPointerTarget::BreakAllReferences()
{
	while( m_firstWeakPointer )
	{
		ReleaseReference( m_firstWeakPointer );
	}
}


/** Adds a reference, returns the new reference count.
 */

inline
int
vsWeakPointerTarget::AddReference( vsWeakPointerBase *newReference ) const
{
	assert( newReference != nullptr );

	newReference->m_previousWeakPointer = nullptr;
	newReference->m_nextWeakPointer = m_firstWeakPointer;

	m_firstWeakPointer = newReference;

	if( newReference->m_nextWeakPointer )
	{
		newReference->m_nextWeakPointer->m_previousWeakPointer = newReference;
	}

	return ++m_referenceCount;
}


/**< Releases a reference, returns the new reference count.
 */

inline
int
vsWeakPointerTarget::ReleaseReference( vsWeakPointerBase *oldReference ) const
{
	assert( oldReference != nullptr );

	if( oldReference->m_previousWeakPointer )
	{
		oldReference->m_previousWeakPointer->m_nextWeakPointer = oldReference->m_nextWeakPointer;
	}
	else
	{
		assert( oldReference == m_firstWeakPointer );
		m_firstWeakPointer = oldReference->m_nextWeakPointer;
	}

	if( oldReference->m_nextWeakPointer )
	{
		oldReference->m_nextWeakPointer->m_previousWeakPointer = oldReference->m_previousWeakPointer;
	}

	oldReference->Clear();
	oldReference->m_previousWeakPointer = nullptr;
	oldReference->m_nextWeakPointer = nullptr;

	m_referenceCount--;

	return m_referenceCount;
}


#endif // MIX_TYPES_POINTERS_WEAKPOINTERTARGET_H
