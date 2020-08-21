/*
 *  VS_ShaderCache.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/11/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SHADERCACHE_H
#define VS_SHADERCACHE_H

#include "VS_Singleton.h"
#include "VS_ShaderRef.h"

namespace vsShaderCache
{
	void Startup();
	void Shutdown();

	bool HasShader( const vsString& name );
	vsShader* GetShader( const vsString& name );
	void AddShader( const vsString& name, vsShader *shader );

	vsShaderRef* LoadShader( const vsString& vFile, const vsString& fFile, bool lit, bool texture, uint32_t optionFlags = 0 );
};

#endif // VS_SHADERCACHE_H

