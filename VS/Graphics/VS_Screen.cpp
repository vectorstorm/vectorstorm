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
#include "VS_Thread.h"
#include "VS_Profile.h"
#include "VS_Sleep.h"

#include "VS_TimerSystem.h"

const int c_fifoSize = 1024 * 4000;		// 4mb for our FIFO display list
vsScreen *	vsScreen::s_instance = nullptr;

vsScreen::vsScreen(int width, int height, int depth, vsRenderer::WindowType windowType, int bufferCount, bool vsync, bool antialias,bool highDPI):
	m_renderer(nullptr),
	m_pipeline(nullptr),
	m_scene(nullptr),
	m_sceneCount(0),
	m_fifoUsageLastFrame(0),
	m_fifoHighWater(0),
	m_width(width),
	m_height(height),
	m_bufferCount(bufferCount),
	m_depth(depth),
	m_windowType(windowType),
	m_antialias(antialias),
	m_vsync(vsync),
	m_resized(false),
	m_currentRenderTarget(nullptr),
	m_currentSettings(nullptr)
{
	vsAssert(s_instance == nullptr, "Tried to create a second vsScreen");
	s_instance = this;
	int flags = 0;
	if ( windowType != vsRenderer::WindowType_Window )
		flags |= vsRenderer::Flag_Fullscreen;
	if ( windowType == vsRenderer::WindowType_FullscreenWindow )
		flags |= vsRenderer::Flag_FullscreenWindow;
	if ( vsync )
		flags |= vsRenderer::Flag_VSync;
	if ( antialias )
		flags |= vsRenderer::Flag_Antialias;
	if ( highDPI )
		flags |= vsRenderer::Flag_HighDPI;
	flags |= vsRenderer::Flag_Resizable;

	vsLog("Width before:  %d", m_width);
	m_renderer = new vsRenderer_OpenGL3(m_width, m_height, m_depth, flags, bufferCount);

	m_width = m_renderer->GetWidth();
	m_height = m_renderer->GetHeight();
	vsLog("Width after:  %d", m_width);

	m_aspectRatio = ((float)m_width)/((float)m_height);
	vsLog("Screen Ratio:  %f", m_aspectRatio);

	m_fifo = new vsDisplayList(c_fifoSize);
	m_fifo->SetResizable();
}

vsScreen::~vsScreen()
{
	vsLog(" >> FIFO High water mark:  %d of %d (%0.2f%% usage)", m_fifoHighWater, c_fifoSize, 100.f * (float)m_fifoHighWater / c_fifoSize);
	vsDelete( m_renderer );
}

void
vsScreen::Deinit()
{
	DestroyScenes();
	vsDelete( m_fifo );
	s_instance = nullptr;
}

void
vsScreen::NotifyResized(int width, int height)
{
	m_width = width;
	m_height = height;
	m_renderer->NotifyResized(width, height);
	for ( int i = 0; i < m_sceneCount; i++ )
	{
		m_scene[i]->UpdateVideoMode();
	}

	// vsAssert(m_width == m_renderer->GetWidth() && m_height == m_renderer->GetHeight(), "Whaaa?");

	//delete m_renderer;

    vsTextureManager::Instance()->CollectGarbage(); // flush any unused client-side textures now, so they don't accidentally go away and go into the global heap.

	m_aspectRatio = ((float)m_width)/((float)m_height);
	vsLog("Screen aspect ratio:  %f", m_aspectRatio);
	m_resized = true;
	BuildDefaultPipeline();
}

void
vsScreen::UpdateVideoMode(int width, int height, int depth, vsRenderer::WindowType windowType, int bufferCount, bool antialias, bool vsync)
{
	if ( width == m_width &&
			height == m_height &&
			depth == m_depth &&
			windowType == m_windowType &&
			bufferCount == m_bufferCount &&
			antialias == m_antialias &&
			vsync == m_vsync )
		return;

	m_bufferCount = bufferCount;
	m_aspectRatio = ((float)m_width)/((float)m_height);
	m_depth = depth;
	m_windowType = windowType;
	m_antialias = antialias;
	m_vsync = vsync;
	m_renderer->UpdateVideoMode(width, height, depth, m_windowType, bufferCount, antialias, vsync);

	m_width = m_renderer->GetWidth();
	m_height = m_renderer->GetHeight();

	for ( int i = 0; i < m_sceneCount; i++ )
	{
		m_scene[i]->UpdateVideoMode();
	}

	//delete m_renderer;

    vsTextureManager::Instance()->CollectGarbage(); // flush any unused client-side textures now, so they don't accidentally go away and go into the global heap.

	m_aspectRatio = ((float)m_width)/((float)m_height);
	vsLog("Screen aspect ratio:  %f", m_aspectRatio);
	m_resized = true;
	BuildDefaultPipeline();
}

