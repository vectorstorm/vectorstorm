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
public:
	struct Uniform
	{
		vsString name;
		union
		{
			bool b;
			float f32;
		};
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

private:
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

	int32_t m_lightAmbientLoc;
	int32_t m_lightDiffuseLoc;
	int32_t m_lightSpecularLoc;
	int32_t m_lightPositionLoc;
	int32_t m_lightHalfVectorLoc;

	Uniform *m_uniform;
	Attribute *m_attribute;

	int32_t m_uniformCount;
	int32_t m_attributeCount;

	void SetUniformValueF( int i, float value );
	void SetUniformValueB( int i, bool value );

protected:
	uint32_t m_shader;

public:

	vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	virtual ~vsShader();

	static vsShader *Load( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );

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

	virtual void Prepare( vsMaterial *activeMaterial ); // called before we start rendering something with this shader
};


#endif // VS_SHADER_H

