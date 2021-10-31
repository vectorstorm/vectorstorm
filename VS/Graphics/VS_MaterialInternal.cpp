/*
 *  VS_MaterialInternal.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_MaterialInternal.h"

#include "VS_DisplayList.h"
#include "VS_Material.h"
#include "VS_Shader.h"
#include "VS_ShaderRef.h"
#include "VS_ShaderCache.h"

#include "VS_File.h"
#include "VS_Record.h"
#include "VS_Token.h"

#include "VS_Renderer_OpenGL3.h"

#include <atomic>

static const vsString s_modeString[DRAWMODE_MAX] =
{
	"absolute",	//DrawMode_Absolute,
	"normal",	//DrawMode_Normal,
	"premultiplied",	//DrawMode_Normal,
	"lit",		//DrawMode_Lit,
	"add",		//DrawMode_Add,
	"subtract",	//DrawMode_Subtract,
	"multiply",	//DrawMode_Multiply,
	"multiplyAbsolute"	//DrawMode_MultiplyAbsolute,
};

static const vsString s_cullString[CULL_MAX] =
{
	"front",	// Cull_Front
	"back",		// Cull_Back
	"none"		// Cull_None
};

static std::atomic<int>	s_codeMaterialCount( 0 );
vsMaterialInternal::vsMaterialInternal():
	vsResource(vsFormatString("CodeMaterial%02d", s_codeMaterialCount++)),
	m_textureCount(0),
	m_shaderIsMine(false),
	m_shader(nullptr),
	m_shaderRef(nullptr),
	m_color(c_white),
	m_specularColor(c_black),
	m_drawMode(DrawMode_Normal),
	m_cullingType(Cull_Back),
	m_alphaRef(0.f),
	m_depthBiasConstant(0.f),
	m_depthBiasFactor(0.f),
	m_glowFactor(1.f),
	m_layer(0),
	m_stencilOp(StencilOp_None),
	m_stencilRead(false),
	m_stencilWrite(false),
	m_alphaTest(false),
	m_fog(false),
	m_zRead(true),
	m_zWrite(true),
	m_clampU(false),
	m_clampV(false),
	m_glow(false),
	m_preGlow(false),
	m_postGlow(false),
	m_postGeneric(false),
	m_hasColor(true),
	m_blend(true),
	m_flags(0)
{
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
	{
		m_texture[i] = nullptr;
		m_textureFromFile[i] = false;
	}

	SetShader();
}

vsMaterialInternal::vsMaterialInternal( const vsString &name ):
	vsResource(name),
	m_textureCount(0),
	m_shaderIsMine(false),
	m_shader(nullptr),
	m_shaderRef(nullptr),
	m_color(c_white),
	m_specularColor(c_black),
	m_drawMode(DrawMode_Normal),
	m_cullingType(Cull_Back),
	m_alphaRef(0.f),
	m_depthBiasConstant(0.f),
	m_depthBiasFactor(0.f),
	m_layer(0),
	m_stencilOp(StencilOp_None),
	m_stencilRead(false),
	m_stencilWrite(false),
	m_alphaTest(false),
	m_fog(false),
	m_zRead(true),
	m_zWrite(true),
	m_clampU(false),
	m_clampV(false),
	m_glow(false),
	m_preGlow(false),
	m_postGlow(false),
	m_postGeneric(false),
	m_hasColor(true),
	m_blend(true),
	m_flags(0)
{
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
	{
		m_texture[i] = nullptr;
		m_textureFromFile[i] = false;
	}

	vsString fileName = vsFormatString("materials/%s.mat", name.c_str());

	if (vsFile::Exists(fileName))
	{
		vsFile materialFile(fileName);
		LoadFromFile( &materialFile );
	}
	else
	{
		vsAssert(vsFile::Exists(fileName), vsFormatString("Unable to open material %s", name.c_str()));
	}
	SetShader();
}

vsMaterialInternal::~vsMaterialInternal()
{
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
		vsDelete( m_texture[i] );
	if ( m_shaderIsMine )
		vsDelete( m_shader );
	vsDelete( m_shaderRef );
}

void
vsMaterialInternal::Reload()
{
	vsString fileName = vsFormatString("materials/%s.mat", GetName().c_str());

	if (vsFile::Exists(fileName))
	{
		vsFile materialFile(fileName);
		LoadFromFile( &materialFile );
	}
	else
	{
		// not a file-based material;  just reload its shader.
		m_shader->Reload();
	}
}

void
vsMaterialInternal::LoadFromFile( vsFile *materialFile )
{
	vsRecord r, sr;

	bool setFog = false;

	for ( int i = 0; i < m_textureCount; i++ )
	{
		if ( m_textureFromFile[i] )
			vsDelete( m_texture[i] );
	}
	m_textureCount = 0;

	while( materialFile->Record(&r) )
	{
		if ( r.GetLabel().AsString() == "Material" )
		{
			for ( int i = 0; i < r.GetChildCount(); i++ )
			{
				vsRecord *sr = r.GetChild(i);
				vsString label = sr->GetLabel().AsString();

				if( label == "color" )
				{
					vsAssert( sr->GetTokenCount() == 4, "Wrong number of color components??" );
					m_color = sr->Color();
					m_hasColor = true;
				}
				else if ( label == "specular" )
				{
					vsAssert( sr->GetTokenCount() == 4, "Wrong number of color components??" );
					m_specularColor = sr->Color();
					m_hasColor = true;
				}
				else if ( label == "alpharef" )
				{
					vsAssert( sr->GetTokenCount() == 1, "Wrong number of alphatest tokens??" );
					m_alphaRef = sr->GetToken(0).AsFloat();
					m_alphaTest = true;
				}
				else if ( label == "culling" )
				{
					vsAssert( sr->GetTokenCount() == 1, "Wrong number of culling tokens??" );
					vsString cullString = sr->String();
					for ( int cull = 0; cull < CULL_MAX; cull++ )
					{
						if ( cullString == s_cullString[cull] )
						{
							m_cullingType = (CullingType)cull;
						}
					}
				}
				else if ( label == "depthbias" )
				{
					vsAssert( sr->GetTokenCount() == 2, "Wrong number of depthbias tokens??" );
					m_depthBiasConstant = sr->GetToken(0).AsFloat();
					m_depthBiasFactor = sr->GetToken(1).AsFloat();
				}
				else if ( label == "zread" )
				{
					m_zRead = sr->Bool();
				}
				else if ( label == "zwrite" )
				{
					m_zWrite = sr->Bool();
				}
				else if ( label == "glow" )
				{
					m_glow = sr->Bool();
				}
				else if ( label == "glowFactor" )
				{
					m_glowFactor = sr->GetToken(0).AsFloat();
				}
				else if ( label == "preglow" )
				{
					m_preGlow = sr->Bool();
				}
				else if ( label == "postglow" )
				{
					m_postGlow = sr->Bool();
				}
				else if ( label == "postgeneric" )
				{
					m_postGeneric = sr->Bool();
				}
				else if ( label == "fog" )
				{
					m_fog = sr->Bool();
					setFog = true;
				}
				else if ( label == "mode" )
				{
					vsAssert( sr->GetTokenCount() == 1, "Mode directive with more than one token??" );
					vsString modeString = sr->String();
					for ( int mode = 0; mode < DRAWMODE_MAX; mode++ )
					{
						if ( modeString == s_modeString[mode] )
						{
							m_drawMode = (vsDrawMode)mode;
							break;
						}
					}

					// backwards compatibility;  if we haven't said otherwise, "Lit" implies fog.
					if ( !setFog && m_drawMode == DrawMode_Lit )
					{
						m_fog = true;
					}
				}
				else if ( label == "texture" )
				{
					vsAssert( sr->GetTokenCount() >= 1, "Texture directive with more than one token??" );
					vsString textureString = sr->String();
					m_texture[m_textureCount] = new vsTexture( textureString );
					m_textureFromFile[m_textureCount] = true;

					if ( sr->GetTokenCount() == 2 )
					{
						vsString textureMode = sr->GetToken(1).AsString();
						if ( textureMode == "nearest" )
							m_texture[m_textureCount]->GetResource()->SetNearestSampling();
					}
					m_textureCount++;
				}
				else if ( label == "shader" )
				{
					if ( m_shaderIsMine )
						vsDelete( m_shader );
					m_shader = nullptr;

					vsAssert( sr->GetTokenCount() == 2, "Shader directive without more than two tokens??" );
					vsString vString = sr->GetToken(0).AsString();
					vsString fString = sr->GetToken(1).AsString();
					m_shaderRef = vsShaderCache::LoadShader( vString, fString, m_drawMode == DrawMode_Lit, HasAnyTextures() );
					m_shader = m_shaderRef->GetShader();
					m_shaderIsMine = false;
				}
				else if ( label == "clampU" )
				{
					m_clampU = sr->Bool();
				}
				else if ( label == "clampV" )
				{
					m_clampV = sr->Bool();
				}
				else if ( label == "blend" )
				{
					m_blend = sr->Bool();
				}
				else if ( label == "layer" )
				{
					m_layer = sr->Int();
				}
				else if ( label == "flags" )
				{
					m_flags = sr->Int();
				}
				else if ( label == "stencilOp" )
				{
					vsString request = sr->String();
					std::string opString[STENCILOP_MAX] =
					{
						"none",
						"one",
						"zero",
						"inc",
						"dec",
						"invert"
					};

					for ( int i = 0; i < STENCILOP_MAX; i++ )
					{
						if ( opString[i] == request )
						{
							m_stencilOp = (StencilOp)i;
						}
					}
				}
				else if ( label == "stencilRead" )
				{
					m_stencilRead = sr->Bool();
				}
				else if ( label == "stencilWrite" )
				{
					m_stencilWrite = sr->Bool();
				}
			}
			break;
		}
	}

}

void
vsMaterialInternal::SetShader()
{
	if (!m_shader && !m_shaderRef )
	{
		m_shader = vsRenderer_OpenGL3::Instance()->DefaultShaderFor(this);
		m_shaderIsMine = false;
	}
}

bool
vsMaterialInternal::HasAnyTextures() const
{
	if ( m_texture[0] != nullptr )
		return true;
	return false;
}

void
vsMaterialInternal::SetTexture(int i, vsTexture *texture)
{
	vsDelete(m_texture[i]);
	m_texture[i] = new vsTexture(texture);
	m_textureFromFile[i] = false;
}

