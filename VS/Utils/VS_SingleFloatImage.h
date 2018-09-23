/*
 *  VS_SingleSingleFloatImage.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 29/08/2018
 *  Copyright 2018 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SINGLEFLOATIMAGE_H
#define VS_SINGLEFLOATIMAGE_H

#include "VS/Graphics/VS_Texture.h"
#include "VS_OpenGL.h"

class vsStore;
class vsColor;

class vsSingleFloatImage
{
	float*			m_pixel;
	int				m_pixelCount;

	unsigned int	m_width;
	unsigned int	m_height;

	static int		m_textureMakerCount;

	int				PixelIndex(int u, int v) const { return u + (v*m_width); }

    void            LoadFromSurface( SDL_Surface *source );

	uint32_t m_pbo;
	GLsync m_sync;

protected:

	void		CopyTo( vsSingleFloatImage *other );

public:

	vsSingleFloatImage();
	vsSingleFloatImage( unsigned int width, unsigned int height );
    vsSingleFloatImage( const vsString &filename_in );
    vsSingleFloatImage( vsTexture *texture );
	~vsSingleFloatImage();

	int				GetWidth() { return m_width; }
	int				GetHeight() { return m_height; }

	float			GetPixel(unsigned int u, unsigned int v) const;
	void			SetPixel(unsigned int u, unsigned int v, float c);

	void			Clear( float clearValue );
	void			Copy( vsSingleFloatImage *other );
	void			Read( vsTexture *texture );

	// sometimes, we're going to use a single vsImage lots of times for repeatedly
	// reading back a single texture.  In that case, you can call this Prep function
	// to pre-allocate space, so that it happens at a desirable time, instead of
	// stuttering the first time you read back data.
	void			PrepForAsyncRead( vsTexture *texture );

	void			AsyncRead( vsTexture *texture );
	void			AsyncReadRenderTarget(vsRenderTarget *target, int buffer);
	bool			AsyncReadIsReady();

	void			AsyncMap(); // map our async-read data into ourselves so we can be accessed to get pixels directly
	void			AsyncUnmap(); // unmap

	// vsTexture *		Bake();

	vsStore *		BakePNG(int compression);
	void			SavePNG(int compression, const vsString& filename);
	void *			RawData() { return m_pixel; }
};

#endif // VS_SINGLEFLOATIMAGE_H

