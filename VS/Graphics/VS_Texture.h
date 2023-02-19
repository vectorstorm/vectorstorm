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
	enum Options
	{
		Option_ClampU = BIT(0),
		Option_ClampV = BIT(1),
		Option_LinearSampling = BIT(2),
	};
	uint8_t m_options;

public:
	vsTexture(const vsString &filename_in);
	vsTexture(const vsTexture &other);
	vsTexture(vsTexture *other); // deprecated
	vsTexture(vsTextureInternal *ti);
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

	// We set 'clamping' state on the vsTexture, so different vsTexture
	// instances using the same backing texture data can have different
	// settings for the texture.  These settings get copied if you make
	// duplicates of a vsTexture object, but are reset to default on vsTexture
	// objects made from a filename or vsTextureInternal.
	void SetClampU( bool u );
	void SetClampV( bool v );
	void SetClampUV( bool uv ) { SetClampU(uv); SetClampV(uv); }
	void SetClampUV( bool u, bool v ) { SetClampU(u); SetClampV(v); }
	bool GetClampU() const { return m_options & Option_ClampU; }
	bool GetClampV() const { return m_options & Option_ClampV; }

	bool GetLinearSampling() const { return m_options & Option_LinearSampling; }

	void SetLinearSampling();
	void SetNearestSampling();
};

#endif //VS_TEXTURE_H
