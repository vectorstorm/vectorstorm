/*
 *  VS_Screen.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Screen.h"
#include "VS_DisplayList.h"
#include "VS_RenderPipeline.h"
#include "VS_RenderPipelineStage.h"
#include "VS_RenderPipelineStageBlit.h"
#include "VS_RenderPipelineStageScenes.h"
#include "VS_Renderer_OpenGL3.h"
#include "VS_RenderTarget.h"
#include "VS_Scene.h"
#include "VS_System.h"
#include "VS_TextureManager.h"

#include "VS_TimerSystem.h"

const int c_fifoSize = 1024 * 1024;		// 1 megabyte for our FIFO display list
vsScreen *	vsScreen::s_instance = NULL;

vsScreen::vsScreen(int width, int height, int depth, bool fullscreen, bool vsync):
	m_renderer(NULL),
	m_pipeline(NULL),
	m_scene(NULL),
	m_sceneCount(0),
	m_width(width),
	m_height(height),
	m_depth(depth),
	m_fullscreen(fullscreen),
	m_resized(false),
	m_currentRenderTarget(NULL),
	m_currentSettings(NULL)
{
	vsAssert(s_instance == NULL, "Tried to create a second vsScreen");
	s_instance = this;
	int flags = 0;
	if ( fullscreen )
		flags |= vsRenderer::Flag_Fullscreen;
	if ( vsync )
		flags |= vsRenderer::Flag_VSync;
	flags |= vsRenderer::Flag_Resizable;

	m_renderer = new vsRenderer_OpenGL3(width, height, depth, flags);

	m_aspectRatio = ((float)m_width)/((float)m_height);
	printf("Screen Ratio:  %f\n", m_aspectRatio);

	m_fifo = new vsDisplayList(c_fifoSize);
	m_subfifo = new vsDisplayList(c_fifoSize);
}

vsScreen::~vsScreen()
{
	DestroyScenes();
	vsDelete( m_renderer );
	vsDelete( m_fifo );
	vsDelete( m_subfifo );
	s_instance = NULL;
}

void
vsScreen::UpdateVideoMode(int width, int height, int depth, bool fullscreen)
{
	if ( width == m_width && height == m_height && depth == m_depth && fullscreen == m_fullscreen )
		return;

	m_width = width;
	m_height = height;
	m_aspectRatio = ((float)m_width)/((float)m_height);
	m_depth = depth;
	m_fullscreen = fullscreen;
	m_renderer->UpdateVideoMode(width, height, depth, fullscreen);
	for ( int i = 0; i < m_sceneCount; i++ )
	{
		m_scene[i]->UpdateVideoMode();
	}

	//delete m_renderer;

    vsTextureManager::Instance()->CollectGarbage(); // flush any unused client-side textures now, so they don't accidentally go away and go into the global heap.

	m_aspectRatio = ((float)m_width)/((float)m_height);
	printf("Screen Ratio:  %f\n", m_aspectRatio);
	m_resized = true;
}

void
vsScreen::CheckVideoMode()
{
	m_resized = m_renderer->CheckVideoMode();

	if ( m_resized )
	{
		// okay.  If we resized here, it means that we underwent an underlying
		// resolution change.  So we have the same number of window manager
		// points, but a different number of pixels, now.  Since fonts care about
		// that sort of thing, let's let them know.
	}
}

void
vsScreen::CreateScenes(int count)
{
	DestroyScenes();

#if defined(DEBUG_SCENE)
	count++;	// extra layer for our debug data
#endif // DEBUG_SCENE

	m_scene = new vsScene *[count];
	for ( int i = 0; i < count; i++ )
		m_scene[i] = new vsScene;
	m_sceneCount = count;

	m_pipeline = new vsRenderPipeline(2);
	m_pipeline->SetStage(0, new vsRenderPipelineStageScenes( m_scene, m_sceneCount, m_renderer->GetMainRenderTarget(), m_defaultRenderSettings ));
	m_pipeline->SetStage(1, new vsRenderPipelineStageBlit( m_renderer->GetMainRenderTarget(), m_renderer->GetPresentTarget() ));

#if defined(DEBUG_SCENE)
	m_scene[m_sceneCount-1]->SetDebugCamera();
#endif // DEBUG_SCENE
}

vsRenderTarget *
vsScreen::GetMainRenderTarget()
{
	return m_renderer->GetMainRenderTarget();
}

vsRenderTarget *
vsScreen::GetPresentTarget()
{
	return m_renderer->GetPresentTarget();
}

void
vsScreen::DestroyScenes()
{
	if ( m_pipeline )
		vsDelete( m_pipeline );
	if ( m_scene )
	{
		for ( int i = 0; i < m_sceneCount; i++ )
			vsDelete( m_scene[i] );
		vsDeleteArray( m_scene );

		m_scene = NULL;
	}
	m_sceneCount = 0;
}

void
vsScreen::Update( float timeStep )
{
	m_resized = false;
	for ( int i = 0; i < m_sceneCount; i++ )
	{
		m_scene[i]->Update( timeStep );
	}
}

// static size_t s_fifoHighWaterMark = c_fifoSize / 2;	// don't start warning us about how much display list we're using until we're at least half full
//static long s_fifoHighWaterMark = 0;	// don't start warning us about how much display list we're using until we're at least half full

void
vsScreen::Draw()
{
	DrawPipeline(m_pipeline);
}

void
vsScreen::DrawPipeline( vsRenderPipeline *pipeline )
{
	m_currentSettings = &m_defaultRenderSettings;

	m_renderer->PreRender(m_defaultRenderSettings);
	m_fifo->Clear();
	pipeline->Draw(m_fifo);
#ifdef DEBUG_SCENE
	m_renderer->RenderDisplayList(m_fifo);
	m_fifo->Clear();
	m_scene[m_sceneCount-1]->Draw(m_fifo);
#endif
	m_renderer->RenderDisplayList(m_fifo);
	m_renderer->PostRender();

	m_currentSettings = NULL;
}

vsScene *
vsScreen::GetScene(int i)
{
	vsAssert( i < m_sceneCount, "Requested non-allocated vsLayer!" );

#if defined(DEBUG_SCENE)
	vsAssert( i < m_sceneCount-1, "Requested debug scene, which won't be there in release builds!" );
#endif // DEBUG_SCENE

	if ( m_scene )
	{
		return m_scene[i];
	}
	return NULL;
}

float
vsScreen::GetTrueAspectRatio()
{
	if ( m_currentRenderTarget )
	{
		if ( m_currentSettings && m_currentSettings->useCustomAspectRatio )
		{
			return m_currentSettings->aspectRatio;
		}
		else
		{
			return m_currentRenderTarget->GetWidth() / float(m_currentRenderTarget->GetHeight());
		}
	}

	return m_aspectRatio;
}

int
vsScreen::GetWidth()
{
	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Three:
		case Orientation_Nine:
			return GetTrueHeight();
			break;
		default:
			break;
	}
	return GetTrueWidth();
}

int
vsScreen::GetHeight()
{
	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Three:
		case Orientation_Nine:
			return GetTrueWidth();
			break;
		default:
			break;
	}
	return GetTrueHeight();
}

float
vsScreen::GetAspectRatio()
{
	switch( vsSystem::Instance()->GetOrientation() )
	{
		case Orientation_Three:
		case Orientation_Nine:
			return 1.f / GetTrueAspectRatio();
			break;
		default:
			break;
	}
	return GetTrueAspectRatio();
}

bool
vsScreen::SupportsShaders()
{
	return true;
}

vsImage *
vsScreen::Screenshot()
{
	return m_renderer->Screenshot();
}

vsImage *
vsScreen::ScreenshotDepth()
{
	return m_renderer->ScreenshotDepth();
}

vsImage *
vsScreen::ScreenshotAlpha()
{
	return m_renderer->ScreenshotAlpha();
}

#if defined(DEBUG_SCENE)

vsScene *
vsScreen::GetDebugScene()
{
	return m_scene[ m_sceneCount-1 ];
}

#endif // DEBUG_SCENE
