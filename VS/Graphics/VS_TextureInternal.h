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
class vsSingleFloatImage;
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

	bool		m_lockedSampling;

	vsRenderBuffer *m_tbo;
	vsRenderTarget *m_renderTarget; // this is NOT owned by us!
	int				m_surfaceBuffer; // which buffer within the renderTarget was this?

	// bool		m_nearestSampling;

	enum
	{
		State_ClampU = BIT(0),
		State_ClampV = BIT(1),
		State_LinearSampling = BIT(2),
		State_Mipmap = BIT(3)
	};
	uint8_t m_state;

public:

	vsTextureInternal( const vsString &string );
	vsTextureInternal( const vsString &name, const vsArray<vsString> &mipmaps );
	vsTextureInternal( const vsString &name, const vsImage *image );
	vsTextureInternal( const vsString &name, const vsFloatImage *image );
	vsTextureInternal( const vsString &name, const vsHalfFloatImage *image );
	vsTextureInternal( const vsString &name, const vsSingleFloatImage *image );
	vsTextureInternal( const vsString &name, const vsHalfIntImage *image );
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
	void		Blit( vsSingleFloatImage *image, const vsVector2D& where);

	// void		SetNearestSampling();
	// void		SetLinearSampling(bool linearMipmaps = true);

	uint32_t		GetTexture() const { return m_texture; }

	bool IsTextureBuffer() const { return m_tbo != nullptr; }
	vsRenderBuffer *GetTextureBuffer() { return m_tbo; }

	int		GetWidth() const { return m_width; }
	int		GetHeight() const { return m_height; }
    bool        IsDepth() const { return m_depth; }

	bool IsSamplingLocked() const { return m_lockedSampling; }

	// ===============================================================================
	// used as a cache during rendering, so we can remember what render state
	// is set on this texture.
	bool IsClampedU() const { return (m_state & State_ClampU) != 0; }
	bool IsClampedV() const { return (m_state & State_ClampV) != 0; }

	void SetClampedU(bool c) { if ( c ) m_state |= State_ClampU; else m_state &= ~State_ClampU; }
	void SetClampedV(bool c) { if ( c ) m_state |= State_ClampV; else m_state &= ~State_ClampV; }

	bool IsLinearSampling() const { return (m_state & State_LinearSampling) != 0; }
	void SetLinearSampling(bool linear) { if ( linear ) m_state |= State_LinearSampling; else m_state &= ~State_LinearSampling; }

	bool IsUseMipmap() const { return (m_state & State_Mipmap) != 0; }
	void SetUseMipmap(bool mipmap) { if ( mipmap ) m_state |= State_Mipmap; else m_state &= ~State_Mipmap; }
	// ===============================================================================

	// void	ApplyClampUV( bool u, bool v );
	// void	ClampUV( bool u, bool v );

	int		GetGLWidth() { return m_glTextureWidth; }
	int		GetGLHeight() { return m_glTextureHeight; }

	uint32_t		ScaleColour(uint32_t ini, float amt);
	uint32_t		SafeAddColour(uint32_t a, uint32_t b);

	friend class vsRenderTarget;
};

#endif // VS_TEXTUREINTERNAL_H

