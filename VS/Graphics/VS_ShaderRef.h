/*
 *  VS_ShaderRef.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/11/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SHADERREF_H
#define VS_SHADERREF_H

#include "VS_Shader.h"
// #include "VS/Utils/VS_Cache.h"

// [NOTE] TO SELF:
//
// I wanted to hook shaders into the vsCache system, but it turns out that the
// vsCache setup doesn't work with shaders, where I need to map *multiple*
// strings and other settings to an object;  vsCache is pretty strongly
// opinionated about mapping just a single string to a single object.
//
// So in retrospect, no, I'm not going to be able to just wrap vsShader
// in the vsCache system and call it a day;  I'm going to need to architect
// something similar but custom for this particular system.  And that's what
// the vsShaderRef is for;  it's an object that can hold a vsShader pointer
// and lets the owner destroy it, without destroying the shared shader object.
//
// [TODO]:  Needs reference counting on the vsShader object.

class vsShaderRef
{
	vsShader *m_shader;
public:

	vsShaderRef( vsShader *shader );

	vsShader* GetShader() { return m_shader; }
};

#endif // VS_SHADERREF_H

