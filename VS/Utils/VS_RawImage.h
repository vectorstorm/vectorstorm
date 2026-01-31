/*
 *  VS_RawImage.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/12/2024
 *  Copyright 2024 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RAWIMAGE_H
#define VS_RAWIMAGE_H

// For use if you have some weird image format that you need to pass into the
// renderer for some reason, and don't need to access from the CPU side except
// to load the data in.

class vsTexture;

class vsRawImage
{
public:

	enum Format
	{
		Format_Byte,
		Format_HalfInt,
		Format_Int,
		Format_HalfFloat,
		Format_Float
	};
private:

	Format m_format;
	int m_channels;
	int m_width;
	int m_height;
	void* m_pixels;
	int m_length;

public:
	vsRawImage( unsigned int width, unsigned int height, Format format, unsigned int channels );
	~vsRawImage();

	const void* RawData() const { return m_pixels; }
	void* RawData() { return m_pixels; }
	Format GetFormat() const { return m_format; }
	int GetChannels() const { return m_channels; }
	int GetLength() const { return m_length; }
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }

	vsTexture *		Bake( const vsString& name = vsEmptyString ) const;
};

#endif // VS_RAWIMAGE_H

