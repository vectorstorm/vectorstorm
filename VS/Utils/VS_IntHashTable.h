/*
 *  VS_IntHashTable.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/07/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_INTHASHTABLE_H
#define VS_INTHASHTABLE_H

#include "VS/Utils/VS_Debug.h"
#include "VS/Math/VS_Math.h"

uint32_t vsCalculateIntHash(uint32_t key);

template <typename T>
class vsIntHashEntry
{
public:
	T					m_item;
	uint32_t			m_key;

	vsIntHashEntry<T> *	m_next;

	vsIntHashEntry(): m_key(0), m_next(NULL) {}
	vsIntHashEntry( const T &t, uint32_t key ) : m_item(t), m_key(key), m_next(NULL) {}
};

template <typename T>
class vsIntHashTable
{
	vsIntHashEntry<T>		*m_bucket;
	int					m_bucketCount;

	// we're going to need to shift our results to the right to give ourselves
	// the right number of bits to index into a bucket.  Our hashes are 32-bit,
	// so if we have two buckets, we need to shift right by 31 bits.  If we have
	// four buckets, we need to shift right by 30 bits.  And so on.
	int					m_shift;

	uint32_t HashToBucket(uint32_t hash) const
	{
		// Fibonocci hash.  We're going to multiply by
		// (uint32_t::max / golden_ratio) (adjusted to be odd),
		// and then shift down to produce the right number of bits.
		//
		const uint32_t factor = 2654435839U;
		return (hash * factor) >> m_shift;
	}

	const vsIntHashEntry<T>*		FindHashEntry(uint32_t key) const
	{
		uint32_t  hash = vsCalculateIntHash(key);
		int bucket = HashToBucket(hash);

		vsIntHashEntry<T> *ent = m_bucket[bucket].m_next;
		while( ent )
		{
			if ( ent->m_key == hash && ent->m_key == key )
			{
				return ent;
			}
			ent = ent->m_next;
		}
		return NULL;
	}

	vsIntHashEntry<T>*		FindHashEntry(uint32_t key)
	{
		uint32_t  hash = vsCalculateIntHash(key);
		int bucket = HashToBucket(hash);

		vsIntHashEntry<T> *ent = m_bucket[bucket].m_next;
		while( ent )
		{
			if ( ent->m_key == hash && ent->m_key == key )
			{
				return ent;
			}
			ent = ent->m_next;
		}
		return NULL;
	}

public:

	vsIntHashTable(int bucketCount)
	{
		m_bucketCount = vsNextPowerOfTwo(bucketCount);
		m_shift = 32 - vsHighBitPosition(m_bucketCount);

		m_bucket = new vsIntHashEntry<T>[m_bucketCount];
	}

	~vsIntHashTable()
	{
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			while ( m_bucket[i].m_next )
			{
				vsIntHashEntry<T> *toDelete = m_bucket[i].m_next;
				m_bucket[i].m_next = toDelete->m_next;

				vsDelete( toDelete );
			}
		}
		vsDeleteArray( m_bucket );
	}

	void	AddItemWithKey( const T &item, uint32_t key)
	{
		vsIntHashEntry<T> *ent = new vsIntHashEntry<T>( item, key );

		int bucket = HashToBucket(key);

		ent->m_next = m_bucket[bucket].m_next;
		m_bucket[bucket].m_next = ent;
	}

	void	RemoveItemWithKey( const T &item, uint32_t key)
	{
		uint32_t  hash = vsCalculateIntHash(key);
		int bucket = HashToBucket(hash);
		bool found = false;

		vsIntHashEntry<T> *ent = &m_bucket[bucket];
		while( ent->m_next )
		{
			if ( ent->m_next->m_key == hash && ent->m_next->m_key == key )
			{
				vsIntHashEntry<T> *toDelete = ent->m_next;
				ent->m_next = toDelete->m_next;
				found = true;

				vsDelete( toDelete );
				break;
			}
			ent = ent->m_next;
		}
		vsAssert(found, "Error: couldn't find key??");
	}

	const T *		FindItem( const uint32_t key ) const
	{
		const vsIntHashEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return &ent->m_item;
		}
		return NULL;
	}

	T *		FindItem( uint32_t key )
	{
		vsIntHashEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return &ent->m_item;
		}
		return NULL;
	}

	T& operator[]( uint32_t key )
	{
		T* result = FindItem(key);
		if ( result )
			return *result;
		T newItem;
		AddItemWithKey( newItem, key );
		return *FindItem(key);
	}

	int GetHashEntryCount() const
	{
		int count = 0;
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			vsIntHashEntry<T>* shuttle = m_bucket[i].m_next;
			while ( shuttle )
			{
				count++;
				shuttle = shuttle->m_next;
			}
		}
		return count;
	}

	// This 'i' value is NOT CONSTANT.  As things are added and removed
	// to this hash table, these hash entries can move around.  Only useful
	// for internal inspection of the hash table
	const vsIntHashEntry<T>* GetHashEntry(int i) const
	{
		int count = i;
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			vsIntHashEntry<T>* shuttle = m_bucket[i].m_next;
			while ( shuttle )
			{
				if ( count == 0 )
					return shuttle;
				count--;
				shuttle = shuttle->m_next;
			}
		}
		return NULL;
	}

	bool operator==( const vsIntHashTable<T>& other ) const
	{
		if ( m_bucketCount != other.m_bucketCount )
			return false;
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			vsIntHashEntry<T>* shuttle = m_bucket[i].m_next;
			vsIntHashEntry<T>* oshuttle = other.m_bucket[i].m_next;
			while( shuttle && oshuttle )
			{
				if ( shuttle->m_item != oshuttle->m_item ||
						shuttle->m_key != oshuttle->m_key )
					return false;
				shuttle = shuttle->m_next;
				oshuttle = oshuttle->m_next;
			}
			// now if we didn't reach the end of the bucket at the same time on
			// both, that's also a failure
			if ( shuttle || oshuttle )
				return false;
		}
		return true;
	}

};

#endif // VS_INTHASHTABLE_H

