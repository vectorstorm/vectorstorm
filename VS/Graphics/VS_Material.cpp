/*
 *  VS_Material.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Material.h"
#include "VS_MaterialInternal.h"

#include "VS_Texture.h"

static int	c_codeMaterialCount = 0;

vsMaterial *vsMaterial::White = NULL;

/*vsMaterial::vsMaterial():
vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++))
{
}
*/
vsMaterial::vsMaterial( const vsString &name ):
	vsCacheReference<vsMaterialInternal>(name)
{
}

vsMaterial::vsMaterial( vsMaterial *other ):
vsCacheReference<vsMaterialInternal>(other->GetResource()->GetName())
{
}

vsMaterial::vsMaterial( const vsString & textureName, vsDrawMode mode ):
	vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++))
{
	// I believe that this is only being used by the font system right now.
	
	vsMaterialInternal *mi = reinterpret_cast<vsMaterialInternal *>(m_resource);
	mi->m_texture = new vsTexture(textureName);
	mi->m_drawMode = mode;
	mi->m_hasColor = false;
	mi->m_zRead = false;
	mi->Compile();
}

vsMaterial::vsMaterial( const vsString &textureName, vsDrawMode mode, const vsColor &c, const vsColor &sc):
	vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++))
{
	vsMaterialInternal *mi = reinterpret_cast<vsMaterialInternal *>(m_resource);
	mi->m_texture = new vsTexture(textureName);
	mi->m_drawMode = mode;
	mi->m_color = c;
	mi->m_specularColor = sc;
	mi->Compile();
}

vsMaterial::vsMaterial( vsTexture *texture, vsDrawMode mode, const vsColor &c, const vsColor &sc):
vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++))
{
	vsMaterialInternal *mi = reinterpret_cast<vsMaterialInternal *>(m_resource);
	mi->m_texture = new vsTexture(texture);
	mi->m_drawMode = mode;
	mi->m_color = c;
	mi->m_specularColor = sc;
	mi->Compile();
}

vsMaterial::vsMaterial( vsDrawMode mode, const vsColor &c, const vsColor &sc):
	vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++))
{
	vsMaterialInternal *mi = reinterpret_cast<vsMaterialInternal *>(m_resource);
	mi->m_texture = NULL;
	mi->m_drawMode = mode;
	mi->m_color = c;
	mi->m_specularColor = sc;
	mi->Compile();
}

