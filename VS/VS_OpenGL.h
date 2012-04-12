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
#include <SDL/SDL.h>
#if defined(__APPLE_CC__)
#include <glew/glew.h>
#else
#include "glew.h"
#endif
#endif
