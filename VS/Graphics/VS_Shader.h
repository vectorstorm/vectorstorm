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
class vsShaderVariant;
class vsMaterial;

#include "VS_Color.h"
#include "VS/Math/VS_Vector.h"
#include "VS_MaterialInternal.h"
#include "VS/Utils/VS_AutomaticInstanceList.h"
#include "VS/Utils/VS_Array.h"

struct vsShaderVariantDefinition
{
	vsString name;
	int bitId;
};

struct vsShaderAutoBitDefinition
{
	int bitId;
	vsString uniformName;
	uint32_t uniformUID;
};


class vsShader: public vsAutomaticInstanceList<vsShader>
{
public:
	struct Uniform
	{
		vsString name;
		uint32_t uid;
		union
		{
			int32_t i32;
			uint32_t u32;
			float f32;
		};
		vsVector4D vec4; // for vectors of up to 4 floats
		vsMatrix4x4 mat;
		int32_t loc;
		int32_t type;
		int32_t arraySize;
		int32_t def;
	};
	struct Attribute
	{
		vsString name;
		int32_t loc;
		int32_t type;
		int32_t arraySize;
	};
private:
	vsString m_vertexShaderFile;
	vsString m_fragmentShaderFile;
	vsString m_vertexShaderText;
	vsString m_fragmentShaderText;

	uint32_t m_variantBitsSupported;

	vsShaderVariant *m_current;
	vsArray< vsShaderVariant* > m_variant;

	bool m_system; // system shader;  should not be reloaded!

	void FigureOutAvailableVariants( const vsString& shaderSource );

protected:
	bool m_litBool;
	bool m_textureBool;

public:

	vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture, uint32_t variantBits = 0, const vsString& vfilename = vsEmptyString, const vsString& ffilename = vsEmptyString );
	virtual ~vsShader();

	static vsShader *Load( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	static vsShader *Load_System( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture );
	static void ReloadAll();
	void Reload();

	uint32_t GetShaderId() const;
	uint32_t GetVariantBitsSupported() const { return m_variantBitsSupported; }
	uint32_t GetCurrentVariantBits();
	void SetForVariantBits( uint32_t bits );
	static uint32_t GetVariantBitsFor( const vsShaderValues *values );

	void SetFog( bool fog, const vsColor& color, float fogDensity );
	void SetColor( const vsColor& color );
	void SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] );
	void SetLocalToWorld( const vsMatrix4x4* localToWorld, int matCount );
	void SetInstanceColors( vsRenderBuffer *colors );
	void SetInstanceColors( const vsColor* color, int matCount );
	void SetLocalToWorld( vsRenderBuffer* buffer );
	void SetWorldToView( const vsMatrix4x4& worldToView );
	void SetViewToProjection( const vsMatrix4x4& projection );
	void SetViewport( const vsVector2D& dims );

	const Uniform *GetUniform(int i) const;
	int32_t GetUniformId(const vsString& name) const;
	int32_t GetUniformCount() const;
	int32_t GetAttributeCount() const;

	void SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector );

	virtual void Prepare( vsMaterial *activeMaterial, vsShaderValues *values = nullptr, vsRenderTarget *renderTarget = nullptr ); // called before we start rendering something with this shader
	void ValidateCache( vsMaterial *activeMaterial ); // after rendering something to check that our cache is working.


	// should be set just once at game start.
	static void SetShaderVariantDefinitions( const vsArray<vsShaderVariantDefinition>& definitions );
	static void SetAutoBits( const vsArray<vsShaderAutoBitDefinition>& definitions );

	// what combinations of shader bit patterns should we precompile?
	static void SetPreCompileBitPatterns( const vsArray<uint32_t>& patterns );
};


#endif // VS_SHADER_H

