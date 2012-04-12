/*
 *  VS_Texture.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/08/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_TEXTURE_H
#define VS_TEXTURE_H

//class vsTextureInternal;
#include "VS_TextureInternal.h"
#include "VS/Utils/VS_Cache.h"

class vsTexture : public vsCacheReference<vsTextureInternal>
{
public:
	vsTexture(const vsString &filename_in);
	vsTexture(vsTexture *other);
	~vsTexture();
	
};

#endif //VS_TEXTURE_H
