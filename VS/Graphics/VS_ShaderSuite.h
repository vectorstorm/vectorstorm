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
	enum OwnerType
	{
		OwnerType_Game,
		OwnerType_System
	};
protected:
	vsShader *	m_shader[SHADERTYPE_MAX];
	OwnerType m_ownerType;
public:
	vsShaderSuite();
	virtual ~vsShaderSuite();

	virtual void InitShaders(const vsString &vertexShader, const vsString &fragmentShader, OwnerType system);

	vsShader *	GetShader(ShaderType type);
};

#endif /* VS_SHADERSUITE_H */

