/*
 *  VS_TextureManager.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_TEXTURE_MANAGER_H
#define VS_TEXTURE_MANAGER_H

#include "VS_Texture.h"

class vsTextureInternal;

#include "VS/Utils/VS_Cache.h"

class vsTextureManager : public vsCache<vsTextureInternal>
{
public:
	
	vsTextureManager();

	vsTextureInternal *	LoadTexture( const vsString &name );
};


#endif // VS_TEXTURE_MANAGER_H

