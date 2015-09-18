/*
 *  VS_RenderPipelineStageScenes.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RenderPipelineStageScenes.h"
#include "VS_Renderer.h"
#include "VS_Scene.h"

vsRenderPipelineStageScenes::vsRenderPipelineStageScenes( vsScene *scene, vsRenderTarget *target, const vsRenderer::Settings& settings, bool clear ):
	m_scene(new vsScene*[1]),
	m_sceneCount(1),
	m_target(target),
	m_settings(settings),
	m_clear(clear)
{
	m_scene[0] = scene;
}

vsRenderPipelineStageScenes::vsRenderPipelineStageScenes( vsScene **scenes, int sceneCount, vsRenderTarget *target, const vsRenderer::Settings& settings, bool clear ):
	m_scene(new vsScene*[sceneCount]),
	m_sceneCount(sceneCount),
	m_target(target),
	m_settings(settings),
	m_clear(clear)
{
	for ( int i = 0; i < sceneCount; i++ )
	{
		m_scene[i] = scenes[i];
	}
}

vsRenderPipelineStageScenes::~vsRenderPipelineStageScenes()
{
	vsDeleteArray(m_scene);
}

void
vsRenderPipelineStageScenes::Draw( vsDisplayList *list )
{
	list->SetRenderTarget( m_target );
	if ( m_clear )
		list->ClearRenderTarget();
	for ( int i = 0; i < m_sceneCount; i++ )
	{
		if ( m_scene[i] )
			m_scene[i]->Draw( list, m_settings );
	}
	if ( m_target )
		list->ResolveRenderTarget( m_target );
}

