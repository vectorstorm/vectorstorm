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

template<class T> class vsArray;

template<class T>
class vsArrayIterator
{
	const vsArray<T> *		m_parent;
	int						m_current;

public:

	vsArrayIterator( const vsArray<T> *array, int initial ): m_parent(array), m_current(initial) {}

	T				Get() { return m_parent->Get(*this); }

	bool			Next() { m_current++; return (m_current < m_parent->ItemCount() ); }
	bool			Prev() { m_current--; return (m_current >= 0); }
	bool						operator==( const vsArrayIterator &b ) { return (m_current == b.m_current && m_parent == b.m_parent ); }
	bool						operator!=( const vsArrayIterator &b ) { return !((*this)==b); }
	vsArrayIterator<T>&		operator++() { Next(); return *this; }
	vsArrayIterator<T>		operator++(int postFix) { vsArrayIterator<T> other(m_parent, m_current); Next(); return other; }

	vsArrayIterator<T>&		operator--() { Prev(); return *this; }
	vsArrayIterator<T>		operator--(int postFix) { vsArrayIterator<T> other(m_parent, m_current); Prev(); return other; }

	friend class vsArray<T>;
};

template<class T>T	operator*(vsArrayIterator<T> i) { return i.Get(); }

template<class T>
class vsArray
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

	typedef vsArrayIterator<T> Iterator;

	vsArray( int initialStorage = 4 )
	{
		m_array = new T[ initialStorage ];
		m_arrayLength = 0;
		m_arrayStorage = initialStorage;
	}

	virtual ~vsArray()
	{
		vsDeleteArray( m_array );
	}

	T		Get( const vsArrayIterator<T> &iter ) const
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

	T	&GetItem(int id)
	{
		vsAssert(id >= 0 && id < m_arrayLength, "Out of bounds vsArray access");
		return m_array[id];
	}

	T	&operator[](int id)
	{
		return GetItem(id);
	}

	static const int npos = -1;
};

#endif // VS_ARRAY_H

