/*
 *  VS_RenderPipelineStage.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RENDERPIPELINESTAGE_H
#define VS_RENDERPIPELINESTAGE_H

class vsDisplayList;

class vsRenderPipelineStage
{
public:
	vsRenderPipelineStage();
	virtual ~vsRenderPipelineStage();

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGE_H

