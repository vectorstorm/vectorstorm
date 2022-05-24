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
#include "VS_RenderBuffer.h"
#include "VS/Files/VS_File.h"

vsTexture::vsTexture(const vsString &filename_in):
	vsCacheReference<vsTextureInternal>(filename_in)
{
}

vsTexture::vsTexture( vsTexture *other ):
	vsCacheReference<vsTextureInternal>( other )
{
}

vsTexture::vsTexture(vsTextureInternal *ti):
	vsCacheReference<vsTextureInternal>( ti )
{
}

vsTexture::~vsTexture()
{
}

vsTexture*
vsTexture::MakeBufferTexture( const vsString& name, vsRenderBuffer *buffer )
{
	vsTextureInternal *t = new vsTextureInternal(name, buffer);
	vsTextureManager::Instance()->Add(t);
	return new vsTexture(t->GetName());
}
