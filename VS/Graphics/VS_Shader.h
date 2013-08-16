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
	uint32_t m_shader;

	int32_t m_alphaRefLoc;
	int32_t m_resolutionLoc;
	int32_t m_globalTimeLoc;
	int32_t m_mouseLoc;

public:

	vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	virtual ~vsShader();

	uint32_t GetShaderId() { return m_shader; }

    void SetAlphaRef( float aref );

	virtual void Prepare(); // called before we start rendering something with this shader
};


#endif // VS_SHADER_H

