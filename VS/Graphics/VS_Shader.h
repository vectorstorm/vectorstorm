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
class vsMatrix4x4;
class vsShaderValues;

#include "VS_Color.h"
#include "VS/Math/VS_Vector.h"
#include "VS_MaterialInternal.h"
#include "VS/Utils/VS_AutomaticInstanceList.h"

class vsShader: public vsAutomaticInstanceList<vsShader>
{
public:
	struct Uniform
	{
		vsString name;
		// struct
		// {
			int b;
			float f32;
			vsVector4D vec4; // for vectors of up to 4 floats
		// };
		int32_t loc;
		int32_t type;
		int32_t arraySize;
	};
	struct Attribute
	{
		vsString name;
		int32_t loc;
		int32_t type;
		int32_t arraySize;
	};
	struct Block
	{
		vsString name;
	};

private:
	int32_t m_colorLoc;
	int32_t m_instanceColorAttributeLoc;
	int32_t m_resolutionLoc;
	int32_t m_mouseLoc;
	int32_t m_fogColorId;
	int32_t m_fogDensityId;
	int32_t m_textureLoc;
	int32_t m_shadowTextureLoc;
	int32_t m_localToWorldLoc;
	int32_t m_localToWorldAttributeLoc;
	int32_t m_worldToViewLoc;
	int32_t m_cameraPositionLoc;
	int32_t m_viewToProjectionLoc;

	int32_t m_lightAmbientLoc;
	int32_t m_lightDiffuseLoc;
	int32_t m_lightSpecularLoc;
	int32_t m_lightPositionLoc;
	int32_t m_lightHalfVectorLoc;

	Uniform *m_uniform;
	Attribute *m_attribute;
	Block *m_block;

	int32_t m_uniformCount;
	int32_t m_attributeCount;
	int32_t m_blockCount;

	int32_t m_globalTimeUniformId;

	vsString m_vertexShaderFile;
	vsString m_fragmentShaderFile;

	void SetUniformValueF( int i, float value );
	void SetUniformValueB( int i, bool value );
	void SetUniformValueVec3( int i, const vsVector3D& value );
	void SetUniformValueVec3( int i, const vsColor& value ); // only rgb channels used
	void SetUniformValueVec4( int i, const vsVector4D& value );
	void SetUniformValueVec4( int i, const vsColor& value );
	void SetUniformValueMat4( int i, const vsMatrix4x4& value );

	void Compile( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );

protected:
	uint32_t m_shader;
	bool m_litBool;
	bool m_textureBool;

public:

	vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	virtual ~vsShader();

	static vsShader *Load( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	static void ReloadAll();

	uint32_t GetShaderId() { return m_shader; }

	void SetFog( bool fog, const vsColor& color, float fogDensity );
	void SetColor( const vsColor& color );
	void SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] );
	void SetLocalToWorld( const vsMatrix4x4* localToWorld, int matCount );
	void SetInstanceColors( vsRenderBuffer *colors );
	void SetInstanceColors( const vsColor* color, int matCount );
	void SetLocalToWorld( vsRenderBuffer* buffer );
	void SetWorldToView( const vsMatrix4x4& worldToView );
	void SetViewToProjection( const vsMatrix4x4& projection );

	const Uniform *GetUniform(int i) const { return &m_uniform[i]; }
	int32_t GetUniformId(const vsString& name) const;
	int32_t GetUniformCount() const { return m_uniformCount; }
	int32_t GetAttributeCount() const { return m_attributeCount; }

	void SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector );

	virtual void Prepare( vsMaterial *activeMaterial, vsShaderValues *values = NULL ); // called before we start rendering something with this shader
	void ValidateCache( vsMaterial *activeMaterial ); // after rendering something to check that our cache is working.
};


#endif // VS_SHADER_H

