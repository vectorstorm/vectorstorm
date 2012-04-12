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

//class vsTexture;
class vsFile;
class vsDisplayList;
class vsShader;

enum CullingType
{
	Cull_Front,
	Cull_Back,
	Cull_None,
	CULL_MAX
};

class vsMaterialInternal : public vsResource
{
public:

	vsShader *  m_shader;
	vsTexture *	m_texture;			// what texture do we use?
	vsColor		m_color;			// what basic colour?  (If a texture is applied, this will multiply with the texture color)
	vsColor		m_specularColor;	// if we're of "Lit" type, this is our specular color
	vsDrawMode	m_drawMode;
	CullingType	m_cullingType;
	float		m_alphaRef;
	float		m_depthBiasConstant;
	float		m_depthBiasFactor;
	int			m_layer;
	bool		m_alphaTest;
	bool		m_fog;
	bool		m_zRead;
	bool		m_zWrite;
	bool		m_clampU;
	bool		m_clampV;
	bool		m_glow;
	bool		m_hasColor;

	vsDisplayList *	m_displayList;

	vsMaterialInternal( const vsString & materialName );
	vsMaterialInternal( const vsString & textureName, vsDrawMode mode, const vsColor &c, const vsColor &sc = vsColor::Black );
	vsMaterialInternal( vsTexture *texture, vsDrawMode mode, const vsColor &c, const vsColor &sc = vsColor::Black );
	vsMaterialInternal( vsDrawMode mode, const vsColor &c, const vsColor &sc = vsColor::Black);
	~vsMaterialInternal();

	void		LoadFromFile( vsFile *file );
	void		Compile();

	vsTexture *	GetTexture() const { return m_texture; }

	void operator=(const vsMaterialInternal &b);
	bool operator==(const vsMaterialInternal &b) const { return (m_color==b.m_color && m_drawMode==b.m_drawMode); }
	bool operator!=(const vsMaterialInternal &b) const { return !((*this)==b); }
};


#endif // VS_MATERIALINTERNAL_H

