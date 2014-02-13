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

#include "VS_File.h"
#include "VS_Record.h"
#include "VS_Token.h"

static const vsString s_modeString[DRAWMODE_MAX] =
{
	"normal",	//DrawMode_Normal,
	"lit",		//DrawMode_Lit,
	"add",		//DrawMode_Add,
	"subtract",	//DrawMode_Subtract,
	"absolute", //DrawMode_Absolute
};

static const vsString s_cullString[CULL_MAX] =
{
	"front",	// Cull_Front
	"back",		// Cull_Back
	"none"		// Cull_None
};

vsMaterialInternal::vsMaterialInternal( const vsString &name ):
	vsResource(name),
	m_shader(NULL),
	m_texture(NULL),
	m_color(c_white),
	m_specularColor(c_black),
	m_drawMode(DrawMode_Normal),
	m_cullingType(Cull_Back),
	m_alphaRef(0.f),
	m_depthBiasConstant(0.f),
	m_depthBiasFactor(0.f),
	m_layer(0),
	m_stencil(StencilOp_None),
	m_alphaTest(false),
	m_fog(false),
	m_zRead(true),
	m_zWrite(true),
	m_clampU(false),
	m_clampV(false),
	m_glow(false),
	m_postGlow(false),
	m_hasColor(true),
	m_blend(false)
{
	vsString fileName = vsFormatString("materials/%s.mat", name.c_str());

	if (vsFile::Exists(fileName))
	{
		vsFile materialFile(fileName);
		LoadFromFile( &materialFile );
	}
}

vsMaterialInternal::~vsMaterialInternal()
{
	vsDelete( m_texture );
	vsDelete( m_shader );
}

void
vsMaterialInternal::LoadFromFile( vsFile *materialFile )
{
	vsRecord r, sr;

	bool setFog = false;

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
				else if ( label == "postglow" )
				{
					m_postGlow = sr->Bool();
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
					vsAssert( sr->GetTokenCount() == 1, "Texture directive with more than one token??" );
					vsString textureString = sr->String();
					m_texture = new vsTexture( textureString );
				}
				else if ( label == "shader" )
				{
					vsAssert( sr->GetTokenCount() == 2, "Shader directive without more than two tokens??" );
					vsString vString = sr->GetToken(0).AsString();
					vsString fString = sr->GetToken(1).AsString();
					m_shader = new vsShader( vString, fString, m_drawMode == DrawMode_Lit, (m_texture != NULL) );
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
				else if ( label == "stencil" )
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
							m_stencil = (StencilOp)i;
						}
					}
				}
			}
			break;
		}
	}
}

