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
#include "VS/Utils/VS_Cache.h"

// [NOTE] TO SELF:  NO!
//
// This approach isn't going to work;  the "vsCache" system is built around
// the idea that a name maps to an object, and that doesn't work with shaders,
// where I need to map *multiple* strings and other settings to an object.
//
// So in retrospect, no, I'm not going to be able to just wrap this whole thing
// in the vsCache system and call it a day;  I'm going to need to architect
// something similar but custom for this particular system.

class vsShaderRef/*: public vsCacheReference<vsShader>*/
{
	vsShader *m_shader;
public:

	// vsShaderRef( const vsString& name );
	vsShaderRef( vsShader *shader );

	vsShader* GetShader() { return m_shader; }
};

#endif // VS_SHADERREF_H

