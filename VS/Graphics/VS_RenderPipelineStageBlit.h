/*
 *  VS_RenderPipelineStageBlit.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 22/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RENDERPIPELINESTAGEBLIT_H
#define VS_RENDERPIPELINESTAGEBLIT_H

#include "VS_RenderPipelineStage.h"

class vsRenderTarget;

class vsRenderPipelineStageBlit: public vsRenderPipelineStage
{
	vsRenderTarget *m_from;
	vsRenderTarget *m_to;
public:
	vsRenderPipelineStageBlit( vsRenderTarget *from, vsRenderTarget *to );

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGEBLIT_H

