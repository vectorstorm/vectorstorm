/*
 *  VS_ArrayStore.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 31/10/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */


#ifndef VS_ARRAY_STORE_H
#define VS_ARRAY_STORE_H

#include "VS/Utils/VS_Demangle.h"


template<class T> class vsArrayStore;

template<class T>
class vsArrayStoreIterator
{
	const vsArrayStore<T> *		m_parent;
	int							m_current;

public:

	vsArrayStoreIterator( const vsArrayStore<T> *array, int initial ): m_parent(array), m_current(initial) {}

	T*				Get() { return m_parent->Get(*this); }

	bool			Next() { m_current++; return (m_current < m_parent->ItemCount() ); }
	bool			Prev() { m_current--; return (m_current >= 0); }
	bool						operator==( const vsArrayStoreIterator &b ) { return (m_current == b.m_current && m_parent == b.m_parent ); }
	bool						operator!=( const vsArrayStoreIterator &b ) { return !((*this)==b); }
	vsArrayStoreIterator<T>&	operator++() { Next(); return *this; }
	vsArrayStoreIterator<T>		operator++(int postFix) { UNUSED(postFix); vsArrayStoreIterator<T> other(m_parent, m_current); Next(); return other; }

	vsArrayStoreIterator<T>&	operator--() { Prev(); return *this; }
	vsArrayStoreIterator<T>		operator--(int postFix) { UNUSED(postFix); vsArrayStoreIterator<T> other(m_parent, m_current); Prev(); return other; }
	T* operator->() { return Get(); }
	T* operator*() { return Get(); }

	friend class vsArrayStore<T>;
};

template<class T>
class vsArrayStore
{
	// SortFunction should return TRUE if a < b. (and therefore should be
	// earlier in the array).
	typedef bool(SortFunction)(const T* a, const T* b);

	T **				m_array;
	int					m_arrayLength;		// how many things actually in our array?
	int					m_arrayStorage;		// how big is our storage?  (We can fit this many things into our array without resizing it)

	int	FindEntry( const T* item ) const
	{
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			if ( m_array[i] == item )
				return i;
		}

		return npos;
	}

