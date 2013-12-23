/*
 *  VS_OpenGL.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/11/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#if TARGET_OS_IPHONE
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#else
#include <SDL2/SDL.h>
#if defined(__APPLE_CC__)
#include <glew.h>
#else
#include <gl/glew.h>
#endif
#endif
