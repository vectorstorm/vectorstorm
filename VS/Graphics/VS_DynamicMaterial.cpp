/*
 *  VS_DynamicMaterial.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 07/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_DynamicMaterial.h"
#include "VS_MaterialInternal.h"
#include "VS_Shader.h"
#include "VS_ShaderCache.h"
#include "VS_ShaderRef.h"

vsDynamicMaterial::vsDynamicMaterial():
	vsMaterial()
{
	GetResource()->SetTransient();	// dynamic materials are transient;  once the material is destroyed, its internal storage should also be destroyed.
}

void
vsDynamicMaterial::SetShader( vsShader *shader, bool owned )
{
	GetResource()->m_shader = shader;
	GetResource()->m_shaderIsMine = owned;
	SetupParameters();
}

void
vsDynamicMaterial::SetShader( const vsString &vShader, const vsString &fShader )
{
	bool hasTextures = GetResource()->HasAnyTextures();
	if ( GetResource()->m_shaderIsMine )
	{
		vsDelete( GetResource()->m_shader );
	}
	vsDelete( GetResource()->m_shaderRef );
	GetResource()->m_shaderRef = vsShaderCache::LoadShader(vShader, fShader, GetResource()->m_drawMode == DrawMode_Lit, hasTextures);
	GetResource()->m_shader = GetResource()->m_shaderRef->GetShader();
	GetResource()->m_shaderIsMine = false;
	SetupParameters();
}

void
vsDynamicMaterial::SetShader()
{
	if ( GetResource()->m_shaderIsMine )
	{
		vsDelete( GetResource()->m_shader );
	}
	else
		GetResource()->m_shader = nullptr;

	GetResource()->SetShader();
	SetupParameters();
}

void
vsDynamicMaterial::SetTexture( int i, vsTexture *texture, bool linear )
{
	vsAssert(i >= 0 && i < MAX_TEXTURE_SLOTS, "Out of range texture requested");
	vsDelete(GetResource()->m_texture[i]);
	if ( texture )
	{
		GetResource()->m_texture[i] = new vsTexture(texture);
		if ( !linear )
			GetResource()->m_texture[i]->GetResource()->SetNearestSampling();
	}
}

void
vsDynamicMaterial::SetShadowTexture( vsTexture *texture )
{
	SetTexture( 8, texture );
}

void
vsDynamicMaterial::SetBufferTexture( vsTexture *texture)
{
	SetTexture( 9, texture );
}

void
vsDynamicMaterial::SetTexture( int i, const vsString &texture, bool linear )
{
	vsAssert(i >= 0 && i < MAX_TEXTURE_SLOTS, "Out of range texture requested");
	vsDelete(GetResource()->m_texture[i]);
	GetResource()->m_texture[i] = new vsTexture(texture);
	if ( !linear )
		GetResource()->m_texture[i]->GetResource()->SetNearestSampling();
}

void
vsDynamicMaterial::SetColor( const vsColor& color )
{
	GetResource()->m_color = color;
	GetResource()->m_hasColor = true;
}

void
vsDynamicMaterial::SetSpecularColor( const vsColor& specularColor )
{
	GetResource()->m_specularColor = specularColor;
}

void
vsDynamicMaterial::SetDrawMode( vsDrawMode drawMode )
{
	GetResource()->m_drawMode = drawMode;
}

void
vsDynamicMaterial::SetCullingType( CullingType cullingType )
{
	GetResource()->m_cullingType = cullingType;
}

void
vsDynamicMaterial::SetAlphaRef( float alphaRef )
{
	GetResource()->m_alphaRef = alphaRef;
}

void
vsDynamicMaterial::SetDepthBiasConstant( float depthBiasConstant )
{
	GetResource()->m_depthBiasConstant = depthBiasConstant;
}

void
vsDynamicMaterial::SetDepthBiasFactor( float depthBiasFactor )
{
	GetResource()->m_depthBiasFactor = depthBiasFactor;
}

void
vsDynamicMaterial::SetLayer( int layer )
{
	GetResource()->m_layer = layer;
}

void
vsDynamicMaterial::SetStencil( StencilOp stencil )
{
	GetResource()->m_stencilOp = stencil;
}

void
vsDynamicMaterial::SetAlphaTest( bool alphaTest )
{
	GetResource()->m_alphaTest = alphaTest;
}

void
vsDynamicMaterial::SetFog( bool fog )
{
	GetResource()->m_fog = fog;
	SetUniformB("fog", fog );
}

void
vsDynamicMaterial::SetZRead( bool zRead )
{
	GetResource()->m_zRead = zRead;
}

void
vsDynamicMaterial::SetZWrite( bool zWrite )
{
	GetResource()->m_zWrite = zWrite;
}

void
vsDynamicMaterial::SetClampU( bool clampU )
{
	GetResource()->m_clampU = clampU;
}

void
vsDynamicMaterial::SetClampV( bool clampV )
{
	GetResource()->m_clampV = clampV;
}

void
vsDynamicMaterial::SetGlow( bool glow )
{
	GetResource()->m_glow = glow;
}

void
vsDynamicMaterial::SetPostGlow( bool postGlow )
{
	GetResource()->m_postGlow = postGlow;
}

void
vsDynamicMaterial::SetHasColor( bool hasColor )
{
	GetResource()->m_hasColor = hasColor;
}

void
vsDynamicMaterial::SetBlend( bool blend )
{
	GetResource()->m_blend = blend;
}

