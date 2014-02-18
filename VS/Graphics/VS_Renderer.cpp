/*
 *  VS_Renderer.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Renderer.h"

#include "VS_Camera.h"
#include "VS_Debug.h"
#include "VS_DisplayList.h"
#include "VS_Image.h"
#include "VS_MaterialInternal.h"
#include "VS_Matrix.h"
#include "VS_RenderBuffer.h"
#include "VS_Screen.h"
#include "VS_Shader.h"
#include "VS_System.h"
#include "VS_Texture.h"
#include "VS_TextureInternal.h"

#include "VS_RenderSchemeBloom.h"
#include "VS_RenderSchemeShader.h"
#include "VS_RenderSchemeFixedFunction.h"

#include "VS_OpenGL.h"

#include "VS_TimerSystem.h"

vsRenderer*  vsRenderer::s_instance = NULL;

vsRenderer::Settings::Settings():
	shaderSuite(NULL),
    aspectRatio(1.f),
	polygonOffsetUnits(0.f),
    useCustomAspectRatio(false),
	writeColor(true),
	writeDepth(true),
	invertCull(false)
{
	vsAssert(s_instance == NULL, "Duplicate vsRenderer instance?");
}


#if TARGET_OS_IPHONE

#define glOrtho( a, b, c, d, e, f ) glOrthof( a, b, c, d, e, f )
#define glFrustum( a, b, c, d, e, f ) glFrustumf( a, b, c, d, e, f )
#define glClearDepth( a ) glClearDepthf( a )
#define glFogi( a, b ) glFogx( a, b )
#define glTexParameteri( a, b, c ) glTexParameterx( a, b, c )

#endif

vsRenderer::vsRenderer(int width, int height, int depth, int flags):
	m_width(width),
	m_height(height),
	m_viewportWidth(width),
	m_viewportHeight(height)
{
	vsAssert(s_instance == NULL, "Duplicate vsRenderer instance?");
}

vsRenderer::~vsRenderer()
{
	s_instance = NULL;
}

