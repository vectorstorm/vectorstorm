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
	void			AsyncRead( vsTexture *texture );
	void			AsyncReadRenderTarget(vsRenderTarget *target, int buffer);
	bool			AsyncReadIsReady();

	// vsTexture *		Bake();

	vsStore *		BakePNG(int compression);
	void			SavePNG(int compression, const vsString& filename);
	void *			RawData() { return m_pixel; }
};

#endif // VS_SINGLEFLOATIMAGE_H

