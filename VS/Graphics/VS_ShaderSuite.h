/*
 *  VS_ShaderSuite.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 20-06-2012.
 *  Copyright 2012 Trevor Powell.  All rights reserved.
 *
 */
#ifndef VS_SHADERSUITE_H
#define VS_SHADERSUITE_H

class vsShader;

class vsShaderSuite
{
public:
	enum ShaderType
	{
		Normal,
		NormalTex,
		Lit,
		LitTex,
		SHADERTYPE_MAX
	};
protected:
	vsShader *	m_shader[SHADERTYPE_MAX];
public:
	vsShaderSuite();
	virtual ~vsShaderSuite();

	virtual void InitShaders(const vsString &vertexShader, const vsString &fragmentShader);

	vsShader *	GetShader(ShaderType type);
};

#endif /* VS_SHADERSUITE_H */

