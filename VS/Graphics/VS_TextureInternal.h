/*
 *  VS_TextureInternal.h
 *  Lord
 *
 *  Created by Trevor Powell on 25/02/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_TEXTUREINTERNAL_H
#define VS_TEXTUREINTERNAL_H

struct SDL_Surface;

#include "VS/Utils/VS_Cache.h"

class vsImage;
class vsRenderTarget;
class vsSurface;

class vsTextureInternal : public vsResource
{
	uint32		m_texture;

	float		m_glTextureWidth;
	float		m_glTextureHeight;

	float		m_width;
	float		m_height;
    bool        m_depth;

	int			m_refCount;

	bool		m_premultipliedAlpha;

	void		ProcessSurface( SDL_Surface *surface );


public:

	vsTextureInternal( const vsString &string );
	vsTextureInternal( const vsString &name, vsImage *image );
	vsTextureInternal( const vsString &name, vsSurface *surface, bool depth=false );
	~vsTextureInternal();

	uint32		GetTexture() { return m_texture; }

	void		AddReference() { m_refCount++; }
	void		RemoveReference() { m_refCount--; }
	int			GetReferenceCount() { return m_refCount; }

	float		GetWidth() { return m_width; }
	float		GetHeight() { return m_height; }
    bool        IsDepth() { return m_depth; }

	float		GetGLWidth() { return m_glTextureWidth; }
	float		GetGLHeight() { return m_glTextureHeight; }

	uint32		ScaleColour(uint32 ini, float amt);
	uint32		SafeAddColour(uint32 a, uint32 b);

	friend class vsRenderTarget;
};

#endif // VS_TEXTUREINTERNAL_H