public:

	typedef vsArrayStoreIterator<T> Iterator;

	explicit vsArrayStore( int initialStorage = 4 )
	{
		m_array = new T*[ initialStorage ];
		m_arrayLength = 0;
		m_arrayStorage = initialStorage;
	}

	virtual ~vsArrayStore()
	{
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			vsDelete( m_array[i] );
		}
		vsDeleteArray( m_array );
		m_arrayLength = 0;
		m_arrayStorage = 0;
	}

	// take the elements from that other store, including transferring
	// ownership
	void Adopt( vsArrayStore<T> &other )
	{
		for ( int i = 0; i < other.m_arrayLength; i++ )
			AddItem( other.m_array[i] );
		other.m_arrayLength = 0;
	}

	T *		Get( const vsArrayStoreIterator<T> &iter ) const
	{
		return m_array[ iter.m_current ];
	}

	virtual void	PopBack()
	{
		vsDelete( m_array[m_arrayLength-1] );
		m_arrayLength--;
	}

	void	Clear()
	{
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			vsDelete( m_array[i] );
		}
		m_arrayLength = 0;
	}

	void	AddItem( T *item )
	{
		vsAssert( !Contains(item), "We already contain and own that item??" );
		if ( m_arrayLength < m_arrayStorage )
		{
			m_array[ m_arrayLength++ ] = item;
		}
		else
		{
			// reallocate our array and copy data into it.
			int newSize = vsMax( 4, m_arrayStorage * 2 );
			Reserve( newSize );
			return AddItem( item );
		}
	}

	void Reserve( int newSize )
	{
		if ( newSize <= m_arrayStorage )
			return;

		T **newArray = new T*[newSize];
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			newArray[i] = m_array[i];
		}
		vsDeleteArray( m_array );
		m_array = newArray;

		m_arrayStorage = newSize;
	}


	bool	RemoveItem( const T *item )
	{
		int index = FindEntry(item);
		if ( index != npos )
		{
			T* toBeDeleted = m_array[index];
			for ( int i = index; i < m_arrayLength-1; i++ )
			{
				m_array[i] = m_array[i+1];
			}
			m_arrayLength--;
			vsDelete( toBeDeleted );
		}
		return index != npos;
	}

	// removes this item, but doesn't destroy it.
	bool	ReleaseItem( const T *item )
	{
		int index = FindEntry(item);
		if ( index != npos )
		{
			for ( int i = index; i < m_arrayLength-1; i++ )
			{
				m_array[i] = m_array[i+1];
			}
			m_arrayLength--;
		}
		return index != npos;
	}

	vsArrayStoreIterator<T>	RemoveItem( vsArrayStoreIterator<T> &item )
	{
		int index = item.m_current;

		if ( index != npos )
		{
			T* toBeDeleted = m_array[index];
			for ( int i = index; i < m_arrayLength-1; i++ )
			{
				m_array[i] = m_array[i+1];
			}
			m_arrayLength--;
			vsDelete( toBeDeleted );
		}

		if ( index < m_arrayLength )
			return item;

		return End();
	}

	bool	Contains( T *item ) const
	{
		return (npos != FindEntry(item));
	}

	int		Find( T *item ) const
	{
		return FindEntry(item);
	}

	bool	IsEmpty() const
	{
		return ( m_arrayLength == 0 );
	}

	int		ItemCount() const
	{
		return m_arrayLength;
	}

	vsArrayStoreIterator<T>	Begin() const
	{
		return 		vsArrayStoreIterator<T>(this,0);
	}

	vsArrayStoreIterator<T>	End() const
	{
		return 		vsArrayStoreIterator<T>(this,m_arrayLength);
	}

	vsArrayStoreIterator<T>	Front() const
	{
		return 		vsArrayStoreIterator<T>(this,0);
	}

	vsArrayStoreIterator<T>	Back() const
	{
		return 		vsArrayStoreIterator<T>(this,m_arrayLength-1);
	}

	void SetArraySize( int size )
	{
		Reserve(size);
		// add or remove elements to make us this size.
		while( ItemCount() > size )
		{
			PopBack();
		}
		while ( ItemCount() < size )
		{
			AddItem( new T() );
		}
	}

	T*& GetItem(int id)
	{
		vsAssert(id >= 0 && id < m_arrayLength,
				vsFormatString("Out of bounds vsArray access: requested element %d, capacity of %d (array of %s)", id, ItemCount(), Demangle( typeid(T).name() ) )
				);
		return m_array[id];
	}

	const T* GetItemConst(int id) const
	{
		vsAssert(id >= 0 && id < m_arrayLength,
				vsFormatString("Out of bounds vsArray access: requested element %d, capacity of %d (array of %s)", id, ItemCount(), Demangle( typeid(T).name() ) )
				);
		return m_array[id];
	}

	const T* operator[](int id) const
	{
		return GetItemConst(id);
	}
	T*& operator[](int id)
	{
		return GetItem(id);
	}
	const T*& Random() const
	{
		return operator[]( vsRandom::GetInt( ItemCount() ) );
	}

	T* Random()
	{
		return operator[]( vsRandom::GetInt( ItemCount() ) );
	}


	void Sort( SortFunction lessThanFn )
	{
		// simple bubble sort as a first experiment with this approach.
		bool sorted = false;
		while ( !sorted )
		{
			sorted = true;
			for ( int i = 0; i < ItemCount()-1; i++ )
			{
				if ( lessThanFn(GetItem(i+1), GetItem(i) ) )
				{
					T* swap = m_array[i];
					m_array[i] = m_array[i+1];
					m_array[i+1] = swap;
					sorted = false;
				}
			}
		}
	}

	void MoveItemBefore(T* item, int newIndex)
	{
		int oldIndex = FindEntry(item);

		// If it's not in our list already, add it
		if (oldIndex == npos)
		{
			AddItem(item);
			oldIndex = ItemCount() - 1;
		}

		if (newIndex >= ItemCount() - 1)
		{
			newIndex = ItemCount() - 1;
		}

		if (newIndex < 0)
		{
			newIndex = 0;
		}

		int direction = oldIndex > newIndex ? -1 : 1;

		for (int i = oldIndex; i != newIndex; i += direction)
		{
			m_array[i] = m_array[i + direction];
		}

		m_array[newIndex] = item;
	}

	static const int npos = -1;
};

#endif // VS_ARRAY_STORE_H

