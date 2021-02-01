/*
 *  VS_Pool.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 4/05/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_POOL_H
#define VS_POOL_H

#include "VS_VolatileArray.h"

template<class T>
class vsPool
{
	vsVolatileArray<T*>				m_unusedList;

	int						m_count;
	int						m_unusedCount;
	bool					m_expandable;

public:

	enum Type
	{
		Type_Static,
		Type_Expandable
	};
	vsPool( int maxCount, Type t = Type_Static ):
		m_count(maxCount),
		m_unusedCount(maxCount),
		m_expandable( (t == Type_Expandable) )
	{
		for ( int i = 0; i < m_count; i++ )
		{
			m_unusedList.AddItem( new T );
		}
	}

	~vsPool()
	{
		vsAssert(m_count ==  m_unusedCount, "Not all instances returned to the pool before pool shutdown??");

		while ( !m_unusedList.IsEmpty() )
		{
			T* object = *m_unusedList.Begin();
			m_unusedList.RemoveItem( object );
			vsDelete(object);
		}
	}

	T*	Borrow()
	{
		if ( !m_expandable )
		{
			vsAssert( m_unusedCount > 0, "No more available!" );
		}
		if ( m_unusedCount <= 0 )
		{
			m_unusedCount++;
			m_count++;
			m_unusedList.AddItem( new T );
		}
		m_unusedCount--;

		T* result = m_unusedList[0];
		m_unusedList.RemoveItem(result);
		//m_usedList.AddItem(result);

		vsAssert( m_unusedCount == m_unusedList.ItemCount(), "Out of sync pool counts??" );
		return result;
	}

	void Return( T* item )
	{
		vsAssert(item != NULL, "Trying to return a NULL item to the pool???");
		m_unusedCount++;

		//m_usedList.RemoveItem(item);
		m_unusedList.AddItem(item);
		vsAssert( m_unusedCount == m_unusedList.ItemCount(), "Out of sync pool counts??" );
	}

	bool IsEmpty()
	{
		return (m_unusedCount == 0 && !m_expandable);
	}
};

#endif // VS_POOL_H
