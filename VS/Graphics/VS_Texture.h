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

#include "VS_TextureInternal.h"
#include "VS/Utils/VS_Cache.h"
class vsRenderBuffer;

class vsTexture : public vsCacheReference<vsTextureInternal>
{
public:
	vsTexture(const vsString &filename_in);
	vsTexture(vsTexture *other);
	~vsTexture();

	/**
	 * Utility function to create a new buffer texture from a render buffer.  Handles
	 * the internal details of registering with the texture manager and etc.
	 *
	 * [WARNING] This interface does not take ownership of the buffer data; the
	 * vsRenderBuffer object must continue to exist for as long as the returned
	 * texture does!
	 */
	static vsTexture* MakeBufferTexture( const vsString& name, vsRenderBuffer *buffer );

};

#endif //VS_TEXTURE_H
