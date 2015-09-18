/*
 *  VS_MaterialInternal.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 23/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MATERIALINTERNAL_H
#define VS_MATERIALINTERNAL_H

#include "VS/Utils/VS_Cache.h"

#include "VS_Material.h"

#include "VS_Color.h"
#include "VS_Texture.h"

#define MAX_TEXTURE_SLOTS (8)
//class vsTexture;
class vsFile;
class vsShader;

enum CullingType
{
	Cull_Front,
	Cull_Back,
	Cull_None,
	CULL_MAX
};

enum StencilOp
{
	StencilOp_None,	// do nothing to stencil buffer (the default)
	StencilOp_One,	// write a value of '1' into the stencil buffer
	StencilOp_Zero, // write a value of '0' into the stencil buffer
	StencilOp_Inc,	// increment the value already in the stencil buffer
	StencilOp_Dec,  // decrement the value already in the stencil buffer
	StencilOp_Invert, // If the value is 0, make it 1.  Otherwise, make it 0.
	STENCILOP_MAX
};

class vsMaterialInternal : public vsResource
{
	void		SetShader();

	int			m_textureCount; // used during loading;  NOT set for dynamic materials!
public:

	bool		m_shaderIsMine;						// if true, we own this shader and must destroy it.
	vsShader *  m_shader;
	vsTexture *	m_texture[MAX_TEXTURE_SLOTS];		// what texture do we use?
	vsTexture *	m_shadowTexture;		// shadow texture?
	vsColor		m_color;			// what basic colour?  (If a texture is applied, this will multiply with the texture color)
	vsColor		m_specularColor;	// if we're of "Lit" type, this is our specular color
	vsDrawMode	m_drawMode;
	CullingType	m_cullingType;
	float		m_alphaRef;
	float		m_depthBiasConstant;
	float		m_depthBiasFactor;
	int			m_layer;
	StencilOp	m_stencil;
	bool		m_alphaTest;
	bool		m_fog;
	bool		m_zRead;
	bool		m_zWrite;
	bool		m_clampU;
	bool		m_clampV;
	bool		m_glow;
	bool		m_postGlow;
	bool		m_hasColor;
	bool		m_blend;

	vsMaterialInternal(); // no material name;  we'll create our own name instead.
	vsMaterialInternal( const vsString & name ); // for loading this material from a file
	vsMaterialInternal( const vsString & textureName, vsDrawMode mode, const vsColor &c, const vsColor &sc = c_black );
	vsMaterialInternal( vsTexture *texture, vsDrawMode mode, const vsColor &c, const vsColor &sc = c_black );
	vsMaterialInternal( vsDrawMode mode, const vsColor &c, const vsColor &sc = c_black);
	~vsMaterialInternal();

	void		LoadFromFile( vsFile *file );

	vsTexture *	GetTexture(int i = 0) const { return m_texture[i]; }
	bool HasAnyTextures() const;

	void operator=(const vsMaterialInternal &b);
	bool operator==(const vsMaterialInternal &b) const { return (m_color==b.m_color && m_drawMode==b.m_drawMode); }
	bool operator!=(const vsMaterialInternal &b) const { return !((*this)==b); }

	friend class vsDynamicMaterial;
};


#endif // VS_MATERIALINTERNAL_H