void
vsScreen::CheckVideoMode()
{
	// note that we might move and resize at the same time.  We don't ever
	// want to set 'm_resized' to false, except in 'Update'!
	m_resized |= m_renderer->CheckVideoMode();

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
		m_scene[i] = new vsScene( vsFormatString("Default Scene %d", i) );
	m_sceneCount = count;

	BuildDefaultPipeline();

#if defined(DEBUG_SCENE)
	m_scene[m_sceneCount-1]->SetDebugCamera();
#endif // DEBUG_SCENE
}

void
vsScreen::BuildDefaultPipeline()
{
	vsDelete( m_pipeline );
	m_pipeline = new vsRenderPipeline(3);
	m_pipeline->SetStage(0, new vsRenderPipelineStageScenes( m_scene, m_sceneCount, m_renderer->GetMainRenderTarget(), m_defaultRenderSettings, true ));
#if defined(DEBUG_SCENE)
	m_pipeline->SetStage(1, new vsRenderPipelineStageScenes( GetDebugScene(), m_renderer->GetMainRenderTarget(), m_defaultRenderSettings, true ));
#endif // DEBUG_SCENE
	m_pipeline->SetStage(2, new vsRenderPipelineStageBlit( m_renderer->GetMainRenderTarget(), m_renderer->GetPresentTarget() ));
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

		m_scene = nullptr;
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

namespace
{
	vsMutex s_pipelineDrawMutex;

	struct QueuedDraw
	{
		vsRenderPipeline *pipeline;
		vsShaderOptions *options;
	};
	vsArray<QueuedDraw*> s_draws;
	vsArray<QueuedDraw*> s_finishedDraws;
}

void
vsScreen::DrawPipeline_ThreadSafe( vsRenderPipeline *pipeline, vsShaderOptions *customOptions )
{
	if ( vsThread::IsMainThread() )
	{
		// easy case, we're already on the main thread so just call DrawPipeline!
		return DrawPipeline(pipeline, customOptions);
	}

	// Now, we need to lock a mutex to touch the queued draws list.
	QueuedDraw qd;
	qd.pipeline = pipeline;
	qd.options = customOptions;

	s_pipelineDrawMutex.Lock();
	s_draws.AddItem( &qd );
	s_pipelineDrawMutex.Unlock();

	bool done = false;
	do
	{
		vsSleep(0);

		s_pipelineDrawMutex.Lock();
		if ( s_finishedDraws.Contains(&qd) )
		{
			s_finishedDraws.RemoveItem(&qd);
			done = true;
		}
		s_pipelineDrawMutex.Unlock();
	}
	while( !done );

	return; // and now we're done!
}

void
vsScreen::DrawPipeline( vsRenderPipeline *pipeline, vsShaderOptions *customOptions )
{
	{
		s_pipelineDrawMutex.Lock();

		while ( !s_draws.IsEmpty() )
		{
			QueuedDraw *qd = s_draws[0];
			s_draws.RemoveItem(qd);

			_DrawPipeline( qd->pipeline, qd->options );
			s_finishedDraws.AddItem(qd);
		}

		s_pipelineDrawMutex.Unlock();
	}

	_DrawPipeline( pipeline, customOptions );
}

void
vsScreen::_DrawPipeline( vsRenderPipeline *pipeline, vsShaderOptions *customOptions )
{
	vsTimerSystem::Instance()->BeginDraw();

	PROFILE_GL("DrawPipeline");
	m_currentSettings = &m_defaultRenderSettings;

	m_fifo->Clear();
	{
		PROFILE("GatherRenderables");
		if ( customOptions )
			m_fifo->PushShaderOptions(*customOptions);
		pipeline->Draw(m_fifo);
		if ( customOptions )
			m_fifo->PopShaderOptions();
	}
	m_fifoUsageLastFrame = m_fifo->GetSize();
	if ( m_fifoUsageLastFrame > m_fifoHighWater )
	{
		m_fifoHighWater = m_fifoUsageLastFrame;
// #define TRACE_FIFO_SIZE
#ifdef TRACE_FIFO_SIZE
		vsLog(" >> New FIFO High water mark:  %d of %d (%0.2f%% usage)", m_fifoHighWater, c_fifoSize, 100.f * (float)m_fifoHighWater / c_fifoSize);
#endif
	}
// #ifdef DEBUG_SCENE
// 		m_scene[m_sceneCount-1]->Draw(m_fifo);
// #endif
	{
		PROFILE_GL("ClearState");
		m_renderer->ClearState();
	}
	m_renderer->RenderDisplayList(m_fifo);

	pipeline->PostDraw();

	m_currentSettings = nullptr;
	vsTimerSystem::Instance()->EndDraw();
}

void
vsScreen::Present()
{
	vsTimerSystem::Instance()->BeginPresent();
	m_renderer->Present();
	vsTimerSystem::Instance()->EndPresent();
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
	return nullptr;
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
vsScreen::Screenshot_Async()
{
	return m_renderer->Screenshot_Async();
}

vsImage *
vsScreen::ScreenshotBack()
{
	return m_renderer->ScreenshotBack();
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

