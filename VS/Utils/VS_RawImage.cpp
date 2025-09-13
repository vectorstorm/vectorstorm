/*
 *  VS_RawImage.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/12/2024
 *  Copyright 2024 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RawImage.h"

#include "VS_TextureInternal.h"
#include "VS_Texture.h"

vsRawImage::vsRawImage( unsigned int width, unsigned int height, Format format, unsigned int channels ):
	m_format(format),
	m_channels(channels),
	m_width(width),
	m_height(height),
	m_pixels(nullptr)
{
	int channelSize = 1;
	switch ( format )
	{
		case Format_Byte:
			channelSize = 1;
			break;
		case Format_HalfInt:
			channelSize = 2;
			break;
		case Format_Int:
			channelSize = 4;
			break;
		case Format_HalfFloat:
			channelSize = 2;
			break;
		case Format_Float:
			channelSize = 4;
			break;
	}
	m_length = width * height * channels * channelSize;
	m_pixels = malloc( m_length );
}

vsRawImage::~vsRawImage()
{
	free( m_pixels );
}

vsTexture *
vsRawImage::Bake( const vsString& name_in ) const
{
	static int s_textureMakerCount = 0;
	vsString name(name_in);
	if ( name.empty() )
		name = vsFormatString("RawMakerTexture%d", s_textureMakerCount++);

	if ( vsCache<vsTextureInternal>::Instance()->Exists( name ) )
	{
		vsLog("Trying to re-bake a vsRawImage: %s", name_in);
		vsLog("We don't support doing that, right now!");
		// [TODO] Implement this
		//
		// texture for this screenshot already exists;  let's just blit over it!
		// vsTextureInternal *texture = vsCache<vsTextureInternal>::Instance()->Get( name );
		// texture->Blit(this, vsVector2D::Zero);
		// vsLog("Blitting new screenshot over existing texture!");
	}
	else
	{
		// vsLog("No existing texture!");

		vsTextureInternal *texture = new vsTextureInternal( name, this );
		vsCache<vsTextureInternal>::Instance()->Add( texture );
	}

	return new vsTexture(name);
}
