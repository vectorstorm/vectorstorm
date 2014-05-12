/*
 *  VS_OpenGL.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/11/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_OPENGL_H
#define VS_OPENGL_H

#if TARGET_OS_IPHONE
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#else
#include <SDL2/SDL.h>
#if defined(__APPLE_CC__)
#include <glew.h>
#else
#include <GL/glew.h>
#endif
#endif

#define CHECK_GL_ERRORS
#ifdef CHECK_GL_ERRORS
	void			CheckGLError(const char* string);
#else
	inline void			CheckGLError(const char* string) {}
#endif

#endif //VS_OPENGL_H
