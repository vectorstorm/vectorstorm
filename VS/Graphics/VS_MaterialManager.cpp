/*
 *  VS_MaterialManager.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_MaterialManager.h"

vsMaterialManager::vsMaterialManager():
vsCache<vsMaterialInternal>(512)
{
	if ( vsMaterial::White == NULL )
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

