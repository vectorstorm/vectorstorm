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

vsDynamicMaterial::vsDynamicMaterial():
	vsMaterial()
{
}

void
vsDynamicMaterial::SetShader( vsShader *shader )
{
	GetResource()->m_shader = shader;
	GetResource()->m_shaderIsMine = true;
	SetupParameters();
}

void
vsDynamicMaterial::SetShader( const vsString &vShader, const vsString &fShader )
{
	bool hasTextures = (GetResource()->m_texture[0] != NULL) || GetResource()->m_shadowTexture || GetResource()->m_bufferTexture;
	GetResource()->m_shader = vsShader::Load(vShader, fShader, GetResource()->m_drawMode == DrawMode_Lit, hasTextures);
	GetResource()->m_shaderIsMine = true;
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
		GetResource()->m_shader = NULL;

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
	else
	{
		vsDelete( GetResource()->m_texture[i] );
	}
}

void
vsDynamicMaterial::SetShadowTexture( vsTexture *texture )
{
	if ( texture )
	{
		GetResource()->m_shadowTexture = new vsTexture(texture);
	}
	else
	{
		vsDelete( GetResource()->m_shadowTexture );
	}
}

void
vsDynamicMaterial::SetBufferTexture( vsTexture *texture)
{
	if ( texture )
	{
		GetResource()->m_bufferTexture = new vsTexture(texture);
	}
	else
	{
		vsDelete( GetResource()->m_bufferTexture );
	}
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
	GetResource()->m_stencil = stencil;
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

