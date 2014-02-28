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
class vsMaterial;
class vsRenderTarget;
class vsShader;

#define BLOOM_PASSES (6)

class vsRenderPipelineStageBloom: public vsRenderPipelineStage
{
	vsDynamicMaterial *m_hipassMaterial;
	vsDynamicMaterial *m_horizontalBlurMaterial[BLOOM_PASSES];
	vsDynamicMaterial *m_verticalBlurMaterial[BLOOM_PASSES];
	vsDynamicMaterial *m_combineMaterial;
	vsDynamicMaterial *m_straight;
	vsMaterial *m_white;
	vsRenderTarget *m_from;
	vsRenderTarget *m_to;
	vsRenderTarget *m_pass[BLOOM_PASSES];
	vsRenderTarget *m_pass2[BLOOM_PASSES];
public:
	vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, vsRenderTarget *pass[BLOOM_PASSES], vsRenderTarget *pass2[BLOOM_PASSES] );
	virtual ~vsRenderPipelineStageBloom();

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGEBLOOM_H

