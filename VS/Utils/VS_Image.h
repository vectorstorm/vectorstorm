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

class vsStore;
class vsColor;

class vsImage
{
	vsColor *		m_pixel;
	int				m_pixelCount;

	unsigned int				m_width;
	unsigned int				m_height;

	static int		m_textureMakerCount;

	int				PixelIndex(int u, int v) { return u + (v*m_width); }

    void            LoadFromSurface( SDL_Surface *source );

public:

	vsImage( unsigned int width, unsigned int height );
    vsImage( const vsString &filename_in );
    vsImage( vsTexture *texture );
	~vsImage();

	int				GetWidth() { return m_width; }
	int				GetHeight() { return m_height; }

	vsColor &		Pixel(unsigned int u, unsigned int v);

	void			Clear( const vsColor &clearColor );

	vsTexture *		Bake();
	vsStore *		BakePNG(int compression);
};

#endif // VS_IMAGE_H

