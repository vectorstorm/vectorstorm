/*
 *  VS_Material.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MATERIAL_H
#define VS_MATERIAL_H

#include "VS/Utils/VS_Cache.h"

#include "VS_Color.h"
#include "VS_Texture.h"

enum vsDrawMode
{
	DrawMode_Normal,
	DrawMode_Lit,
	DrawMode_Add,
	DrawMode_Subtract,

	DRAWMODE_MAX
};

class vsMaterialInternal;

class vsMaterial : public vsCacheReference<vsMaterialInternal>
{
public:

	//vsMaterial();
	vsMaterial( const vsString &name );
	vsMaterial( vsMaterial *other );
	vsMaterial( const vsString & textureName, vsDrawMode mode, const vsColor &c, const vsColor &sc = c_black );
	vsMaterial( const vsString & textureName, vsDrawMode mode );
	vsMaterial( vsTexture *texture, vsDrawMode mode, const vsColor &c, const vsColor &sc = c_black );
	vsMaterial( vsDrawMode mode, const vsColor &c, const vsColor &sc = c_black);
//	~vsMaterial();

	bool operator==(const vsMaterial &b) const { return (m_resource == b.m_resource); }
	bool operator!=(const vsMaterial &b) const { return !((*this)==b); }

	static vsMaterial *	White;
};

#endif // VS_MATERIAL_H

