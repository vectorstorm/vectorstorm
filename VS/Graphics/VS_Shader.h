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
#include "VS_MaterialInternal.h"
class vsMatrix4x4;

class vsShader
{
	uint32_t m_shader;

	int32_t m_alphaRefLoc;
	int32_t m_resolutionLoc;
	int32_t m_globalTimeLoc;
	int32_t m_mouseLoc;
	int32_t m_fogLoc;
	int32_t m_textureLoc;
	int32_t m_localToWorldLoc;
	int32_t m_worldToViewLoc;
	int32_t m_viewToProjectionLoc;

public:

	vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	virtual ~vsShader();

	uint32_t GetShaderId() { return m_shader; }

    void SetAlphaRef( float aref );
	void SetFog( bool fog );
	void SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] );
	void SetLocalToWorld( const vsMatrix4x4& localToWorld );
	void SetWorldToView( const vsMatrix4x4& worldToView );
	void SetViewToProjection( const vsMatrix4x4& projection );

	virtual void Prepare(); // called before we start rendering something with this shader
};


#endif // VS_SHADER_H

