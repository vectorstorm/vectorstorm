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

vsMaterial::vsMaterial():
	vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++))
{
}

vsMaterial::vsMaterial( const vsString &name ):
	vsCacheReference<vsMaterialInternal>(name)
{
}

vsMaterial::vsMaterial( vsMaterial *other ):
	vsCacheReference<vsMaterialInternal>(other->GetResource()->GetName())
{
}

