/*
 *  VS_RenderPipelineStageScene.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RENDERPIPELINESTAGESCENE_H
#define VS_RENDERPIPELINESTAGESCENE_H

#include "VS_RenderPipelineStage.h"
#include "VS_Renderer.h"

class vsDisplayList;
class vsScene;
class vsRenderTarget;

class vsRenderPipelineStageScene: public vsRenderPipelineStage
{
	vsScene *m_scene;
	vsRenderTarget *m_target;
	vsRenderer::Settings m_settings;
public:
	vsRenderPipelineStageScene( vsScene *scene, vsRenderTarget *target, const vsRenderer::Settings& settings );

	virtual void Draw( vsDisplayList *list );
};

#endif // VS_RENDERPIPELINESTAGESCENE_H

