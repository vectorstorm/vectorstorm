/*
 *  VS_HalfFloatImage.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 27/04/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_HALFFLOATIMAGE_H
#define VS_HALFFLOATIMAGE_H

class vsColor;
class vsTexture;
// uses halfint for each channel.  Confusingly, a vsHalfFloatImage uses twice
// as much data as a vsImage.  Sorry!  (This is because the vsImage is actually
// a vsQuarterImage, as it's using a quarter integer for each color channel)
class vsHalfFloatImage
{
private:
	vsColor*		m_pixel;
	int				m_pixelCount;

	unsigned int	m_width;
	unsigned int	m_height;

	static bool		s_allowLoadFailure;

	int				PixelIndex(int u, int v) const { return u + (v*m_width); }

protected:

public:

	vsHalfFloatImage( unsigned int width, unsigned int height );
	~vsHalfFloatImage();

	int				GetWidth() const { return m_width; }
	int				GetHeight() const { return m_height; }

	vsColor			GetPixel(unsigned int u, unsigned int v) const;
	void			SetPixel(unsigned int u, unsigned int v, const vsColor &c);

	vsColor			GetRawPixel(unsigned int u, unsigned int v) const;
	void			SetRawPixel(unsigned int u, unsigned int v, const vsColor& c);

	vsTexture *		Bake( const vsString& name = vsEmptyString ) const;
	void *			RawData() { return m_pixel; }
	const void *	RawData() const { return m_pixel; }
};

#endif // VS_HALFFLOATIMAGE_H

