/*
 *  VS_TextureManager.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_TextureManager.h"

#include "VS_Texture.h"
#include "VS_Image.h"
#include "VS_TextureInternal.h"

vsTextureManager::vsTextureManager():
	vsCache<vsTextureInternal>(512)
{
}

vsTextureInternal *
vsTextureManager::LoadTexture( const vsString &filename )
{
	return Get( filename );
}
