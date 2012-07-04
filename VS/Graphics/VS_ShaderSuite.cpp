/*
 *  VS_ShaderSuite.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 20-06-2012.
 *  Copyright 2012 Trevor Powell.  All rights reserved.
 *
 */
#include "VS_ShaderSuite.h"
#include "VS_Shader.h"

vsShaderSuite::vsShaderSuite()
{
	for ( int i = 0; i < SHADERTYPE_MAX; i++ )
	{
		m_shader[i] = NULL;
	}
}

vsShaderSuite::~vsShaderSuite()
{
	for ( int i = 0; i < SHADERTYPE_MAX; i++ )
	{
		delete m_shader[i];
	}
}

void
vsShaderSuite::InitShaders(const vsString &vertexShader, const vsString &fragmentShader)
{
	m_shader[Normal] = new vsShader(vertexShader, fragmentShader, false, false);
	m_shader[NormalTex] = new vsShader(vertexShader, fragmentShader, false, true);
	m_shader[Lit] = new vsShader(vertexShader, fragmentShader, true, false);
	m_shader[LitTex] = new vsShader(vertexShader, fragmentShader, true, true);
}

vsShader *
vsShaderSuite::GetShader(ShaderType type)
{
	return m_shader[type];
}

