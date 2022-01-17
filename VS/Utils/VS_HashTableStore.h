/*
 *  VS_HashTableStore.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 24/09/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */


#ifndef VS_HASHTABLESTORE_H
#define VS_HASHTABLESTORE_H

#include "VS_HashTable.h"

template <typename T>
class vsHashStoreEntry
{
public:
	T *					m_item;
	vsString			m_key;
	uint32_t				m_keyHash;

	vsHashStoreEntry<T> *	m_next;

	vsHashStoreEntry() : m_item(nullptr) { m_key = vsEmptyString; m_keyHash = 0, m_next = nullptr; }
	vsHashStoreEntry( T *t, const vsString &key, int keyHash ) : m_item(t) { m_key = key; m_keyHash = keyHash, m_next = nullptr; }
};

template <typename T>
class vsHashTableStore
{
	vsHashStoreEntry<T>		*m_bucket;
	int					m_bucketCount;

	vsHashStoreEntry<T>*		FindHashEntry( const vsString &key )
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = hash % m_bucketCount;

		vsHashStoreEntry<T> *ent = m_bucket[bucket].m_next;
		while( ent )
		{
			if ( ent->m_keyHash == hash && ent->m_key == key )
			{
				return ent;
			}
			ent = ent->m_next;
		}
		return nullptr;
	}

	const vsHashStoreEntry<T>*		FindHashEntry( const vsString &key ) const
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = hash % m_bucketCount;

		vsHashStoreEntry<T> *ent = m_bucket[bucket].m_next;
		while( ent )
		{
			if ( ent->m_keyHash == hash && ent->m_key == key )
			{
				return ent;
			}
			ent = ent->m_next;
		}
		return nullptr;
	}

public:

	vsHashTableStore(int bucketCount)
	{
		m_bucketCount = bucketCount;

		m_bucket = new vsHashStoreEntry<T>[m_bucketCount];
	}

	~vsHashTableStore()
	{
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			while ( m_bucket[i].m_next )
			{
				vsHashStoreEntry<T> *toDelete = m_bucket[i].m_next;
				m_bucket[i].m_next = toDelete->m_next;

				vsDelete( toDelete->m_item );
				vsDelete( toDelete );
			}
		}
		vsDeleteArray( m_bucket );
	}

	void	AddItemWithKey( T* item, const vsString &key )
	{
		uint32_t hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		vsHashStoreEntry<T> *ent = new vsHashStoreEntry<T>( item, key, hash );

		int bucket = hash % m_bucketCount;

		ent->m_next = m_bucket[bucket].m_next;
		m_bucket[bucket].m_next = ent;
	}

	void	RemoveItemWithKey( T* item, const vsString &key )
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = hash % m_bucketCount;
		bool found = false;

		vsHashStoreEntry<T> *ent = &m_bucket[bucket];
		while( ent->m_next )
		{
			if ( ent->m_next->m_keyHash == hash && ent->m_next->m_key == key )
			{
				vsHashStoreEntry<T> *toDelete = ent->m_next;
				ent->m_next = toDelete->m_next;
				found = true;

				vsDelete( toDelete->m_item );
				vsDelete( toDelete );
				break;
			}
			ent = ent->m_next;
		}
		vsAssert(found, "Error: couldn't find key??");
	}

	T *		FindItem( const vsString &key )
	{
		vsHashStoreEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return ent->m_item;
		}
		return nullptr;
	}

	const T *		FindItem( const vsString &key ) const
	{
		const vsHashStoreEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return ent->m_item;
		}
		return nullptr;
	}
};

#endif // VS_HASHTABLE_H

