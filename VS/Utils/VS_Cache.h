/*
 *  VS_Cache.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_CACHE_H
#define VS_CACHE_H

// #include "VS_HashTable.h"
#include "VS/Utils/VS_Singleton.h"
#include "VS/Utils/VS_Debug.h"

template <typename T> class vsCache;

class vsResource
{
	vsString		m_name;
	int				m_refCount;
	bool			m_transient; // if true, we get destroyed immediately if our refcount reaches 0.

public:

						vsResource( const vsString &name ) { m_name = name; m_refCount = 0; m_transient = false; }
	virtual				~vsResource()
	{
		if ( GetReferenceCount() != 0 )
			vsLog("Error:  %d references to %s still exist.", GetReferenceCount(), m_name.c_str());
	}

	void				SetTransient() { m_transient = true; }

	void				AddReference()	{ m_refCount++; }
	void				ReleaseReference()	{ m_refCount--; vsAssert( m_refCount >= 0, "Refcount negative??" ); }
	int					GetReferenceCount() const { return m_refCount; }
	bool				IsTransient() const { return m_transient; }

	const vsString &	GetName() const { return m_name; }
};

template <typename T>
class vsCacheReference
{
protected:
	T *	m_resource;

	vsCacheReference() {}

public:

	vsCacheReference( const vsString &name ) { m_resource = vsCache<T>::Instance()->Get(name);  m_resource->AddReference(); }
	vsCacheReference( T *resource ) { m_resource = resource; vsCache<T>::Instance()->Add(resource); m_resource->AddReference(); }
	vsCacheReference( const vsCacheReference<T> *other ) { m_resource = other->m_resource; m_resource->AddReference(); }
	virtual ~vsCacheReference() { m_resource->ReleaseReference(); }

	T*		GetResource() const { return m_resource; }
	void operator=(const vsCacheReference<T> &b)
	{
		// add before release, just because that's always safer.
		b.m_resource->AddReference();
		m_resource->ReleaseReference();
		m_resource = b.m_resource;
	}

	const vsString &	GetName() const { return m_resource->GetName(); }
};


// "T" must be derived from the vsResource class, above
template <typename T>
class vsCacheEntry
{
public:
	T *		m_item;

	vsString			m_key;
	uint32_t				m_keyHash;

	vsCacheEntry<T> *	m_next;

	vsCacheEntry() { m_item = nullptr; m_key = vsEmptyString; m_keyHash = 0; m_next = nullptr; }
	vsCacheEntry( T * object, const vsString &key, int keyHash ) { m_item = object; m_key = key; m_keyHash = keyHash, m_next = nullptr; }

	~vsCacheEntry()
	{
		vsDelete( m_item );
	}

	T *		GetItem()		{ return m_item; }
};

template <typename T>
class vsCache : public vsSingleton< vsCache<T> >
{
protected:
	vsCacheEntry<T>		*m_bucket;
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
		const uint32_t factor = 2654435839U;
		return (hash * factor) >> m_shift;
	}

	vsCacheEntry<T>*		FindHashEntry( const vsString &key )
	{
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = HashToBucket(hash);

		vsCacheEntry<T> *ent = m_bucket[bucket].m_next;
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


	void	Remove( T* item )
	{
		vsString &key = item->GetName();
		uint32_t  hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		int bucket = HashToBucket(hash);

		vsCacheEntry<T> *ent = &m_bucket[bucket];
		while( ent->m_next )
		{
			if ( ent->m_next->m_keyHash == hash && ent->m_next->m_key == key )
			{
				vsCacheEntry<T> *toDelete = ent->m_next;
				ent->m_next = toDelete->m_next;
				vsDelete( toDelete );
				break;
			}
		}
	}

	vsCacheEntry<T> *	Find( const vsString &key )
	{
		vsCacheEntry<T> *ent = FindHashEntry(key);
		if ( ent )
		{
			return ent;
		}
		return nullptr;
	}

public:

	vsCache(int bucketCount)
	{
		m_bucketCount = vsNextPowerOfTwo(bucketCount);
		m_shift = 32 - vsHighBitPosition(m_bucketCount);

		m_bucket = new vsCacheEntry<T>[m_bucketCount];
	}

	~vsCache()
	{
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			while ( m_bucket[i].m_next )
			{
				vsCacheEntry<T> *toDelete = m_bucket[i].m_next;
				m_bucket[i].m_next = toDelete->m_next;

				vsDelete( toDelete );
			}
		}
		vsDeleteArray( m_bucket );
	}

	void	Add( T* item )
	{
		const vsString &key = item->GetName();
		uint32_t hash = vsCalculateHash(key.c_str(), (uint32_t)key.length());
		vsCacheEntry<T> *ent = new vsCacheEntry<T>( item, key, hash );

		int bucket = HashToBucket(hash);

		ent->m_next = m_bucket[bucket].m_next;
		m_bucket[bucket].m_next = ent;
	}

	T *	Get( const vsString &name )
	{
		vsCacheEntry<T> *ce = Find( name );
		if ( ce )
		{
			return ce->GetItem();
		}
		else
		{
			T *object = new T(name);
			Add( object );
			return Get( name );
		}
	}

	// returns true if we have this item in the cache, false otherwise.
	bool Exists( const vsString &name )
	{
		return (nullptr != Find( name ));
	}

	void	Release( T* object )
	{
		vsCacheEntry<T> *ce = Find( object->GetName() );
		vsAssert(ce, "Error:  released object wasn't actually in cache??");
		if ( ce )
		{
			ce->ReleaseReference();

			if ( ce->m_item->IsTransient() &&
					ce->m_item->GetReferenceCount() == 0 )
			{
				Remove( object );
			}
		}
	}

	void	CollectGarbage()
	{
		for ( int i = 0; i < m_bucketCount; i++ )
		{
			vsCacheEntry<T> *ent = &m_bucket[i];
			while( ent->m_next )
			{
				vsCacheEntry<T> *toDelete = ent->m_next;
				if ( toDelete->m_item->GetReferenceCount() == 0 )
				{
					ent->m_next = toDelete->m_next;

					vsDelete( toDelete );
				}
				else
				{
					ent = ent->m_next;
				}
			}
		}
	}

	/*
	void	Add( T* object )
	{
		vsCacheEntry<T> *ce = m_table.FindItem( object->GetName() );
		vsAssert(!ce, "Error:  released object wasn't actually in cache??");

		m_table.AddItemWithKey( new vsCacheEntry<T>(object), object->GetName() );
	}*/
};

#endif // VS_CACHE_H

