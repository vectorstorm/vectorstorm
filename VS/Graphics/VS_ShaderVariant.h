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

class vsVertexArrayObject;

class vsShaderVariant
{
public:

private:
	int32_t m_colorUniformId;
	int32_t m_instanceColorAttributeLoc;
	int32_t m_hasInstanceColorsUniformId;
	int32_t m_resolutionUniformId;
	int32_t m_mouseUniformId;
	int32_t m_fogColorId;
	int32_t m_fogDensityId;
	int32_t m_textureUniformId;
	int32_t m_shadowTextureUniformId;
	int32_t m_bufferTextureUniformId;
	int32_t m_localToWorldUniformId;
	int32_t m_localToWorldAttributeLoc;
	int32_t m_worldToViewUniformId;
	int32_t m_cameraPositionUniformId;
	int32_t m_cameraDirectionUniformId;
	int32_t m_viewToProjectionUniformId;
	int32_t m_viewportUniformId;

	int32_t m_lightAmbientUniformId;
	int32_t m_lightDiffuseUniformId;
	int32_t m_lightSpecularUniformId;
	int32_t m_lightPositionUniformId;
	int32_t m_lightHalfVectorUniformId;

	int32_t m_depthOnlyUniformId;

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

	inline void SetUniformValueF( int i, float value );
	inline void SetUniformValueB( int i, bool value );
	inline void SetUniformValueI( int i, int value );
	inline void SetUniformValueUI( int i, unsigned int value );
	inline void SetUniformValueVec2( int i, const vsVector2D& value );
	inline void SetUniformValueVec3( int i, const vsVector3D& value );
	inline void SetUniformValueVec3( int i, const vsColor& value ); // only rgb channels used
	inline void SetUniformValueVec4( int i, const vsVector4D& value );
	inline void SetUniformValueVec4( int i, const vsColor& value );
	inline void SetUniformValueMat4( int i, const vsMatrix4x4& value );

	void Compile( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture, uint32_t variantBits );

	void DoPreprocessor( vsString &s );

protected:
	uint32_t m_shader;
	uint32_t m_variantBits; // What variant bits are set on me.
	bool m_litBool;
	bool m_textureBool;

public:

	vsShaderVariant( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture, uint32_t variantBits = 0, const vsString& vfilename = vsEmptyString, const vsString& ffilename = vsEmptyString );
	virtual ~vsShaderVariant();
	void Reload( const vsString& vertexShader, const vsString &fragmentShader );

	uint32_t GetShaderId() const { return m_shader; }
	uint32_t GetVariantBits() const { return m_variantBits; }
	// uint32_t GetAvailableVariantBits() const { return m_availableVariantBits; }

	void SetFog( bool fog, const vsColor& color, float fogDensity );
	void SetColor( vsVertexArrayObject *vao, const vsColor& color );
	void SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] );
	void SetLocalToWorld( vsVertexArrayObject *vao, const vsMatrix4x4* localToWorld, int matCount );
	void SetInstanceColors( vsVertexArrayObject *vao, vsRenderBuffer *colors );
	void SetInstanceColors( vsVertexArrayObject *vao, const vsColor* color, int matCount );
	void SetLocalToWorld( vsVertexArrayObject *vao, vsRenderBuffer* buffer );
	void SetWorldToView( const vsMatrix4x4& worldToView );
	void SetViewToProjection( const vsMatrix4x4& projection );
	void SetViewport( const vsVector2D& viewportDims );

	const vsShader::Uniform *GetUniform(int i) const { return &m_uniform[i]; }
	int32_t GetUniformId(const vsString& name) const;
	int32_t GetUniformCount() const { return m_uniformCount; }
	int32_t GetAttributeCount() const { return m_attributeCount; }

	void Prepare( vsMaterial *activeMaterial, vsShaderValues *values = nullptr, vsRenderTarget *renderTarget = nullptr ); // called before we start rendering something with this shader
	void SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector );

	friend class vsShader;
};

#endif // VS_SHADERVARIANT_H

