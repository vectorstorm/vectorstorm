/*
 *  VS_VolatileArrayStore.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 22/01/2015
 *  Copyright 2015 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_VOLATILEARRAYSTORE_H
#define VS_VOLATILEARRAYSTORE_H

template<class T> class vsVolatileArrayStore;

template<class T>
class vsVolatileArrayStoreIterator
{
	const vsVolatileArrayStore<T> *m_parent;
	int							m_current;

public:

	vsVolatileArrayStoreIterator( const vsVolatileArrayStore<T> *array, int initial ): m_parent(array), m_current(initial) {}

	T*				Get() { return m_parent->Get(*this); }

	bool			Next() { m_current++; return (m_current < m_parent->ItemCount() ); }
	bool			Prev() { m_current--; return (m_current >= 0); }
	bool						operator==( const vsVolatileArrayStoreIterator &b ) { return (m_current == b.m_current && m_parent == b.m_parent ); }
	bool						operator!=( const vsVolatileArrayStoreIterator &b ) { return !((*this)==b); }
	vsVolatileArrayStoreIterator<T>&	operator++() { Next(); return *this; }
	vsVolatileArrayStoreIterator<T>		operator++(int postFix) { vsVolatileArrayStoreIterator<T> other(m_parent, m_current); Next(); return other; }

	vsVolatileArrayStoreIterator<T>&	operator--() { Prev(); return *this; }
	vsVolatileArrayStoreIterator<T>		operator--(int postFix) { vsVolatileArrayStoreIterator<T> other(m_parent, m_current); Prev(); return other; }
	T* operator->() { return Get(); }
	T* operator*() { return Get(); }

	friend class vsVolatileArrayStore<T>;
};

template<class T>
class vsVolatileArrayStore
{
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

	typedef vsVolatileArrayStoreIterator<T> Iterator;

	explicit vsVolatileArrayStore( int initialStorage = 4 )
	{
		m_array = new T*[ initialStorage ];
		m_arrayLength = 0;
		m_arrayStorage = initialStorage;
	}

	virtual ~vsVolatileArrayStore()
	{
		for ( int i = 0; i < m_arrayLength; i++ )
		{
			vsDelete( m_array[i] );
		}
		vsDeleteArray( m_array );
	}

	T *		Get( const vsVolatileArrayStoreIterator<T> &iter ) const
	{
		return m_array[ iter.m_current ];
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
		if ( m_arrayLength < m_arrayStorage )
		{
			m_array[ m_arrayLength++ ] = item;
		}
		else
		{
			// reallocate our array and copy data into it.
			int newSize = vsMax( 4, m_arrayStorage * 2 );

			T **newArray = new T*[newSize];
			for ( int i = 0; i < m_arrayLength; i++ )
			{
				newArray[i] = m_array[i];
			}
			vsDeleteArray(m_array);
			m_array = newArray;
			m_arrayStorage = newSize;

			return AddItem( item );
		}
	}

	bool	RemoveItem( const T *item )
	{
		int index = FindEntry(item);
		if ( index != npos )
		{
			vsDelete( m_array[index] );
			// move the last element into this position
			m_array[index] = m_array[m_arrayLength-1];
			m_arrayLength--;
		}
		return index != npos;
	}

	vsVolatileArrayStoreIterator<T>	RemoveItem( vsVolatileArrayStoreIterator<T> &item )
	{
		int index = item.m_current;

		if ( index != npos )
		{
			vsDelete( m_array[index] );
			m_array[index] = m_array[m_arrayLength-1];
			m_arrayLength--;
		}

		if ( index < m_arrayLength )
			return item;

		return End();
	}

	// removes this item, but doesn't destroy it.
	bool	ReleaseItem( const T *item )
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

	vsVolatileArrayStoreIterator<T>	Begin() const
	{
		return 		vsVolatileArrayStoreIterator<T>(this,0);
	}

	vsVolatileArrayStoreIterator<T>	End() const
	{
		return 		vsVolatileArrayStoreIterator<T>(this,m_arrayLength);
	}

	T	*GetItem(int id)
	{
		vsAssert(id >= 0 && id < m_arrayLength, "Out of bounds vsVolatileArrayStore access");
		return m_array[id];
	}

	const T	*GetItem(int id) const
	{
		vsAssert(id >= 0 && id < m_arrayLength, "Out of bounds vsVolatileArrayStore access");
		return m_array[id];
	}


	const T	*operator[](int id) const
	{
		return GetItem(id);
	}

	T	*operator[](int id)
	{
		return GetItem(id);
	}
	static const int npos = -1;
};

#endif // VS_VOLATILEARRAYSTORE_H

