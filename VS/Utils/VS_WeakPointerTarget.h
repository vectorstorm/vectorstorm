#ifndef MIX_TYPES_POINTERS_WEAKPOINTERTARGET_H
#define MIX_TYPES_POINTERS_WEAKPOINTERTARGET_H

#include <atomic>

class vsWeakPointerTarget
{
	template<class T> friend class vsWeakPointer;
	class Proxy
	{
		vsWeakPointerTarget *m_target;
		std::atomic<int> m_refCount;
	public:

		Proxy( vsWeakPointerTarget *target ):
			m_target(target),
			m_refCount(0)
		{
		}

		~Proxy()
		{
			if ( m_target )
				m_target->Detach();
			m_target = nullptr;
		}

		vsWeakPointerTarget *Get() { return m_target; }
		const vsWeakPointerTarget *Get() const { return m_target; }

		void Detach() { m_target = nullptr; if ( m_refCount == 0 ) delete this; }
		void IncRef() { m_refCount++; }
		void DecRef() { if ( --m_refCount <= 0 && !m_target ) delete this; }
	};

	Proxy *m_proxy;

public:
	vsWeakPointerTarget();
			///< Initialize the reference count to 0.

	vsWeakPointerTarget( const vsWeakPointerTarget &other );
	vsWeakPointerTarget &operator=( const vsWeakPointerTarget &other );

	virtual ~vsWeakPointerTarget();

	void	Detach();
	void	BreakAllReferences();

	Proxy*	GetProxy();
};


#include "VS_WeakPointer.h"

inline
vsWeakPointerTarget::vsWeakPointerTarget():
	m_proxy(nullptr)
{
}

inline
vsWeakPointerTarget::vsWeakPointerTarget( const vsWeakPointerTarget &other ):
	vsWeakPointerTarget()
{
}

inline
vsWeakPointerTarget &
vsWeakPointerTarget::operator=( const vsWeakPointerTarget &other )
{
	// nothing to copy, and we probably don't need to detach pointers to us.
	return *this;
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
	if ( m_proxy )
	{
		m_proxy->Detach();
		m_proxy = nullptr;
	}
}

inline
void
vsWeakPointerTarget::Detach()
{
	BreakAllReferences();
}

inline
vsWeakPointerTarget::Proxy*
vsWeakPointerTarget::GetProxy()
{
	if ( !m_proxy )
		m_proxy = new Proxy(this);
	return m_proxy;
}


#endif // MIX_TYPES_POINTERS_WEAKPOINTERTARGET_H
