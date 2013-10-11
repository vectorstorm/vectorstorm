/*
 *  VS_RendererPretty.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_DisplayList.h"
#include "VS_RendererPretty.h"
#include "VS_Screen.h"

#include "VS_OpenGL.h"

vsRendererPretty::vsRendererPretty()
{
//	GLuint txtnumber;						// Texture ID
//	unsigned int* data;						// Stored Data
//	unsigned long len = ((128 * 128)* 4 * sizeof(unsigned int));
	
	// Create Storage Space For Texture Data (128x128x4)
//	data = (unsigned int*)new GLuint[len];
//	memset(data, 0, len);
	
//	glGenTextures(1, &txtnumber);
//	glBindTexture(GL_TEXTURE_2D, txtnumber);
//	glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
//	delete [] data;
	
//	m_offscreenTexture = txtnumber;
	
	
}

vsRendererPretty::~vsRendererPretty()
{
}

void
vsRendererPretty::PreRender(const Settings &s)
{
	Parent::PreRender(s);
	// give us thicker lines
	glLineWidth( 2.0f );
}

void
vsRendererPretty::RenderDisplayList( vsDisplayList *list )
{
	Parent::RenderDisplayList(list);
}

void
vsRendererPretty::PostRender()
{
	Parent::PostRender();
}

