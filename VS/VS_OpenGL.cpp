/*
 *  VS_OpenGL.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/05/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_OpenGL.h"

#ifdef CHECK_GL_ERRORS
void CheckGLError(const char* string)
{
	char enums[][20] =
	{
		"invalid enumeration", // GL_INVALID_ENUM
		"invalid value",       // GL_INVALID_VALUE
		"invalid operation",   // GL_INVALID_OPERATION
		"stack overflow",      // GL_STACK_OVERFLOW
		"stack underflow",     // GL_STACK_UNDERFLOW
		"out of memory"        // GL_OUT_OF_MEMORY
	};

	GLenum errcode = glGetError();
	if (errcode == GL_NO_ERROR)
		return;

	errcode -= GL_INVALID_ENUM;

	vsString errString = vsFormatString("OpenGL %s in '%s'", enums[errcode], string);
	vsLog(errString);
	vsAssert(false,errString);
}
#endif
