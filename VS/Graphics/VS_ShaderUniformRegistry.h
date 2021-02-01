/*
 *  VS_ShaderUniformRegistry.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/07/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SHADERUNIFORMREGISTRY_H
#define VS_SHADERUNIFORMREGISTRY_H

#include "VS/Utils/VS_HashTable.h"

namespace vsShaderUniformRegistry
{
	void Startup();
	void Shutdown();

	int UID( const vsString& uniformName ); // returns or allocates a uid for this uniform
};

#endif // VS_SHADERUNIFORMREGISTRY_H

