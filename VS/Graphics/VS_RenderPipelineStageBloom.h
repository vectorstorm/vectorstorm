/*
 *  VS_RenderPipelineStageBloom.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 24/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RENDERPIPELINESTAGEBLOOM_H
#define VS_RENDERPIPELINESTAGEBLOOM_H

#include "VS_RenderPipelineStage.h"

class vsDynamicMaterial;
class vsRenderTarget;
class vsShader;

#define BLOOM_PASSES (6)

class vsRenderPipelineStageBloom: public vsRenderPipelineStage
{
	vsShader *m_combine;
	vsShader *m_filter;
	vsShader *m_hipass;
	vsDynamicMaterial *m_hipassMaterial;
	vsDynamicMaterial *m_filterMaterial;
	vsDynamicMaterial *m_combineMaterial;
	vsRenderTarget *m_from;
	vsRenderTarget *m_to;
	vsRenderTarget *m_pass[BLOOM_PASSES];
public:
	vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, vsRenderTarget *pass[BLOOM_PASSES] );
	virtual ~vsRenderPipelineStageBloom();

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGEBLOOM_H

