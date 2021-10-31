/*
 *  VS_LinkedList.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 3/10/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_LINKED_LIST_H
#define VS_LINKED_LIST_H

#include "VS_Pool.h"

template<class T>
class vsListEntry
{
public:
	T					m_item;

	vsListEntry<T> *	m_next;
	vsListEntry<T> *	m_prev;

	vsListEntry() : m_item(nullptr), m_next(nullptr), m_prev(nullptr) {}
	vsListEntry( const T &t ) : m_item(t), m_next(nullptr), m_prev(nullptr) {}
	~vsListEntry() { Extract(); }

	void	Append( vsListEntry<T> *next )
	{
		next->m_next = m_next;
		next->m_prev = this;

		if ( m_next )
		{
			m_next->m_prev = next;
		}
		m_next = next;
	}

	void	Prepend( vsListEntry<T> *prev )
	{
		prev->m_next = this;
		prev->m_prev = m_prev;

		if ( m_prev )
		{
			m_prev->m_next = prev;
		}
		m_prev = prev;
	}

	void	Extract()
	{
		if ( m_next )
			m_next->m_prev = m_prev;
		if ( m_prev )
			m_prev->m_next = m_next;
	}
};

template<class T>
class vsListIterator
{
	vsListEntry<T>	*	m_current;

public:

	vsListIterator( vsListEntry<T> *initial ):
		m_current(initial)
	{
	}

	T&				Get() { return m_current->m_item; }
	bool			Next() { m_current = m_current->m_next; return (m_current != nullptr); }
	bool			Prev() { m_current = m_current->m_prev; return (m_current != nullptr); }
	bool						operator==( const vsListIterator &b ) { return (m_current->m_item == b.m_current->m_item); }
	bool						operator!=( const vsListIterator &b ) { return !((*this)==b); }
	vsListIterator<T>&		operator++() { Next(); return *this; }
	vsListIterator<T>		operator++(int postFix) { vsListIterator<T> other(m_current); Next(); return other; }

	vsListIterator<T>&		operator--() { Prev(); return *this; }
	vsListIterator<T>		operator--(int postFix) { vsListIterator<T> other(m_current); Prev(); return other; }
	T* operator->() { return &Get(); }
	T& operator*() { return Get(); }

	vsListEntry<T> * GetEntry() { return m_current; }
	//void	Append( T item )
	//{
		//vsListEntry<T> *ent = new vsListEntry<T>( item );
		//m_current->Append( ent );
	//}

	//void	Prepend( T item )
	//{
		//vsListEntry<T> *ent = new vsListEntry<T>( item );
		//m_current->Prepend( ent );
	/*}*/
};

template<class T>
class vsLinkedList
{
	// vsPool<vsListEntry<T> >	m_entry;
	vsListEntry<T>		*m_listEntry;
	vsListEntry<T>		*m_tail;

	vsListEntry<T> *	FindEntry( T item ) const
	{
		vsListEntry<T> *ent = m_listEntry->m_next;

		while( ent )
		{
			if ( ent->m_item == item )
			{
				return ent;
			}
			ent = ent->m_next;
		}
		return nullptr;
	}

public:

	typedef vsListIterator<T> Iterator;

	vsLinkedList()//:
		// m_entry(4, vsPool<vsListEntry<T> >::Type_Expandable)
	{
		m_listEntry = new vsListEntry<T>;//m_entry.Borrow();
		m_tail = new vsListEntry<T>;//m_entry.Borrow();

		m_listEntry->m_next = m_tail;
		m_tail->m_prev = m_listEntry;
	}

	~vsLinkedList()
	{
		while ( m_listEntry->m_next )
		{
			vsListEntry<T> *toDelete = m_listEntry->m_next;
			m_listEntry->m_next = m_listEntry->m_next->m_next;
			// m_entry.Return( toDelete );
			vsDelete(toDelete);
		}
		vsDelete( m_listEntry );
		// m_entry.Return( m_listEntry );
		m_listEntry = nullptr;
		m_tail = nullptr;
	}

	void	Clear()
	{
		while ( !IsEmpty() )
		{
			vsListEntry<T> *ent = m_listEntry->m_next;
			ent->Extract();
			vsDelete(ent);
			// m_entry.Return(ent);
		}
	}

	void	AddItem( const T &item )
	{
		vsListEntry<T> *ent = new vsListEntry<T>;//m_entry.Borrow();
		ent->m_item = item;

		m_tail->Prepend( ent );
	}

	// returns false if the item wasn't in the list.
	bool	RemoveItem( T item )
	{
		vsListEntry<T> *ent = FindEntry(item);
		//vsAssert(ent, "Error: couldn't find item??");

		while( ent && ent != m_tail )	// no removing our tail!
		{
			ent->Extract();
			vsDelete(ent);
			// m_entry.Return(ent);

			ent = FindEntry(item);
		}

		return ent != nullptr;
	}

	vsListIterator<T> Remove( vsListIterator<T> &iter )
	{
		vsListIterator<T> next = iter;
		next.Next();

		vsListEntry<T> *ent = iter.GetEntry();
		ent->Extract();
		vsDelete(ent);

		return next;
	}

	void	Prepend( vsListIterator<T> &iter, const T &item )
	{
		vsListEntry<T> *ent = new vsListEntry<T>;//m_entry.Borrow();
		iter.GetEntry()->Prepend( ent );
	}

	bool	Contains( T item ) const
	{
		return (nullptr != FindEntry(item));
	}

	bool	IsEmpty() const
	{
		vsListIterator<T> it(m_listEntry);

		return ( m_listEntry->m_next == m_tail );
	}

	int		ItemCount() const
	{
		int count = 0;

		vsListIterator<T> it(m_listEntry);

		while(++it != m_tail)
		{
			count++;
		}

		return count;
	}

	vsListIterator<T>	Begin() const
	{
		return 		++vsListIterator<T>(m_listEntry);
	}

	vsListIterator<T>	End() const
	{
		return 		vsListIterator<T>(m_tail);
	}

	T	operator[](int n)
	{
		for( vsListIterator<T> iter = Begin(); iter != End(); iter++ )
		{
			if ( n == 0 )
				return *iter;
			n--;
		}

		return nullptr;
	}
};

#endif // VS_LINKED_LIST_H

