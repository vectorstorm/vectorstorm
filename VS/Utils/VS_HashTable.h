/*
 *  VS_HashTable.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 22/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_HASHTABLE_H
#define VS_HASHTABLE_H


uint32_t vsCalculateHash(const char * data, uint32_t len);

template <typename T>
class vsHashEntry
{
public:
	T					m_item;
	vsString			m_key;
	uint32_t				m_keyHash;

	vsHashEntry<T> *	m_next;

	vsHashEntry(): m_key() { m_keyHash = 0, m_next = NULL; }
	vsHashEntry( const T &t, const vsString &key, int keyHash ) : m_item(t) { m_key = key; m_keyHash = keyHash, m_next = NULL; }
};

template <typename T>
class vsHashTable
{
	vsHashEntry<T>		*m_bucket;
	int					m_bucketCount;

	// we're going to need to shift our results to the right to give ourselves
	// the right number of bits to index into a bucket.  Our hashes are 32-bit,
	// so if we have two buckets, we need to shift right by 31 bits.  If we have
	// four buckets, we need to shift right by 30 bits.  And so on.
	int					m_shift;

	uint32_t HashToBucket(uint32_t hash)
	{
		// Fibonocci hash.  We're going to multiply by
		// (uint32_t::max / golden_ratio) (adjusted to be odd),
		// and then shift down to produce the right number of bits.
		//
		const uint32_t factor = 2654435839;
		return (hash * factor) >> m_shift;
	}

	vsHashEntry<T>*		FindHashEntry( const vsString &key )
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = HashToBucket(hash);

		vsHashEntry<T> *ent = m_bucket[bucket].m_next;
		while( ent )
		{
			if ( ent->m_keyHash == hash && ent->m_key == key )
			{
				return ent;
			}
			ent = ent->m_next;
		}
		return NULL;
	}

public:

	vsHashTable(int bucketCount)
	{
		m_bucketCount = vsNextPowerOfTwo(bucketCount);
		m_shift = 32 - vsHighBitPosition(m_bucketCount);

		m_bucket = new vsHashEntry<T>[m_bucketCount];
	}

	~vsHashTable()
	{
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			while ( m_bucket[i].m_next )
			{
				vsHashEntry<T> *toDelete = m_bucket[i].m_next;
				m_bucket[i].m_next = toDelete->m_next;

				vsDelete( toDelete );
			}
		}
		vsDeleteArray( m_bucket );
	}

	void	AddItemWithKey( const T &item, const vsString &key )
	{
		uint32_t hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		vsHashEntry<T> *ent = new vsHashEntry<T>( item, key, hash );

		int bucket = HashToBucket(hash);

		ent->m_next = m_bucket[bucket].m_next;
		m_bucket[bucket].m_next = ent;
	}

	void	RemoveItemWithKey( const T &item, const vsString &key )
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = HashToBucket(hash);
		bool found = false;

		vsHashEntry<T> *ent = &m_bucket[bucket];
		while( ent->m_next )
		{
			if ( ent->m_next->m_keyHash == hash && ent->m_next->m_key == key )
			{
				vsHashEntry<T> *toDelete = ent->m_next;
				ent->m_next = toDelete->m_next;
				found = true;

				vsDelete( toDelete );
				break;
			}
			ent = ent->m_next;
		}
		vsAssert(found, "Error: couldn't find key??");
	}

	T *		FindItem( const vsString &key )
	{
		vsHashEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return &ent->m_item;
		}
		return NULL;
	}

	T& operator[]( const vsString& key )
	{
		T* result = FindItem(key);
		if ( result )
			return *result;
		T newItem;
		AddItemWithKey( newItem, key );
		return *FindItem(key);
	}
};

#endif // VS_HASHTABLE_H

