/*
 *  VS_RenderPipeline.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RENDERPIPELINE_H
#define VS_RENDERPIPELINE_H

class vsDisplayList;
class vsRenderPipelineStage;

class vsRenderPipeline
{
	vsRenderPipelineStage ** m_stage;
	int m_stageCount;

public:
	vsRenderPipeline( int maxStageCount );
	~vsRenderPipeline();

	void SetStage( int stageId, vsRenderPipelineStage *stage );
	void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINE_H

