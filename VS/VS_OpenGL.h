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
#include <SDL3/SDL.h>
#if defined(__APPLE_CC__)
#include <GL/glew.h>
#else
#if !defined(MSVC)
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#endif
#endif


#ifdef VS_GL_DEBUG

#define GL_CHECK(s) CheckGLError(s);
#define GL_CHECK_SCOPED(s) vsGLContext glContextTester(s, __FILE__, __LINE__);

// create a vsGLContext and it will check for GL errors both when it's created
// and when it's destroyed.
class vsGLContext
{
	const char* m_string;
	const char* m_file;
	int m_line;
public:
	vsGLContext( const char* string, const char* file, int line);
	~vsGLContext();
};
void CheckGLError(const char* string);



#else
#define GL_CHECK(s) {}
#define GL_CHECK_SCOPED(s) {}
#endif

#endif //VS_OPENGL_H
