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
	bool m_enabled;
public:
	vsRenderPipelineStage();
	virtual ~vsRenderPipelineStage();

	void SetEnabled(bool enable) { m_enabled = enable; }
	bool IsEnabled() { return m_enabled; }

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGE_H

