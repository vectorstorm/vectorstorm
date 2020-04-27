/*
 *  VS_HalfImage.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/04/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_HALFIMAGE_H
#define VS_HALFIMAGE_H

class vsColor;
class vsTexture;
// uses halfint for each channel.  Confusingly, a vsHalfIntImage uses twice
// as much data as a vsImage.  Sorry!  (This is because the vsImage is actually
// a vsQuarterImage, as it's using a quarter integer for each color channel)
class vsHalfIntImage
{
private:
	uint64_t*		m_pixel;
	int				m_pixelCount;

	unsigned int	m_width;
	unsigned int	m_height;

	static int		s_textureMakerCount;
	static bool		s_allowLoadFailure;

	int				PixelIndex(int u, int v) const { return u + (v*m_width); }

protected:

public:

	vsHalfIntImage( unsigned int width, unsigned int height );
	~vsHalfIntImage();

	int				GetWidth() const { return m_width; }
	int				GetHeight() const { return m_height; }

	vsColor			GetPixel(unsigned int u, unsigned int v) const;
	void			SetPixel(unsigned int u, unsigned int v, const vsColor &c);

	uint64_t		GetRawPixel(unsigned int u, unsigned int v) const;
	void			SetRawPixel(unsigned int u, unsigned int v, uint64_t c);

	vsTexture *		Bake();
	void *			RawData() { return m_pixel; }
};

#endif // VS_HALFIMAGE_H

