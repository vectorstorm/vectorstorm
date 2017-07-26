/*
 *  VS_OpenGL.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/05/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_OpenGL.h"
#include "VS_Profile.h"

void ReportGLError( GLenum errcode, const char* string )
{
	if (errcode == GL_NO_ERROR)
		return;

	char enums[][20] =
	{
		"invalid enumeration", // GL_INVALID_ENUM
		"invalid value",       // GL_INVALID_VALUE
		"invalid operation",   // GL_INVALID_OPERATION
		"stack overflow",      // GL_STACK_OVERFLOW
		"stack underflow",     // GL_STACK_UNDERFLOW
		"out of memory"        // GL_OUT_OF_MEMORY
	};

	errcode -= GL_INVALID_ENUM;

	vsString errString = vsFormatString("OpenGL %s in '%s'", enums[errcode], string);
	vsLog(errString);
	vsAssert(false,errString);
}

#ifdef VS_GL_DEBUG
void CheckGLError(const char* string)
{
	PROFILE("CheckGLError");
	GLenum errcode = glGetError();
	if ( errcode != GL_NO_ERROR )
		ReportGLError(errcode, string);
}

void ReportGLError( GLenum errcode, const char* string );

vsGLContext::vsGLContext( const char* string, const char* file, int line ):
	m_string(string),
	m_file(file),
	m_line(line)
{
	GLenum errcode = glGetError();
	if ( errcode != GL_NO_ERROR )
	{
		vsString s = vsFormatString("%s-Pre (%s:%d)", m_string, m_file, m_line);
		ReportGLError(errcode, s.c_str());
	}
}

vsGLContext::~vsGLContext()
{
	GLenum errcode = glGetError();
	if ( errcode != GL_NO_ERROR )
	{
		vsString s = vsFormatString("%s-Post (%s:%d)", m_string, m_file, m_line);
		ReportGLError(errcode, s.c_str());
	}
}


#endif // VS_GL_DEBUG


