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

#include "VS_HashTable.h"
#include "VS/Utils/VS_Singleton.h"

template <typename T> class vsCache;

class vsResource
{
	vsString		m_name;
	int				m_refCount;

public:

						vsResource( const vsString &name ) { m_name = name; m_refCount = 0; }
	virtual				~vsResource()
	{
		vsAssert( GetReferenceCount() == 0, vsFormatString("Error:  references to %s still exist??", m_name.c_str()) );
	}

	void				AddReference()	{ m_refCount++; }
	void				ReleaseReference()	{ m_refCount--; }
	int					GetReferenceCount() { return m_refCount; }

	const vsString &	GetName() const { return m_name; }
};

template <typename T>
class vsCacheReference
{
protected:
	vsResource *	m_resource;

	vsCacheReference() {}

public:

	vsCacheReference( const vsString &name ) { m_resource = vsCache<T>::Instance()->Get(name);  m_resource->AddReference(); }
	vsCacheReference( vsResource *resource ) { m_resource = resource; m_resource->AddReference(); }
	vsCacheReference( vsCacheReference<T> *other ) { m_resource = other->m_resource; m_resource->AddReference(); }
	virtual ~vsCacheReference() { m_resource->ReleaseReference(); }

	T*		GetResource() const { return reinterpret_cast<T*>(m_resource); }
	virtual void operator=(const vsCacheReference<T> &b)
	{
		// add before release, just because that's always safer.
		b.m_resource->AddReference();
		m_resource->ReleaseReference();
		m_resource = b.m_resource;
	}
};


// "T" must be derived from the vsResource class, above
template <typename T>
class vsCacheEntry
{
public:
	vsResource *		m_item;

	vsString			m_key;
	uint32				m_keyHash;

	vsCacheEntry<T> *	m_next;

	vsCacheEntry() { m_item = NULL; m_key = vsEmptyString; m_keyHash = 0; m_next = NULL; }
	vsCacheEntry( vsCache<T> *cache, T * object, const vsString &key, int keyHash ) { m_item = object; m_key = key; m_keyHash = keyHash, m_next = NULL; }

	~vsCacheEntry()
	{
		vsDelete( m_item );
	}

	T *		GetItem()		{ return reinterpret_cast<T*>(m_item); }
};

template <typename T>
class vsCache : public vsSingleton< vsCache<T> >
{
	vsCacheEntry<T>		*m_bucket;
	int					m_bucketCount;

	vsCacheEntry<T>*		FindHashEntry( const vsString &key )
	{
		uint32  hash = vsCalculateHash(key.c_str(), (uint32)key.length());
		int bucket = hash % m_bucketCount;

		vsCacheEntry<T> *ent = m_bucket[bucket].m_next;
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


	void	Remove( T* item )
	{
		vsString &key = item->GetName();
		uint32  hash = vsCalculateHash(key.c_str(), (uint32)key.length());
		int bucket = hash % m_bucketCount;

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
		return NULL;
	}

public:

	vsCache(int bucketCount)
	{
		m_bucketCount = bucketCount;

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
		uint32 hash = vsCalculateHash(key.c_str(), (uint32)key.length());
		vsCacheEntry<T> *ent = new vsCacheEntry<T>( this, item, key, hash );

		int bucket = hash % m_bucketCount;

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

	void	Release( T* object )
	{
		vsCacheEntry<T> *ce = Find( object->GetName() );
		vsAssert(ce, "Error:  released object wasn't actually in cache??");
		if ( ce )
		{
			ce->ReleaseReference();
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

