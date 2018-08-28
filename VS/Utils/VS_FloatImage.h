/*
 *  VS_FloatImage.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 24/12/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_FLOATIMAGE_H
#define VS_FLOATIMAGE_H

#include "VS/Graphics/VS_Texture.h"

class vsStore;
class vsColor;

class vsFloatImage
{
	vsColor*		m_pixel;
	int				m_pixelCount;

	unsigned int	m_width;
	unsigned int	m_height;

	static int		m_textureMakerCount;

	int				PixelIndex(int u, int v) const { return u + (v*m_width); }

    void            LoadFromSurface( SDL_Surface *source );

public:

	vsFloatImage( unsigned int width, unsigned int height );
    vsFloatImage( const vsString &filename_in );
    vsFloatImage( vsTexture *texture );
	~vsFloatImage();

	int				GetWidth() { return m_width; }
	int				GetHeight() { return m_height; }

	vsColor			GetPixel(unsigned int u, unsigned int v) const;
	void			SetPixel(unsigned int u, unsigned int v, const vsColor &c);

	void			Clear( const vsColor &clearColor );
	void			Copy( vsFloatImage *other );

	void			Read( vsTexture *texture );
	vsTexture *		Bake();

	vsStore *		BakePNG(int compression);
	void			SavePNG(int compression, const vsString& filename);
	void *			RawData() { return m_pixel; }
};

#endif // VS_FLOATIMAGE_H

