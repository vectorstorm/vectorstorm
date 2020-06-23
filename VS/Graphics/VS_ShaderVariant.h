/*
 *  VS_ShaderVariant.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/06/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SHADERVARIANT_H
#define VS_SHADERVARIANT_H

#include "VS_Shader.h"

class vsShaderVariant
{
public:

private:
	int32_t m_colorLoc;
	int32_t m_instanceColorAttributeLoc;
	int32_t m_hasInstanceColorsLoc;
	int32_t m_resolutionLoc;
	int32_t m_mouseLoc;
	int32_t m_fogColorId;
	int32_t m_fogDensityId;
	int32_t m_textureLoc;
	int32_t m_shadowTextureLoc;
	int32_t m_bufferTextureLoc;
	int32_t m_localToWorldLoc;
	int32_t m_localToWorldAttributeLoc;
	int32_t m_worldToViewLoc;
	int32_t m_cameraPositionLoc;
	int32_t m_cameraDirectionLoc;
	int32_t m_viewToProjectionLoc;

	int32_t m_lightAmbientLoc;
	int32_t m_lightDiffuseLoc;
	int32_t m_lightSpecularLoc;
	int32_t m_lightPositionLoc;
	int32_t m_lightHalfVectorLoc;

	int32_t m_depthOnlyLoc;

	vsShader::Uniform *m_uniform;
	vsShader::Attribute *m_attribute;

	int32_t m_uniformCount;
	int32_t m_attributeCount;

	int32_t m_globalTimeUniformId;
	int32_t m_globalSecondsUniformId;
	int32_t m_globalMicrosecondsUniformId;

	vsString m_vertexShaderFile;
	vsString m_fragmentShaderFile;

	bool m_system; // system shader;  should not be reloaded!

	void SetUniformValueF( int i, float value );
	void SetUniformValueB( int i, bool value );
	void SetUniformValueI( int i, int value );
	void SetUniformValueVec2( int i, const vsVector2D& value );
	void SetUniformValueVec3( int i, const vsVector3D& value );
	void SetUniformValueVec3( int i, const vsColor& value ); // only rgb channels used
	void SetUniformValueVec4( int i, const vsVector4D& value );
	void SetUniformValueVec4( int i, const vsColor& value );
	void SetUniformValueMat4( int i, const vsMatrix4x4& value );

	void Compile( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture, uint32_t variantBits );

protected:
	uint32_t m_shader;
	uint32_t m_variantBits; // we might have some variants available.
	bool m_litBool;
	bool m_textureBool;

public:

	vsShaderVariant( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture, uint32_t variantBits = 0, const vsString& vfilename = vsEmptyString, const vsString& ffilename = vsEmptyString );
	virtual ~vsShaderVariant();
	void Reload();

	uint32_t GetShaderId() const { return m_shader; }
	uint32_t GetVariantBits() const { return m_variantBits; }

	void SetFog( bool fog, const vsColor& color, float fogDensity );
	void SetColor( const vsColor& color );
	void SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] );
	void SetLocalToWorld( const vsMatrix4x4* localToWorld, int matCount );
	void SetInstanceColors( vsRenderBuffer *colors );
	void SetInstanceColors( const vsColor* color, int matCount );
	void SetLocalToWorld( vsRenderBuffer* buffer );
	void SetWorldToView( const vsMatrix4x4& worldToView );
	void SetViewToProjection( const vsMatrix4x4& projection );

	const vsShader::Uniform *GetUniform(int i) const { return &m_uniform[i]; }
	int32_t GetUniformId(const vsString& name) const;
	int32_t GetUniformCount() const { return m_uniformCount; }
	int32_t GetAttributeCount() const { return m_attributeCount; }

	void Prepare( vsMaterial *activeMaterial, vsShaderValues *values = NULL, vsRenderTarget *renderTarget = NULL ); // called before we start rendering something with this shader
	void SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector );

	friend class vsShader;
};

#endif // VS_SHADERVARIANT_H

