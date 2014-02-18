/*
 *  VS_RenderPipelineStageScene.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RenderPipelineStageScene.h"
#include "VS_Renderer.h"
#include "VS_Scene.h"


vsRenderPipelineStageScene::vsRenderPipelineStageScene( vsScene *scene, vsRenderTarget *target, const vsRenderer::Settings& settings ):
	m_scene(scene),
	m_target(target),
	m_settings(settings)
{
}

void
vsRenderPipelineStageScene::Draw( vsDisplayList *list )
{
	if ( m_scene && m_scene->IsEnabled() )
	{
		list->SetRenderTarget( m_target );
		m_scene->Draw( list );
	}
}

