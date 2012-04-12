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

#include "VS_Array.h"

template<class T>
class vsPool
{
	vsArray<T*>				m_unusedList;
	
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
		/*
		 *while ( !m_usedList.IsEmpty() )
		 *{
		 *    T* object = *m_usedList.Begin();
		 *    m_usedList.RemoveItem( object );
		 *    vsDelete(object);
		 *}
		 */
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
		
		T* result = *m_unusedList.Begin();
		m_unusedList.RemoveItem(result);
		//m_usedList.AddItem(result);
		
		return result;
	}
	
	void Return( T* item )
	{
		m_unusedCount++;

		//m_usedList.RemoveItem(item);
		m_unusedList.AddItem(item);
	}
	
	//	vsLinkedList<T*> *GetUnusedList()	// intended for any late initialisation that needs to be done by the pool owner.
	//{
	//	return &m_unusedList;
	//}
};

#endif // VS_POOL_H
