/*
 *  VS_Image.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 01-02-2012.
 *  Copyright 2012 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Image.h"

#include "VS_Color.h"
#include "VS_Texture.h"
#include "VS_TextureManager.h"
#include "VS_TextureInternal.h"

#include "VS_File.h"
#include "VS_Store.h"

#if !TARGET_OS_IPHONE
#ifdef __APPLE_CC__
#include <SDL_Image/SDL_Image.h>
#include <libpng15/png.h>
#else
#include "SDL_image.h"
#include <png.h>
#endif
#include "VS_OpenGL.h"

#ifndef _WIN32
#include <zlib.h>
#endif // _WIN32
#endif // TARGET_OS_IPHONE

#define LOAD_VIA_LIBPNG (0)

int vsImage::m_textureMakerCount = 0;

vsImage::vsImage(unsigned int width, unsigned int height):
	m_pixel(NULL),
	m_pixelCount(0),
	m_width(width),
	m_height(height)
{
	m_pixelCount = width * height;

	m_pixel = new vsColor[m_pixelCount];
}
#if LOAD_VIA_LIBPNG
static void png_read_data(png_structp png_ptr,png_bytep data, png_size_t length)
{
	vsStore *store = (vsStore *) png_get_io_ptr(png_ptr);

	size_t bytesRead = store->ReadBuffer( data, length );
	vsAssert(bytesRead == length, "Couldn't give as many bytes as requested??");
}
#endif

vsImage::vsImage( const vsString &filename_in )
{
#if LOAD_VIA_LIBPNG
	vsFile file(filename_in);
	vsStore store(file.GetLength());
	file.Store(&store);

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if ( png_ptr == NULL )
	{
		return;
	}
	png_info *info_ptr = png_create_info_struct(png_ptr);
	if ( png_ptr == NULL )
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return;
	}
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return;
	}
	// set up output control
	png_set_read_fn(png_ptr, (void*)&store, png_read_data);

	png_uint_32 width, height;
	int bit_depth, color_type;

	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	bool outHasAlpha = false;
	switch( color_type )
	{
		case PNG_COLOR_TYPE_RGBA:
			outHasAlpha = true;
			break;
		case PNG_COLOR_TYPE_RGB:
			outHasAlpha = false;
		default:
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			return;
	}

	// if we have already read some of the signature
	//png_set_sig_bytes(png_ptr, 8);
	//png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand(png_ptr);
	if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

	png_uint_32  i, rowbytes;
    uint8*  row_pointers[height];
	png_bytep image_data = NULL;

    png_read_update_info(png_ptr, info_ptr);

	rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    int channels = (int)png_get_channels(png_ptr, info_ptr);

    if ((image_data = (png_bytep)malloc(rowbytes*height)) == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return ;
    }

    for (i = 0;  i < height;  ++i)
        row_pointers[i] = image_data + i*rowbytes;

	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, NULL);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	// now, copy pixel data into ourself.

	m_pixelCount = width*height;
	m_pixel = new vsColor[m_pixelCount];
	for ( int y = 0; y < height; y++ )
	{
		uint8* shuttle = &row_pointers[y][0];
		for ( int x = 0; x < width; x++ )
		{
			int ci = (y*width) + x;
			m_pixel[ci].r = ( *(shuttle++) / 255.f );
			m_pixel[ci].g = ( *(shuttle++) / 255.f );
			m_pixel[ci].b = ( *(shuttle++) / 255.f );
			if ( channels == 4 )
			{
				m_pixel[ci].a = ( *(shuttle++) / 255.f );
			}
			else
			{
				m_pixel[ci].a = 1.f;
			}
		}
	}

	free(image_data);

#else // !LOAD_VIA_LIBPNG
#if !TARGET_OS_IPHONE
	vsString filename = vsFile::GetFullFilename(filename_in);
	SDL_Surface *loadedImage = IMG_Load(filename.c_str());
	vsAssert(loadedImage != NULL, vsFormatString("Unable to load texture %s: %s", filename.c_str(), IMG_GetError()));
	LoadFromSurface(loadedImage);
	SDL_FreeSurface(loadedImage);
#endif
#endif
}

vsImage::vsImage( vsTexture * texture )
{
    m_width = texture->GetResource()->GetWidth();
    m_height = texture->GetResource()->GetHeight();

    m_pixelCount = m_width * m_height;
    m_pixel = new vsColor[m_pixelCount];
    
	const size_t bytesPerPixel = 3;	// RGB
	const size_t imageSizeInBytes = bytesPerPixel * size_t(m_width) * size_t(m_height);

	uint8* pixels = new uint8[imageSizeInBytes];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
    glActiveTexture( GL_TEXTURE0 );
    glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture->GetResource()->GetTexture() );
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	glBindTexture( GL_TEXTURE_2D, 0 );
    GLenum errcode = glGetError();
	vsAssert(errcode == GL_NO_ERROR, "Error converting texture to image");

	for ( unsigned int y = 0; y < m_height; y++ )
	{
		int rowStart = y * m_width * bytesPerPixel;

		for ( unsigned int x = 0; x < m_width; x++ )
		{
			int rInd = rowStart + (x*bytesPerPixel);
			int gInd = rInd+1;
			int bInd = rInd+2;

			int rVal = pixels[rInd];
			int gVal = pixels[gInd];
			int bVal = pixels[bInd];

			Pixel(x,y).Set( rVal/255.f, gVal/255.f, bVal/255.f, 1.0f );
		}
	}

	vsDeleteArray( pixels );
}

vsImage::~vsImage()
{
	vsDeleteArray( m_pixel );
}

vsColor &
vsImage::Pixel(unsigned int u, unsigned int v)
{
	vsAssert(u >= 0 && u < m_width && v >= 0 && v < m_height, "Texel out of bounds!");

	return m_pixel[ PixelIndex(u,v) ];
}

void
vsImage::Clear( const vsColor &clearColor )
{
	for ( int i = 0; i < m_pixelCount; i++ )
	{
		m_pixel[i] = clearColor;
	}
}

vsTexture *
vsImage::Bake()
{
	vsString name = vsFormatString("MakerTexture%d", m_textureMakerCount++);

	vsTextureInternal *ti = new vsTextureInternal(name, this);
	vsTextureManager::Instance()->Add(ti);

	return new vsTexture(name);
}

void
vsImage::LoadFromSurface( SDL_Surface *source )
{
#if !TARGET_OS_IPHONE
	//	SDL_Surface *screen = SDL_GetVideoSurface();
	SDL_Rect	area;
    uint32 saved_flags;
    Uint8  saved_alpha;

	SDL_SetAlpha(source, 0, SDL_ALPHA_OPAQUE);

	m_width = source->w;
	m_height = source->h;

	int w = source->w;
	int h = source->h;

	SDL_Surface *image = SDL_CreateRGBSurface(
											  SDL_SWSURFACE,
											  w, h,
											  32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
											  0x000000FF,
											  0x0000FF00,
											  0x00FF0000,
											  0xFF000000
#else
											  0xFF000000,
											  0x00FF0000,
											  0x0000FF00,
											  0x000000FF
#endif
											  );
	vsAssert(image, "Error??");

    saved_flags = source->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
    saved_alpha = source->format->alpha;
    if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
        SDL_SetAlpha(source, 0, 0);
    }

    /* Copy the surface into the GL-format texture image, to make loading easier */
    area.x = 0;
    area.y = 0;
    area.w = source->w;
    area.h = source->h;
    SDL_BlitSurface(source, &area, image, &area);

    /* Restore the alpha blending attributes */
    if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
        SDL_SetAlpha(source, saved_flags, saved_alpha);
    }

	// now lets copy our image data

    m_pixelCount = w*h;
	m_pixel = new vsColor[m_pixelCount];

    for ( int v = 0; v < h; v++ )
    {
        for ( int u = 0; u < w; u++ )
        {
            int i = v*image->pitch + u*4;
            int ri = i;
            int gi = ri+1;
            int bi = ri+2;
            int ai = ri+3;

            unsigned char r = ((unsigned char*)image->pixels)[ri];
            unsigned char g = ((unsigned char*)image->pixels)[gi];
            unsigned char b = ((unsigned char*)image->pixels)[bi];
            unsigned char a = ((unsigned char*)image->pixels)[ai];

			// flip our image.  Our image is stored upside-down, relative to a standard SDL Surface.
            Pixel(u,(w-1)-v) = vsColor( r / 255.f, g / 255.f, b / 255.f, a / 255.f );
        }
    }

	SDL_FreeSurface(image); /* No longer needed */
