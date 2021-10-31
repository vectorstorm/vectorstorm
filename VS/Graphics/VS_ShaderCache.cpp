/*
 *  VS_ShaderCache.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/11/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_ShaderCache.h"
#include "VS_ShaderRef.h"

#include "VS_TimerSystem.h"

namespace
{
	vsHashTable<vsShader*> *m_cache = nullptr;
};

// [TODO]:  This implementation uses a hash table right now, which doesn't
// correctly clean up after the objects in it.  This was done for rapidity of
// prototyping, but needs to be moved to something better which can actually
// clean up, if I decide I like this approach!

void
vsShaderCache::Startup()
{
	m_cache = new vsHashTable<vsShader*>(32);
}

void
vsShaderCache::Shutdown()
{
	vsDelete(m_cache);
}

vsShaderRef*
vsShaderCache::LoadShader( const vsString& vFile, const vsString& fFile, bool lit, bool texture, uint32_t optionFlags )
{
	vsString uniqueName = vFile + "_" + fFile;
	if ( lit )
		uniqueName += "L";
	if ( texture )
		uniqueName += "T";
	if ( optionFlags )
		uniqueName += vsFormatString("_%d", optionFlags);

	// static int caches = 0;
	// static int loads = 0;

	vsShader *shader = nullptr;
	if ( HasShader(uniqueName) )
	{
		// caches++;
		// unsigned long before = vsTimerSystem::Instance()->GetMicroseconds();
		shader = GetShader(uniqueName);
		// unsigned long after = vsTimerSystem::Instance()->GetMicroseconds();
		// vsLog("Retrieved cached shader [%d] '%s', '%s', %d, %d: %f milliseconds", caches, vFile, fFile, lit, texture, (after-before)/1000.f);
		// vsLog("Serving shader from cache");
	}
	else
	{
		// loads++;
		// unsigned long before = vsTimerSystem::Instance()->GetMicroseconds();
		shader = vsShader::Load( vFile, fFile, lit, texture );
		// unsigned long after = vsTimerSystem::Instance()->GetMicroseconds();
		// vsLog("Loading shader [%d] '%s', '%s', %d, %d: %f milliseconds", loads, vFile, fFile, lit, texture, (after-before)/1000.f);
		AddShader( uniqueName, shader );
	}

	vsShaderRef *ref = new vsShaderRef(shader);
	return ref;
}

bool
vsShaderCache::HasShader( const vsString& name )
{
	if ( m_cache->FindItem(name) )
		return true;
	return false;
}

vsShader*
vsShaderCache::GetShader( const vsString& name )
{
	vsShader** result = m_cache->FindItem(name);

	if ( result )
		return *result;
	return nullptr;
}

void
vsShaderCache::AddShader( const vsString& name, vsShader *shader )
{
	m_cache->AddItemWithKey(shader, name);
}

