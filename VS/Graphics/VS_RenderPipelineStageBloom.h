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
class vsRenderBuffer;

class vsRenderPipelineStageBloom: public vsRenderPipelineStage
{
	struct Pass
	{
		vsDynamicMaterial *m_horizontalBlurMaterial;
		vsDynamicMaterial *m_verticalBlurMaterial;
		vsDynamicMaterial *m_combinePassMaterial;
		vsRenderTarget *m_pass;
		vsRenderTarget *m_pass2;
	};
	struct Pass *m_passes;
	int m_passCount;
	vsDynamicMaterial *m_hipassMaterial;
	vsDynamicMaterial *m_fromMaterial;
	vsRenderTarget *m_from;
	vsRenderTarget *m_to;
	vsRenderBuffer *m_vertices;
	vsRenderBuffer *m_indices;
	vsShader *m_bloomBlurShader;
public:
	// 'dims' is how large our largest blur buffer should be.
	vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, int passes = 3 );
	virtual ~vsRenderPipelineStageBloom();

	virtual void PreparePipeline( vsRenderPipeline *pipeline );

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGEBLOOM_H