#endif // TARGET_OS_IPHONE
}

#if !defined(_WIN32) && !TARGET_OS_IPHONE
static void png_write_data(png_structp png_ptr,png_bytep data, png_size_t length)
{
	vsStore *store = (vsStore *) png_get_io_ptr(png_ptr);

	store->WriteBuffer( data, length );
}
#endif // _WIN32

vsStore *
vsImage::BakePNG(int compression)
{
#if !defined(_WIN32) && !TARGET_OS_IPHONE
	png_structp png_ptr;
	png_infop info_ptr;
	//int ret,funky_format;
	unsigned int i;
	png_colorp palette;
	uint8 *palette_alpha=NULL;
	png_byte **row_pointers=NULL;
	png_ptr=NULL;info_ptr=NULL;palette=NULL;//ret=-1;
	//funky_format=0;
	const int c_bytesPerPixel = 4;
	uint8 *pixels = NULL;

	vsStore *result = new vsStore(2048 * 1024);

	row_pointers=(png_byte **)malloc(m_height * sizeof(png_byte*));
	if (!row_pointers) {
		vsAssert(row_pointers, "Couldn't allocate memory for rowpointers");
		goto savedone;
	}

	png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL,NULL);
	if (!png_ptr){
		vsAssert(png_ptr, "Couldn't allocate memory for PNG file");
		goto savedone;
	}
	info_ptr= png_create_info_struct(png_ptr);
	if (!info_ptr){
		vsAssert(info_ptr, "Couldn't allocate image information for PNG file");
		goto savedone;
	}
	/* setup custom writer functions */
	png_set_write_fn(png_ptr,(void*)result,png_write_data,NULL);

	if (setjmp(png_jmpbuf(png_ptr))){
		vsAssert(0, "Unknown error writing PNG");
		goto savedone;
	}

	if(compression>Z_BEST_COMPRESSION)
		compression=Z_BEST_COMPRESSION;

	if(compression == Z_NO_COMPRESSION) // No compression
	{
		png_set_filter(png_ptr,0,PNG_FILTER_NONE);
		png_set_compression_level(png_ptr,Z_NO_COMPRESSION);
	}
	else if(compression<0) // Default compression
		png_set_compression_level(png_ptr,Z_DEFAULT_COMPRESSION);
	else
		png_set_compression_level(png_ptr,compression);

	png_set_IHDR(png_ptr,info_ptr,
				 m_width,m_height,8,PNG_COLOR_TYPE_RGB_ALPHA,
				 PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);

	//png_set_sRGB_gAMA_and_cHRM(png_ptr, info_ptr, PNG_sRGB_INTENT_RELATIVE);
	png_write_info(png_ptr, info_ptr);

	pixels = new uint8[m_width*m_height*c_bytesPerPixel];
	for ( unsigned int y = 0; y < m_height; y++ )
	{
		int rowStart = y * m_width * c_bytesPerPixel;

		for ( unsigned int x = 0; x < m_width; x++ )
		{
			int rInd = rowStart + (x*c_bytesPerPixel);
			int gInd = rInd+1;
			int bInd = rInd+2;
			int aInd = rInd+3;

			pixels[rInd] = Pixel(x,y).r * 255.f;
			pixels[gInd] = Pixel(x,y).g * 255.f;
			pixels[bInd] = Pixel(x,y).b * 255.f;
			pixels[aInd] = Pixel(x,y).a * 255.f;
		}
	}

	for(i=0;i<m_height;i++)
	{
		// OpenGL numbers its rows from BOTTOM TO TOP.
		// PNG numbers its rows from TOP TO BOTTOM.

		int y = (m_height-1) - i;	// so PNG row 'i' is OpenGL row 'y'.

		row_pointers[i]= (png_byte*)&pixels[ y*m_width*c_bytesPerPixel ];
	}
	png_write_image(png_ptr, row_pointers);

	png_write_end(png_ptr, NULL);
	//ret=0; /* got here, so nothing went wrong. YAY! */

	vsDeleteArray( pixels );

savedone: /* clean up and return */
	png_destroy_write_struct(&png_ptr,&info_ptr);
	if (palette) {
		free(palette);
	}
	if (palette_alpha) {
		free(palette_alpha);
	}
	if (row_pointers) {
		free(row_pointers);
	}
	return result;
#endif // _WIN32 || TARGET_OS_IPHONE

	return NULL;
}

