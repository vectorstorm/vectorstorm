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
private:
	uint32_t*		m_pixel;
	int				m_pixelCount;

	unsigned int	m_width;
	unsigned int	m_height;

	static int		s_textureMakerCount;
	static bool		s_allowLoadFailure;

	uint32_t m_pbo;
	GLsync m_sync;

	int				PixelIndex(int u, int v) const { return u + (v*m_width); }

protected:

	void		CopyTo( vsImage *other );
public:

    vsImage();
	vsImage( unsigned int width, unsigned int height );
    vsImage( const vsString &filename_in );
    vsImage( const vsStore &filedata_in );
    vsImage( vsTexture *texture );
	~vsImage();

	// sometimes, we're going to use a single vsImage lots of times for repeatedly
	// reading back a single texture.  In that case, you can call this Prep function
	// to pre-allocate space, so that it happens at a desirable time, instead of
	// stuttering the first time you read back data.
	void			PrepForAsyncRead( vsTexture *texture );
	bool			IsOK() { return m_pixel != nullptr; }

	// ReadFromFileData() is called with a vsStore of an image.  It should work
	// with either PNG or JPG, or anything else that stb_image supports loading.
	void			ReadFromFileData( const vsStore &store );
	void			Read( vsTexture *texture );
	void			AsyncRead( vsTexture *texture );
	void			AsyncReadRenderTarget(vsRenderTarget *target, int buffer);
	bool			AsyncReadIsReady();

	void			AsyncMap(); // map our async-read data into ourselves so we can be accessed to get pixels directly
	void			AsyncUnmap(); // unmap

	vsImage *		CreateFlipped_V();
	vsImage *		CreateOpaque();

	int				GetWidth() const { return m_width; }
	int				GetHeight() const { return m_height; }

	// 'u' is [0..width-1] from left to right.
	// 'v' is [0..height-1] from bottom to top.
	vsColor			GetPixel(unsigned int u, unsigned int v) const;
	void			SetPixel(unsigned int u, unsigned int v, const vsColor &c);

	uint32_t		GetRawPixel(unsigned int u, unsigned int v) const;
	void			SetRawPixel(unsigned int u, unsigned int v, uint32_t c);

	void			Clear( const vsColor &clearColor );
	void			Copy( vsImage *other );

	vsTexture *		Bake( const vsString& name = vsEmptyString );

	void			SavePNG(const vsString& filename);
	void			SaveJPG(int quality, const vsString& filename); // quality is in [0..100], with higher values being higher quality.
	void			SavePNG_FullAlpha(const vsString& filename);
	void *			RawData() { return m_pixel; }


	static void SetAllowLoadFailure(bool allow) { s_allowLoadFailure = allow; }
};

#endif // VS_IMAGE_H

