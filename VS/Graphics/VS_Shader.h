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
class vsRenderBuffer;

#include "VS_Color.h"
#include "VS/Math/VS_Vector.h"
#include "VS_MaterialInternal.h"
class vsMatrix4x4;

class vsShader
{
	int32_t m_alphaRefLoc;
	int32_t m_colorLoc;
	int32_t m_instanceColorAttributeLoc;
	int32_t m_resolutionLoc;
	int32_t m_globalTimeLoc;
	int32_t m_mouseLoc;
	int32_t m_fogLoc;
	int32_t m_fogColorLoc;
	int32_t m_fogDensityLoc;
	int32_t m_textureLoc;
	int32_t m_localToWorldLoc;
	int32_t m_localToWorldAttributeLoc;
	int32_t m_worldToViewLoc;
	int32_t m_viewToProjectionLoc;
	int32_t m_glowLoc;

	int32_t m_lightAmbientLoc;
	int32_t m_lightDiffuseLoc;
	int32_t m_lightSpecularLoc;
	int32_t m_lightPositionLoc;
	int32_t m_lightHalfVectorLoc;

protected:
	uint32_t m_shader;

public:

	vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	virtual ~vsShader();

	static vsShader *Load( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );

	uint32_t GetShaderId() { return m_shader; }

    void SetAlphaRef( float aref );
	void SetFog( bool fog, const vsColor& color, float fogDensity );
	void SetColor( const vsColor& color );
	void SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] );
	void SetLocalToWorld( const vsMatrix4x4* localToWorld, int matCount );
	void SetInstanceColors( const vsColor* color, int matCount );
	void SetLocalToWorld( vsRenderBuffer* buffer );
	void SetWorldToView( const vsMatrix4x4& worldToView );
	void SetViewToProjection( const vsMatrix4x4& projection );
	void SetGlow( float glowAlpha );

	void SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector );

	virtual void Prepare(); // called before we start rendering something with this shader
};


#endif // VS_SHADER_H

