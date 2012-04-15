/*
 *  VS_Shader.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 1/03/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SHADER_H
#define VS_SHADER_H

class vsDisplayList;

#include "VS_Color.h"
#include "VS/Math/VS_Vector.h"

class vsShader
{
	uint32 m_shader;

public:

	vsShader( const vsString &vertexShader, const vsString &fragmentShader );
	virtual ~vsShader();

	uint32 GetShaderId() { return m_shader; }

	virtual void Prepare() {}	// called before we start rendering something with this shader
};


#endif // VS_SHADER_H

