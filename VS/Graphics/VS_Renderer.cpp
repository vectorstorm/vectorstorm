/*
 *  VS_Renderer.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Renderer.h"

vsRenderer*  vsRenderer::s_instance = nullptr;

vsRenderer::Settings::Settings():
	shaderSuite(nullptr),
    aspectRatio(1.f),
	polygonOffsetUnits(0.f),
    useCustomAspectRatio(false),
	// writeColor(true),
	// writeDepth(true),
	invertCull(false)
{
}

vsRenderer::vsRenderer(int width, int height, int depth, int flags):
	m_width(width),
	m_height(height),
	m_viewportWidth(width),
	m_viewportHeight(height),
	m_refreshRate(60)
{
	vsAssert(s_instance == nullptr, "Duplicate vsRenderer instance?");
	s_instance = this;
}

vsRenderer::~vsRenderer()
{
	s_instance = nullptr;
}

