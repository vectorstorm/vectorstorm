/*
 *  VS_Texture.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Texture.h"
#include "VS_TextureInternal.h"
#include "VS_TextureManager.h"
#include "VS/Files/VS_File.h"

vsTexture::vsTexture(const vsString &filename_in):
	vsCacheReference<vsTextureInternal>(filename_in)
{
}

vsTexture::vsTexture( vsTexture *other ):
	vsCacheReference<vsTextureInternal>( other )
{
}

vsTexture::~vsTexture()
{
}

