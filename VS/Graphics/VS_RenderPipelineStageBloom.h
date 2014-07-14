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

#define BLOOM_PASSES (7)

class vsRenderPipelineStageBloom: public vsRenderPipelineStage
{
	int m_dims;
	vsDynamicMaterial *m_hipassMaterial;
	vsDynamicMaterial *m_horizontalBlurMaterial[BLOOM_PASSES];
	vsDynamicMaterial *m_verticalBlurMaterial[BLOOM_PASSES];
	vsDynamicMaterial *m_combinePassMaterial[BLOOM_PASSES];
	vsDynamicMaterial *m_fromMaterial;
	vsRenderTarget *m_from;
	vsRenderTarget *m_to;
	vsRenderTarget *m_pass[BLOOM_PASSES];
	vsRenderTarget *m_pass2[BLOOM_PASSES];
public:
	// 'dims' is how large our largest blur buffer should be.
	vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, int dims = 1024 );
	virtual ~vsRenderPipelineStageBloom();

	virtual void PreparePipeline( vsRenderPipeline *pipeline );

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGEBLOOM_H

