/*
 *  VS_ShaderUniformRegistry.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/07/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_ShaderUniformRegistry.h"

namespace
{
	vsHashTable<int> *m_uniform = NULL;
	int m_uniformCount = 0;
};

void
vsShaderUniformRegistry::Startup()
{
	m_uniform = new vsHashTable<int>(128);
}

void
vsShaderUniformRegistry::Shutdown()
{
	vsDelete(m_uniform);
}

int
vsShaderUniformRegistry::UID( const vsString& uniformName )
{
	const int *result = m_uniform->FindItem(uniformName);
	if ( result )
	{
		return *result;
	}
	(*m_uniform)[uniformName] = m_uniformCount;
	return m_uniformCount++;
}

