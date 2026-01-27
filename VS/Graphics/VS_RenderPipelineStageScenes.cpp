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
#include "VS_RenderTarget.h"
#include "VS_Scene.h"

vsRenderPipelineStageScenes::vsRenderPipelineStageScenes( vsScene *scene, vsRenderTarget *target, bool clear, vsCamera3D *customCamera ):
	m_scene(new vsScene*[1]),
	m_sceneCount(1),
	m_target(target),
	m_customCamera(customCamera),
	m_clearColor(0,0,0,0),
	m_clear(clear)
{
	m_scene[0] = scene;
}

vsRenderPipelineStageScenes::vsRenderPipelineStageScenes( vsScene **scenes, int sceneCount, vsRenderTarget *target, bool clear, vsCamera3D *customCamera ):
	m_scene(new vsScene*[sceneCount]),
	m_sceneCount(sceneCount),
	m_target(target),
	m_customCamera(customCamera),
	m_clearColor(0,0,0,0),
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
	if ( m_target )
		m_target->CreateDeferred();

	list->SetRenderTarget( m_target );
	if ( m_clear )
		list->ClearRenderTargetColor(m_clearColor);
	for ( int i = 0; i < m_sceneCount; i++ )
	{
		if ( m_scene[i] )
		{
			vsCamera3D *cam = nullptr;
			bool reference;
			if ( m_customCamera && m_scene[i]->Is3D() )
			{
				cam = m_scene[i]->GetCamera3D();
				reference = m_scene[i]->CameraIsReference();
				m_scene[i]->SetCamera3D(m_customCamera);
				m_scene[i]->Draw( list, m_target );
				m_scene[i]->SetCamera3D(cam, reference);
			}
			else
			{
				m_scene[i]->Draw( list, m_target );
			}
		}
	}
	list->ResolveRenderTarget( m_target );
}

