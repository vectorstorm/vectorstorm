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

#include "VS/Math/VS_Box.h"
#include "VS/Utils/VS_Cache.h"

class vsFloatImage;
class vsImage;
class vsRenderBuffer;
class vsRenderTarget;
class vsSurface;

class vsTextureInternal : public vsResource
{
	uint32_t		m_texture;

	int		m_glTextureWidth;
	int		m_glTextureHeight;

	int 		m_width;
	int 		m_height;
    bool        m_depth;

	bool		m_premultipliedAlpha;

	vsRenderBuffer *m_tbo;

	void		ProcessSurface( SDL_Surface *surface );


public:

	vsTextureInternal( const vsString &string );
	vsTextureInternal( const vsString &name, vsImage *image );
	vsTextureInternal( const vsString &name, vsFloatImage *image );
	vsTextureInternal( const vsString &name, vsSurface *surface, int surfaceBuffer=0, bool depth=false );
	vsTextureInternal( const vsString &name, vsRenderBuffer *buffer );
	~vsTextureInternal();

	void		Blit( vsImage *image, const vsVector2D& where);
	void		Blit( vsFloatImage *image, const vsVector2D& where);

	void		SetNearestSampling();
	void		SetLinearSampling();

	uint32_t		GetTexture() { return m_texture; }

	bool IsTextureBuffer() { return m_tbo != NULL; }
	vsRenderBuffer *GetTextureBuffer() { return m_tbo; }

	int		GetWidth() { return m_width; }
	int		GetHeight() { return m_height; }
    bool        IsDepth() { return m_depth; }

	int		GetGLWidth() { return m_glTextureWidth; }
	int		GetGLHeight() { return m_glTextureHeight; }

	uint32_t		ScaleColour(uint32_t ini, float amt);
	uint32_t		SafeAddColour(uint32_t a, uint32_t b);

	friend class vsRenderTarget;
};

#endif // VS_TEXTUREINTERNAL_H

