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

#include "VS/Utils/VS_Debug.h"
#include "VS/Math/VS_Math.h"

uint32_t vsCalculateHash(const char * data, uint32_t len);

template <typename T>
class vsHashEntry
{
public:
	T					m_item;
	vsString			m_key;
	uint32_t			m_keyHash;
	bool				m_used;

	vsHashEntry(): m_key(), m_keyHash(0), m_used(false) {}
	vsHashEntry( const T &t, const vsString &key, int keyHash ) :
		m_item(t),
		m_key(key),
		m_keyHash(keyHash),
		m_used(false)
	{
	}
	vsHashEntry( const vsHashEntry& o ) :
		m_item(o.m_item),
		m_key(o.m_key),
		m_keyHash(o.m_keyHash),
		m_used(o.m_used)
	{
	}

	void Set( const T& item, const vsString& key, const uint32_t hash )
	{
		m_item = item;
		m_key = key;
		m_keyHash = hash;
		m_used = true;
	}

	bool IsEmpty() const { return m_keyHash == 0; }
	bool IsTombstone() const { return m_keyHash == 0 && m_used; }
	bool IsFull() const { return m_keyHash != 0; }
	bool IsFullOrTombstone() const { return m_keyHash != 0 || m_used; }

	void operator=(const vsHashEntry<T>& o)
	{
		m_item = o.m_item;
		m_key = o.m_key;
		m_keyHash = o.m_keyHash;
		m_used = o.m_used;
	}
};

template <typename T>
class vsHashTable
{
	vsHashEntry<T>		*m_bucket;
	int					m_bucketCount;

	int					m_filledBucketCount;
	int					m_tombstoneCount;

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

	const vsHashEntry<T>*		FindHashEntry( const vsString &key ) const
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int startBucket = HashToBucket(hash);

		for ( int i = 0; i < m_bucketCount; i++ )
		{
			int index = (startBucket+i)%m_bucketCount;

			const vsHashEntry<T> &ent = m_bucket[index];
			if ( ent.m_keyHash == hash && ent.m_key == key )
				return &ent;

			if ( ent.IsEmpty() )
				break;
		}
		return nullptr;
	}

	vsHashEntry<T>*		FindHashEntry( const vsString &key )
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int startBucket = HashToBucket(hash);

		for ( int i = 0; i < m_bucketCount; i++ )
		{
			int index = (startBucket+i)%m_bucketCount;

			vsHashEntry<T> &ent = m_bucket[index];
			if ( ent.m_keyHash == hash && ent.m_key == key )
				return &ent;

			if ( ent.IsEmpty() )
				break;
		}
		return nullptr;
	}

	void _ConsiderResize()
	{
		if ( ((m_filledBucketCount+m_tombstoneCount) / (float)m_bucketCount) > 0.7f )
		{
			// resize ourself!
			Resize( m_bucketCount+1 ); // this will bump us to the next power of two
		}
	}

public:

	vsHashTable(int bucketCount)
	{
		m_bucketCount = vsNextPowerOfTwo(bucketCount);
		m_shift = 32 - vsHighBitPosition(m_bucketCount);
		m_filledBucketCount = 0;
		m_tombstoneCount = 0;

		m_bucket = new vsHashEntry<T>[m_bucketCount];
	}

	vsHashTable(const vsHashTable& other):
		m_bucketCount( other.m_bucketCount ),
		m_shift( other.m_shift )
	{
		m_bucket = new vsHashEntry<T>[m_bucketCount];
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			m_bucket[i] = other.m_bucket[i];
		}
		m_filledBucketCount = other.m_filledBucketCount;
		m_tombstoneCount = other.m_tombstoneCount;
	}

	~vsHashTable()
	{
		vsDeleteArray( m_bucket );
	}

	void Resize( int newBucketCount )
	{
		// first, kill our old buckets
		vsHashEntry<T>		*oldBucket = m_bucket;
		int oldBucketCount = m_bucketCount;

		m_bucketCount = vsNextPowerOfTwo(newBucketCount);
		m_shift = 32 - vsHighBitPosition(m_bucketCount);

		vsLog("Resizing vsHashTable from %d to %d buckets", oldBucketCount, m_bucketCount);

		m_bucket = new vsHashEntry<T>[m_bucketCount];

		m_filledBucketCount = 0;
		m_tombstoneCount = 0;
		for ( int i = 0; i < oldBucketCount; i++ )
		{
			if ( !oldBucket[i].IsEmpty() )
				AddItemWithKey( oldBucket[i].m_item, oldBucket[i].m_key );
		}

		vsDeleteArray(oldBucket);
	}


	void Clear()
	{
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			m_bucket[i].m_keyHash = 0;
			m_bucket[i].m_used = false;
		}
		m_filledBucketCount = 0;
		m_tombstoneCount = 0;
	}

	void	AddItemWithKey( const T &item, const vsString &key )
	{
		_ConsiderResize();

		uint32_t hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		// vsHashEntry<T> *ent = new vsHashEntry<T>( item, key, hash );

		int bucket = HashToBucket(hash);

		for ( int i = 0; i < m_bucketCount; i++ )
		{
			int index = (bucket+i)%m_bucketCount;
			if ( m_bucket[index].IsEmpty() )
			{
				m_bucket[index].Set(item,key,hash);
				m_filledBucketCount++;
				break;
			}
		}

	}

	void	RemoveItemWithKey( const T &item, const vsString &key )
	{
		// [TODO] We should verify that the element we remove is actually this item!
		UNUSED(item);
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = HashToBucket(hash);
		bool found = false;

		for ( int i = 0; i < m_bucketCount; i++ )
		{
			int index = (bucket+i)%m_bucketCount;
			if ( !m_bucket[index].IsFullOrTombstone() )
			{
				break; // we don't have it!
			}
			if ( m_bucket[index].m_keyHash == hash && m_bucket[index].m_key == key )
			{
				m_bucket[index].m_keyHash = 0;
				m_filledBucketCount--;
				m_tombstoneCount++;
				return;
			}
		}

		vsAssert(found, "Error: couldn't find key??");
	}

	const T *		FindItem( const vsString &key ) const
	{
		const vsHashEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return &ent->m_item;
		}
		return nullptr;
	}

	T *		FindItem( const vsString &key )
	{
		vsHashEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return &ent->m_item;
		}
		return nullptr;
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

	int GetHashEntryCount() const
	{
		return m_filledBucketCount;
	}

	// This 'i' value is NOT CONSTANT.  As things are added and removed
	// to this hash table, these hash entries can move around.  Only useful
	// for internal inspection of the hash table
	const vsHashEntry<T>* GetHashEntry(int i) const
	{
		int count = i;
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			vsHashEntry<T>* elem = &m_bucket[i];
			if ( elem->IsFull() )
			{
				if ( count == 0 )
					return elem;
				count--;
			}
		}
		return nullptr;
	}

	bool operator==( const vsHashTable<T>& other ) const
	{
		if ( m_bucketCount != other.m_bucketCount )
			return false;
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			vsHashEntry<T>* shuttle = &m_bucket[i];
			vsHashEntry<T>* oshuttle = &other.m_bucket[i];
			if ( shuttle->IsFull() != oshuttle->IsFull() )
				return false;
			if ( shuttle->IsFull() && shuttle->m_item != oshuttle->m_item )
				return false;
		}
		return true;
	}

	void operator=( const vsHashTable<T>& other )
	{
		Clear();
		Resize( other.m_bucketCount );
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			if ( other.m_bucket[i].IsFull() )
				m_bucket[i] = other.m_bucket[i];
		}
	}

};

#endif // VS_HASHTABLE_H

