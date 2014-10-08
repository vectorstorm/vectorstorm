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
	vsVolatileArrayIterator<T>		operator++(int postFix) { vsVolatileArrayIterator<T> other(m_parent, m_current); Next(); return other; }

	vsVolatileArrayIterator<T>&		operator--() { Prev(); return *this; }
	vsVolatileArrayIterator<T>		operator--(int postFix) { vsVolatileArrayIterator<T> other(m_parent, m_current); Prev(); return other; }
	T* operator->() { return &Get(); }

	friend class vsVolatileArray<T>;
};

template<class T>T& operator*(vsVolatileArrayIterator<T>& i) { return i.Get(); }

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

	vsVolatileArray( int initialStorage = 4 )
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
			int newSize = vsMax( 16, m_arrayStorage * 2 );

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
			// move the last element into this position
			m_array[index] = m_array[m_arrayLength-1];
			m_arrayLength--;
		}
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

	vsVolatileArrayIterator<T>	Begin() const
	{
		return 		vsVolatileArrayIterator<T>(this,0);
	}

	vsVolatileArrayIterator<T>	End() const
	{
		return 		vsVolatileArrayIterator<T>(this,m_arrayLength);
	}

	T	&GetItem(int id)
	{
		vsAssert(id >= 0 && id < m_arrayLength, "Out of bounds vsVolatileArray access");
		return m_array[id];
	}

	T	&operator[](int id)
	{
		return GetItem(id);
	}

	static const int npos = -1;
};

#endif // VS_VOLATILEARRAY_H

