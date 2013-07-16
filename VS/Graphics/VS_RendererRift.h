/*
 *  VS_RendererRift.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16-07-2013.
 *  Copyright 2013 Trevor Powell.  All rights reserved.
 *
 */
#ifndef VS_RENDERERRIFT_H
#define VS_RENDERERRIFT_H

#include "VS/Graphics/VS_RendererShader.h"

class vsRendererRift: public vsRendererShader
{
	GLuint			m_distortProg;
	GLint m_textureSampler, m_lensCenter, m_screenCenter, m_scale, m_scaleIn, m_hmdWarpParam;
public:
	vsRendererRift();
	virtual ~vsRendererRift();

	virtual void	PostBloom();
	virtual void	InitPhaseTwo(int width, int height, int depth, bool fullscreen);
	static bool		Supported(bool experimental = false);
};

#endif /* VS_RENDERERRIFT_H */

