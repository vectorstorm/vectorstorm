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

#define DEFAULT_OPTIONS ( vsTexture::Option_LinearSampling )

vsTexture::vsTexture(const vsString &filename_in):
	vsCacheReference<vsTextureInternal>(filename_in),
	m_options(DEFAULT_OPTIONS)
{
}

vsTexture::vsTexture( vsTexture *other ):
	vsCacheReference<vsTextureInternal>( other ),
	m_options(other->m_options)
{
}

vsTexture::vsTexture(const vsTexture &other):
	vsCacheReference<vsTextureInternal>( &other ),
	m_options(other.m_options)
{
}

vsTexture::vsTexture(vsTextureInternal *ti):
	vsCacheReference<vsTextureInternal>( ti ),
	m_options(DEFAULT_OPTIONS)
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

void
vsTexture::SetClampU( bool u )
{
	if ( u )
		m_options |= Option_ClampU;
	else
		m_options &= ~Option_ClampU;
}

void
vsTexture::SetClampV( bool v )
{
	if ( v )
		m_options |= Option_ClampV;
	else
		m_options &= ~Option_ClampV;
}

void
vsTexture::SetLinearSampling()
{
	m_options |= Option_LinearSampling;
}

void
vsTexture::SetNearestSampling()
{
	m_options &= ~Option_LinearSampling;
}


