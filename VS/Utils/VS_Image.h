/*
 *  VS_Image.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 01-02-2012.
 *  Copyright 2012 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_IMAGE_H
#define VS_IMAGE_H

#include "VS/Graphics/VS_Texture.h"
#include "VS_OpenGL.h"

class vsStore;
class vsColor;

class vsImage
{
	uint32_t*		m_pixel;
	int				m_pixelCount;

	unsigned int	m_width;
	unsigned int	m_height;

	static int		m_textureMakerCount;

	uint32_t m_pbo;
	GLsync m_sync;

	int				PixelIndex(int u, int v) const { return u + (v*m_width); }

    void            LoadFromSurface( SDL_Surface *source );
protected:

	void		CopyTo( vsImage *other );
public:

	vsImage( unsigned int width, unsigned int height );
    vsImage( const vsString &filename_in );
    vsImage( vsTexture *texture );
	~vsImage();

	void			Read( vsTexture *texture );
	void			AsyncRead( vsTexture *texture );
	void			AsyncReadRenderTarget(vsRenderTarget *target, int buffer);
	bool			AsyncReadIsReady();

	int				GetWidth() { return m_width; }
	int				GetHeight() { return m_height; }

	vsColor			GetPixel(unsigned int u, unsigned int v) const;
	void			SetPixel(unsigned int u, unsigned int v, const vsColor &c);

	uint32_t		GetRawPixel(unsigned int u, unsigned int v) const;
	void			SetRawPixel(unsigned int u, unsigned int v, uint32_t c);

	void			Clear( const vsColor &clearColor );
	void			Copy( vsImage *other );

	vsTexture *		Bake();

	vsStore *		BakePNG(int compression);
	void			SavePNG(int compression, const vsString& filename);
	void			SavePNG_FullAlpha(int compression, const vsString& filename);
	void *			RawData() { return m_pixel; }
};

#endif // VS_IMAGE_H

