/*
 *  VS_Screen.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SCREEN_H
#define VS_SCREEN_H

#if defined(VS_TIMING_BARS) || defined(_DEBUG)
#define DEBUG_SCENE
#endif

#include "Graphics/VS_Renderer.h"
#include "Graphics/VS_DisplayList.h"
#include "Utils/VS_Singleton.h"

class vsDisplayList;
class vsRenderPipeline;
class vsScene;
class vsRenderTarget;
class vsImage;


class vsScreen
{
	static vsScreen *	s_instance;
	vsRenderer *		m_renderer;		// our renderer
	vsRenderPipeline *	m_pipeline;		// our default pipeline
	vsScene **			m_scene;		// our draw scenes
    vsRenderer::Settings    m_defaultRenderSettings;

	int					m_sceneCount;	// how many layers we have
	size_t				m_fifoUsageLastFrame;
	size_t				m_fifoHighWater;

	vsDisplayList *		m_fifo;			// our FIFO display list, for rendering

	int					m_width;
	int					m_height;
	int					m_bufferCount;
	int					m_depth;
	float				m_aspectRatio;
	vsRenderer::WindowType	m_windowType;
	bool				m_antialias;
	bool				m_vsync;

	bool				m_resized;

	vsRenderTarget *	m_currentRenderTarget;
    const vsRenderer::Settings *m_currentSettings;

	void BuildDefaultPipeline();

public:

	static vsScreen *	Instance() { return s_instance; }

	vsScreen(int width, int height, int depth, vsRenderer::WindowType type, int bufferCount, bool vsync, bool antialias, bool highDPI);
	~vsScreen();

	vsRenderTarget *	GetMainRenderTarget();
	vsRenderTarget *	GetPresentTarget();

	void			UpdateVideoMode(int width, int height, int depth, vsRenderer::WindowType windowType, int bufferCount, bool antialias, bool vsync);
	void			NotifyResized(int width, int height);
	void			CheckVideoMode();

	// The following width/height/aspect ratio functions operate in terms of
	// window manager POINTS, which may or may not equal pixels.  If desired,
	// you can get the actual number of pixels from the MainRenderTarget, above.
	//
	// 'true width' and 'true height' return the actual width and height of the
	// rendering area in its native orientation, whereas the 'width' and 'height'
	// accessors below will correct for screen orientation.  (So if the screen is
	// rotated 90 degrees, GetWidth() will return the 'width' in the rotated
	// context -- for example, GetWidth() == GetTrueHeight(), in that situation.)
	//
	// In general, you want to use GetWidth(), GetHeight(), and GetAspectRatio()
	// instead of these 'True' functions, unless you have some special effect in
	// mind which is going to avoid VectorStorm's built-in orientation
	// adjustments.  (This generally implies that you're avoiding using the
	// vsRenderQueue's matrix stack, which implicitly includes those
	// orientation adjustments)
	//
	int				GetTrueWidth() { return m_width; }
	int				GetTrueHeight() { return m_height; }
	float			GetTrueAspectRatio();

	int				GetWidth();
	int				GetHeight();
	float			GetAspectRatio();

	// returns true if we are using a renderer that supports shaders
	bool			SupportsShaders();

	// Ugh.  Need a nicer interface for this.
	bool			Resized() { return m_resized; }

	// Returns the maximum size of the fifo buffer containing our rendering
	// commands, in bytes.
	size_t			GetFifoSize() { return m_fifo->GetMaxSize(); }
	// Returns the number of bytes we used in the fifo buffer last frame.
	size_t			GetFifoUsage() { return m_fifoUsageLastFrame; }

	void			CreateScenes(int count);
	void			DestroyScenes();

	void			Update( float timeStep );
	void			Draw();
	void			DrawPipeline( vsRenderPipeline *pipeline );

	vsImage *       Screenshot();
	vsImage *       ScreenshotBack();
	vsImage *       ScreenshotDepth();
	vsImage *       ScreenshotAlpha();

	vsScene *		GetScene(int i);

#if defined(DEBUG_SCENE)
	vsScene *		GetDebugScene();
#endif // DEBUG_SCENE
};

#endif // VS_SCREEN_H
