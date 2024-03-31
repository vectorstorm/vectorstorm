/*
 *  VS_VolatileArray.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/09/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_VOLATILEARRAY_H
#define VS_VOLATILEARRAY_H

#include "VS/Math/VS_Random.h"

/**
 * A vsVolatileArray is very much like a vsArray, except that it has faster
 * support for "Remove()" operations, since it is allowed to rearrange its
 * elements.  This means that an object inside a vsVolatileArray is not
 * guaranteed to remain sorted in the same order.
 */

template<class T> class vsVolatileArray;

template<class T>
class vsVolatileArrayIterator
{
	const vsVolatileArray<T> *		m_parent;
	int						m_current;

public:

	vsVolatileArrayIterator( const vsVolatileArray<T> *array, int initial ): m_parent(array), m_current(initial) {}

	T&				Get() { return m_parent->Get(*this); }

	bool			Next() { m_current++; return (m_current < m_parent->ItemCount() ); }
	bool			Prev() { m_current--; return (m_current >= 0); }
	bool						operator==( const vsVolatileArrayIterator &b ) { return (m_current == b.m_current && m_parent == b.m_parent ); }
	bool						operator!=( const vsVolatileArrayIterator &b ) { return !((*this)==b); }
	vsVolatileArrayIterator<T>&		operator++() { Next(); return *this; }
	vsVolatileArrayIterator<T>		operator++(int postFix) { UNUSED(postFix); vsVolatileArrayIterator<T> other(m_parent, m_current); Next(); return other; }

	vsVolatileArrayIterator<T>&		operator--() { Prev(); return *this; }
	vsVolatileArrayIterator<T>		operator--(int postFix) { UNUSED(postFix); vsVolatileArrayIterator<T> other(m_parent, m_current); Prev(); return other; }
	T& operator*() { return Get(); }
	T* operator->() { return &Get(); }

	friend class vsVolatileArray<T>;
};

template<class T>
class vsVolatileArray
{
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

	typedef vsVolatileArrayIterator<T> Iterator;

	vsVolatileArray( const vsVolatileArray& other )
	{
		m_array = new T[ other.ItemCount() ];
		m_arrayLength = other.ItemCount();
		m_arrayStorage = m_arrayLength;
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			m_array[i] = other.m_array[i];
		}
	}

	explicit vsVolatileArray( int initialStorage = 4 )
	{
		m_array = new T[ initialStorage ];
		m_arrayLength = 0;
		m_arrayStorage = initialStorage;
	}

	virtual ~vsVolatileArray()
	{
		vsDeleteArray( m_array );
	}

	T&		Get( const vsVolatileArrayIterator<T> &iter ) const
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

			T *newArray = new T[newSize];
			for ( int i = 0; i < m_arrayLength; i++ )
			{
				newArray[i] = m_array[i];
			}
			vsDeleteArray( m_array );
			m_array = newArray;

			m_arrayStorage = newSize;

			return AddItem( item );
		}
	}

	bool	RemoveItem( const T &item )
	{
		int index = FindEntry(item);
		if ( index != npos )
		{
			return RemoveIndex( index );
		}
		return false;
	}

	bool	RemoveIndex( int index )
	{
		vsAssert(index >= 0 && index < m_arrayLength, "Out of bounds vsVolatileArray access");
		// move the last element into this position
		m_array[index] = m_array[m_arrayLength-1];
		m_arrayLength--;
		return index != npos;
	}

	vsVolatileArrayIterator<T>	RemoveItem( vsVolatileArrayIterator<T> &item )
	{
		int index = item.m_current;
		if ( index != npos )
		{
			m_array[index] = m_array[m_arrayLength-1];
			m_arrayLength--;
		}
		if ( index < m_arrayLength )
			return item;
		return End();
	}

	bool	Contains( const T item ) const
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

	vsVolatileArrayIterator<T>	Begin() const
	{
		return 		vsVolatileArrayIterator<T>(this,0);
	}

	vsVolatileArrayIterator<T>	End() const
	{
		return 		vsVolatileArrayIterator<T>(this,m_arrayLength);
	}

	vsVolatileArrayIterator<T>	Front() const
	{
		return 		vsVolatileArrayIterator<T>(this,0);
	}

	vsVolatileArrayIterator<T>	Back() const
	{
		return 		vsVolatileArrayIterator<T>(this,m_arrayLength-1);
	}


	T	&GetItem(int id)
	{
		vsAssert(id >= 0 && id < m_arrayLength, "Out of bounds vsVolatileArray access");
		return m_array[id];
	}

	const T	&GetItem(int id) const
	{
		vsAssert(id >= 0 && id < m_arrayLength, "Out of bounds vsVolatileArray access");
		return m_array[id];
	}

	void Reserve( int newSize )
	{
		if ( newSize <= m_arrayStorage )
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

	T	&operator[](int id)
	{
		return GetItem(id);
	}

	void operator=( const vsVolatileArray<T>& other )
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


	const T	&operator[](int id) const
	{
		return GetItem(id);
	}
	const T& Random() const
	{
		return operator[]( vsRandom::GetInt( ItemCount() ) );
	}

	T Random()
	{
		return operator[]( vsRandom::GetInt( ItemCount() ) );
	}


	static const int npos = -1;
};

#endif // VS_VOLATILEARRAY_H

