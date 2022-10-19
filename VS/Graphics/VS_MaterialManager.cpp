/*
 *  VS_MaterialManager.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_MaterialManager.h"
#include "VS_Material.h"
#include "VS_Shader.h"
#include "VS_FileCache.h"

vsMaterialManager::vsMaterialManager():
vsCache<vsMaterialInternal>(512)
{
	if ( vsMaterial::White == nullptr )
	{
		vsMaterial::White = new vsMaterial("White");
	}
}

vsMaterialManager::~vsMaterialManager()
{
	vsDelete( vsMaterial::White );
}

vsMaterialInternal *
vsMaterialManager::LoadMaterial( const vsString &name )
{
	return Get( name );
}

void
vsMaterialManager::ReloadAll()
{
	// get rid of any materials we don't need any more, so we're not reloading
	// anything we don't actually need right now.
	CollectGarbage();

	// Also, we need to clear the file cache, or else we'll just re-use data
	// from memory
	vsFileCache::Purge();

	// Also, we need to reload all the shaders, since we're not loading them
	// directly any more.
	vsShader::ReloadAll();

	for ( int b = 0; b < m_bucketCount; b++ )
	{
		vsCacheEntry< vsMaterialInternal > *bucket = &m_bucket[b];
		while( bucket )
		{
			if ( bucket->m_item )
			{
				vsMaterialInternal *m = static_cast<vsMaterialInternal*>(bucket->m_item);
				m->Reload();
			}

			bucket = bucket->m_next;
		}
	}
}

