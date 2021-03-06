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

#include "VS/Math/VS_Box.h"
#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_Cache.h"

class vsFloatImage;
class vsImage;
class vsHalfIntImage;
class vsHalfFloatImage;
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
	vsRenderTarget *m_renderTarget; // this is NOT owned by us!
	int				m_surfaceBuffer; // which buffer within the renderTarget was this?

	bool		m_nearestSampling;

public:

	vsTextureInternal( const vsString &string );
	vsTextureInternal( const vsString &name, const vsArray<vsString> &mipmaps );
	vsTextureInternal( const vsString &name, vsImage *image );
	vsTextureInternal( const vsString &name, vsFloatImage *image );
	vsTextureInternal( const vsString &name, vsHalfFloatImage *image );
	vsTextureInternal( const vsString &name, vsHalfIntImage *image );
	vsTextureInternal( const vsString &name, vsRenderTarget *renderTarget, int surfaceBuffer=0, bool depth=false );
	vsTextureInternal( const vsString &name, vsRenderBuffer *buffer );

	// SetRenderTarget() is for filling in the 'surface' later, if we were
	// created for a surface without actually having allocated everything yet.
	void SetRenderTarget( vsRenderTarget* renderTarget, int surfaceBuffer, bool depth );

	// for hooking up to OpenGL textures created elsewhere.
	// TODO:  THIS SHOULD GO AWAY!  Textures should all be created by VectorStorm!
	vsTextureInternal( const vsString &name, uint32_t glTextureId );

	~vsTextureInternal();

	void		PrepareToBind(); // called immediately before we're bound for rendering

	void		Blit( vsImage *image, const vsVector2D& where);
	void		Blit( vsFloatImage *image, const vsVector2D& where);

	void		SetNearestSampling();
	void		SetLinearSampling(bool linearMipmaps = true);

	uint32_t		GetTexture() { return m_texture; }

	bool IsTextureBuffer() { return m_tbo != NULL; }
	vsRenderBuffer *GetTextureBuffer() { return m_tbo; }

	int		GetWidth() { return m_width; }
	int		GetHeight() { return m_height; }
    bool        IsDepth() { return m_depth; }

	void	ClampUV( bool u, bool v );

	int		GetGLWidth() { return m_glTextureWidth; }
	int		GetGLHeight() { return m_glTextureHeight; }

	uint32_t		ScaleColour(uint32_t ini, float amt);
	uint32_t		SafeAddColour(uint32_t a, uint32_t b);

	friend class vsRenderTarget;
};

#endif // VS_TEXTUREINTERNAL_H

