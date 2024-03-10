/*
 *  VS_TextureManager.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_TextureManager.h"

#include "VS_Texture.h"
#include "VS_Image.h"
#include "VS_TextureInternal.h"
#include "VS_FileCache.h"

vsTextureManager::vsTextureManager():
	vsCache<vsTextureInternal>(512)
{
}

vsTextureInternal *
vsTextureManager::LoadTexture( const vsString &filename )
{
	return Get( filename );
}

void
vsTextureManager::ReloadAll()
{
	// get rid of any textures we don't need any more, so we're not reloading
	// anything we don't actually need right now.
	CollectGarbage();

	// Also, we need to clear the file cache, or else we might just re-use data
	// from memory
	vsFileCache::Purge();

	vsScopedLock lock( m_mutex );
	for ( int b = 0; b < m_bucketCount; b++ )
	{
		vsCacheEntry< vsTextureInternal > *bucket = &m_bucket[b];
		while( bucket )
		{
			if ( bucket->m_item )
			{
				vsTextureInternal *m = static_cast<vsTextureInternal*>(bucket->m_item);
				m->Reload();
			}

			bucket = bucket->m_next;
		}
	}
}

