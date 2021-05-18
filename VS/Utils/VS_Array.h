/*
 *  VS_Array.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 14/09/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */


#ifndef VS_ARRAY_H
#define VS_ARRAY_H

#include "VS/Utils/VS_Demangle.h"

template<class T> class vsArray;

template<class T>
class vsArrayIterator
{
	const vsArray<T> *		m_parent;
	int						m_current;

public:

	vsArrayIterator( const vsArray<T> *array, int initial ): m_parent(array), m_current(initial) {}

	T&				Get() { return m_parent->Get(*this); }

	bool			Next() { m_current = vsMin( m_parent->ItemCount(), m_current+1 ); return (m_current < m_parent->ItemCount() ); }
	bool			Prev() { m_current = vsMax( 0, m_current-1); return (m_current >= 0); }
	bool						operator==( const vsArrayIterator &b ) { return (m_current == b.m_current && m_parent == b.m_parent ); }
	bool						operator!=( const vsArrayIterator &b ) { return !((*this)==b); }
	vsArrayIterator<T>&		operator++() { Next(); return *this; }
	vsArrayIterator<T>		operator++(int postFix) { vsArrayIterator<T> other(m_parent, m_current); Next(); return other; }

	vsArrayIterator<T>&		operator--() { Prev(); return *this; }
	vsArrayIterator<T>		operator--(int postFix) { vsArrayIterator<T> other(m_parent, m_current); Prev(); return other; }
	T& operator*() { return Get(); }
	T* operator->() { return &Get(); }

	friend class vsArray<T>;
};

template<class T>
class vsArray
{
	// SortFunction should return TRUE if a < b. (and therefore should be
	// earlier in the array).
	typedef bool(SortFunction)(const T& a, const T& b);

	T *					m_array;
	int					m_arrayLength;		// how many things actually in our array?
	int					m_arrayStorage;		// how big is our storage?  (We can fit this many things into our array without resizing it)

	int	FindEntry( T item ) const
	{
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			if ( m_array[i] == item )
				return i;
		}

		return npos;
	}

public:

	typedef vsArrayIterator<T> Iterator;

	vsArray( const vsArray<T>& other )
	{
		m_array = new T[ other.ItemCount() ];
		m_arrayLength = other.ItemCount();
		m_arrayStorage = m_arrayLength;
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			m_array[i] = other.m_array[i];
		}
	}

	explicit vsArray( int initialStorage = 4 )
	{
		m_array = new T[ initialStorage ];
		m_arrayLength = 0;
		m_arrayStorage = initialStorage;
	}

	virtual ~vsArray()
	{
		vsDeleteArray( m_array );
	}

	T&		Get( const vsArrayIterator<T> &iter ) const
	{
		return m_array[ iter.m_current ];
	}

	virtual void	Clear()
	{
		m_arrayLength = 0;
	}

	virtual void	PopBack()
	{
		m_arrayLength--;
	}

	void	AddItem( const T &item )
	{
		if ( m_arrayLength < m_arrayStorage )
		{
			m_array[ m_arrayLength++ ] = item;
		}
		else
		{
			// reallocate our array and copy data into it.
			int newSize = vsMax( 4, m_arrayStorage * 2 );
			Reserve(newSize);
			return AddItem( item );
		}
	}

	void Reserve( int newSize )
	{
		if ( newSize <= m_arrayLength )
			return;

		T *newArray = new T[newSize];
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			newArray[i] = m_array[i];
		}
		vsDeleteArray( m_array );
		m_array = newArray;

		m_arrayStorage = newSize;
	}

	bool	RemoveItem( const T &item )
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

	vsArrayIterator<T>	RemoveItem( vsArrayIterator<T> &item )
	{
		int index = item.m_current;
		if ( index != npos )
		{
			for ( int i = index; i < m_arrayLength-1; i++ )
			{
				m_array[i] = m_array[i+1];
			}
			m_arrayLength--;
		}
		if ( index < m_arrayLength )
			return item;
		return End();
	}

	void	RemoveDuplicates()
	{
		vsArrayIterator<T> it = Begin();
		while ( End() != it )
		{
			// Now we need to look for duplicates.
			vsArrayIterator<T> dit = it;
			dit++;

			while ( End() != dit )
			{
				if ( *it == *dit )
					dit = RemoveItem(dit);
				else
					dit++;
			}
			it++;
		}
	}

	bool	Contains( T item ) const
	{
		return (npos != FindEntry(item));
	}

	int		Find( T item ) const
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

	vsArrayIterator<T>	Begin() const
	{
		return 		vsArrayIterator<T>(this,0);
	}

	vsArrayIterator<T>	End() const
	{
		return 		vsArrayIterator<T>(this,m_arrayLength);
	}

	vsArrayIterator<T>	Front() const
	{
		return 		vsArrayIterator<T>(this,0);
	}

	vsArrayIterator<T>	Back() const
	{
		return 		vsArrayIterator<T>(this,m_arrayLength-1);
	}

	T	&GetItem(int id)
	{
		vsAssert(id >= 0 && id < m_arrayLength,
				vsFormatString("Out of bounds vsArray access: requested element %d, capacity of %d (array of %s)", id, ItemCount(), Demangle( typeid(T).name() ) )
				);
		return m_array[id];
	}

	const T	&GetItem(int id) const
	{
		vsAssert(id >= 0 && id < m_arrayLength,
				vsFormatString("Out of bounds vsArray access: requested element %d, capacity of %d (array of %s)", id, ItemCount(), Demangle( typeid(T).name() ) )
				);
		return m_array[id];
	}

	const T	&operator[](int id) const
	{
		return GetItem(id);
	}

	T	&operator[](int id)
	{
		return GetItem(id);
	}

	void SetArraySize( int size )
	{
		// add or remove elements to make us this size.
		while( ItemCount() > size )
		{
			PopBack();
		}
		while ( ItemCount() < size )
		{
			AddItem( T() );
		}
	}

	void operator=( const vsArray<T>& other )
	{
		vsDeleteArray(m_array);
		m_array = new T[ other.ItemCount() ];
		m_arrayLength = other.ItemCount();
		m_arrayStorage = m_arrayLength;
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			m_array[i] = other.m_array[i];
		}
	}

	bool operator==( const vsArray<T>& other ) const
	{
		if ( ItemCount() != other.ItemCount() )
			return false;
		for ( int i = 0; i < ItemCount(); i++ )
		{
			if ( m_array[i] != other.m_array[i] )
				return false;
		}
		return true;
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
					T swap = m_array[i];
					m_array[i] = m_array[i+1];
					m_array[i+1] = swap;
					sorted = false;
				}
			}
		}
	}

	static const int npos = -1;
};

#endif // VS_ARRAY_H

