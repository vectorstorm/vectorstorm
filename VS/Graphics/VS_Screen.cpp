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
#include "VS_Scene.h"
#include "VS_RendererSimple.h"
#include "VS_RendererPretty.h"
#include "VS_RendererBloom.h"
#include "VS_RendererShader.h"
#include "VS_System.h"
#include "VS_TextureManager.h"

#include "VS_TimerSystem.h"

const int c_fifoSize = 1024 * 500;		// 200k for our FIFO display list

vsScreen::vsScreen(int width, int height, int depth, bool fullscreen):
	m_scene(NULL),
	m_sceneCount(0),
	m_width(width),
	m_height(height),
	m_depth(depth),
	m_fullscreen(fullscreen),
	m_currentRenderTarget(NULL),
    m_currentSettings(NULL)
{
	vsRendererSimple *bootstrap = new vsRendererSimple;
	bootstrap->Init(width, height, depth, fullscreen);
	delete bootstrap;

	// MUST call vsRendererShader::Supported, or stuff breaks.  Don't just comment out that call!
#if TARGET_OS_IPHONE
	m_renderer = new vsRendererPretty();
#else
	if ( vsRendererShader::Supported() && vsSystem::Instance()->GetPreferences()->GetBloom() )
		m_renderer = new vsRendererShader();
	else if ( vsRendererBloom::Supported() && vsSystem::Instance()->GetPreferences()->GetBloom() )
		m_renderer = new vsRendererBloom();
	else
		m_renderer = new vsRendererPretty();
#endif
	m_renderer->InitPhaseTwo(width, height, depth, fullscreen);

	m_aspectRatio = ((float)m_width)/((float)m_height);
	printf("Screen Ratio:  %f\n", m_aspectRatio);

	m_fifo = new vsDisplayList(c_fifoSize);
	m_subfifo = new vsDisplayList(c_fifoSize);
}

vsScreen::~vsScreen()
{
	m_renderer->Deinit();

	delete m_fifo;
	delete m_subfifo;
	delete m_renderer;
}

void
vsScreen::UpdateVideoMode(int width, int height, int depth, bool fullscreen)
{
	if ( width == m_width && height == m_height && depth == m_depth && fullscreen == m_fullscreen )
		return;

	m_width = width;
	m_height = height;
	m_depth = depth;
	m_fullscreen = fullscreen;

	m_renderer->Deinit();
	delete m_renderer;

    vsTextureManager::Instance()->CollectGarbage(); // flush any unused client-side textures now, so they don't accidentally go away and go into the global heap.
#if TARGET_OS_IPHONE
	m_renderer = new vsRendererPretty();
#else
	if ( vsRendererShader::Supported() && vsSystem::Instance()->GetPreferences()->GetBloom() )
		m_renderer = new vsRendererShader();
	else if ( vsRendererBloom::Supported() && vsSystem::Instance()->GetPreferences()->GetBloom() )
		m_renderer = new vsRendererBloom();
	else
		m_renderer = new vsRendererPretty();
#endif
	m_renderer->Init(width, height, depth, fullscreen);
	m_renderer->InitPhaseTwo(width, height, depth, fullscreen);

	m_aspectRatio = ((float)m_width)/((float)m_height);
	printf("Screen Ratio:  %f\n", m_aspectRatio);

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


#if defined(DEBUG_SCENE)
	m_scene[m_sceneCount-1]->SetDebugCamera();
#endif // DEBUG_SCENE
}

void
vsScreen::DestroyScenes()
{
	if ( m_scene )
	{
		for ( int i = 0; i < m_sceneCount; i++ )
			delete m_scene[i];
		delete [] m_scene;

		m_scene = NULL;
	}
	m_sceneCount = 0;
}

void
vsScreen::Update( float timeStep )
{
	for ( int i = 0; i < m_sceneCount; i++ )
	{
		m_scene[i]->Update( timeStep );
	}
}

static size_t s_fifoHighWaterMark = c_fifoSize / 2;	// don't start warning us about how much display list we're using until we're at least half full
//static long s_fifoHighWaterMark = 0;	// don't start warning us about how much display list we're using until we're at least half full

void
vsScreen::Draw()
{
    DrawWithSettings( m_defaultRenderSettings );
}

void
vsScreen::DrawWithSettings( const vsRenderer::Settings &s )
{
    m_currentSettings = &s;
	if ( m_scene == NULL )
		return;

	m_renderer->PreRender(s);

	for ( int i = 0; i < m_sceneCount; i++ )
	{
		m_fifo->Clear();
		m_scene[i]->Draw( m_fifo );
		m_renderer->RenderDisplayList(m_fifo);

		if ( m_fifo->GetSize() > s_fifoHighWaterMark )
		{
			printf("New FIFO high water mark:  Layer %d, %0.2fk/%0.2fkk\n", i, m_fifo->GetSize()/1024.f, m_fifo->GetMaxSize()/1024.f);
			s_fifoHighWaterMark = m_fifo->GetSize();
		}
	}

	m_renderer->PostRender();
    m_currentSettings = NULL;
}

bool
vsScreen::DrawSceneToTarget( const vsRenderer::Settings &s, int scene, vsRenderTarget *target )
{
	return DrawSceneRangeToTarget( s, scene, scene, target );
}

bool
vsScreen::DrawSceneRangeToTarget( const vsRenderer::Settings &s, int firstScene, int lastScene, vsRenderTarget *target )
{
    m_currentSettings = &s;
	bool rendered = false;
	if ( m_scene )
	{
		m_subfifo->Clear();

		m_currentRenderTarget = target;
		if ( m_renderer->PreRenderTarget( s, target ) )
		{
			for ( int i = firstScene; i <= lastScene; i++ )
			{
				m_scene[i]->Draw( m_subfifo );
				m_renderer->RenderDisplayList( m_subfifo );
			}
			m_renderer->PostRenderTarget( target );
			rendered = true;
		}
		m_currentRenderTarget = NULL;
	}
    m_currentSettings = NULL;
	return rendered;
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
	return m_renderer->SupportsShaders();
}


void
vsScreen::RenderDisplayList( vsDisplayList *list )
{
	m_renderer->RawRenderDisplayList(list);
}

void
vsScreen::SetOpaqueRendering()
{
	m_renderer->SetDefaultRenderMode( RenderMode_Opaque );
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
