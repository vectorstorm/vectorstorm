/*
 *  VS_Renderer.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDERER_H
#define VS_RENDERER_H

#include "VS/Graphics/VS_Texture.h"

#include "VS_Color.h"
#include "VS_Fog.h"
#include "VS_Material.h"
#include "VS_RendererState.h"
#include "VS_Texture.h"
#include "Math/VS_Transform.h"

class vsCamera2D;
class vsDisplayList;
class vsMaterialInternal;
class vsOverlay;
class vsRenderBuffer;
class vsShader;
class vsShaderSuite;
class vsTransform2D;
class vsVector2D;
struct SDL_Surface;

class vsRenderer
{
public:

	struct Settings
	{
		vsShaderSuite *shaderSuite;	// must be cleaned up by someone else;  this settings object does NOT own shaders!
        float aspectRatio;
		float polygonOffsetUnits;
        bool useCustomAspectRatio;
		bool writeColor;
		bool writeDepth;
		bool invertCull;

		Settings();
	};

	static vsRenderer*  s_instance;
	Settings			m_currentSettings;

	// m_width, m_viewportWidth, m_height, and m_viewportHeight are the
	// sizes of our rendering area in WINDOW MANAGER COORDINATES, not necessarily
	// the sizes in pixels.
	int					m_width;
	int					m_height;
	int					m_viewportWidth;
	int					m_viewportHeight;

	// m_widthPixels, m_heightPixels, etc. are the sizes of our rendering area in
	// PIXELS.  Which may be different than the numbers above on high-dpi displays.
	int					m_widthPixels;
	int					m_heightPixels;
	int					m_viewportWidthPixels;
	int					m_viewportHeightPixels;

	// How fast can our display update its image?  Expressed as a refresh rate,
	// minimum of 1fps.
	int					m_refreshRate;

public:

	static vsRenderer* Instance() { return s_instance; }
	enum
	{
		Flag_Fullscreen = BIT(0),
		Flag_FullscreenWindow = BIT(1),
		Flag_VSync = BIT(2),
		Flag_Resizable = BIT(3),
		Flag_Antialias = BIT(4),
		Flag_HighDPI = BIT(5)
	};

	enum WindowType
	{
		WindowType_Window,
		WindowType_Fullscreen,
		WindowType_FullscreenWindow,
	};
	vsRenderer(int width, int height, int depth, int flags);
	virtual ~vsRenderer();

	virtual bool	CheckVideoMode() = 0;
	virtual void	UpdateVideoMode(int width, int height, int depth, WindowType type, int bufferCount, bool antialias, bool vsync) = 0;
	virtual void	NotifyResized(int width, int height) = 0;

	virtual void	ClearState() = 0; // clear internal state
	virtual void	RenderDisplayList( vsDisplayList *list ) = 0;
	virtual void	Present() = 0; // we're finished with this frame.

	// virtual bool	PreRenderTarget( const vsRenderer::Settings &s, vsRenderTarget *target ) = 0;
	// virtual bool	PostRenderTarget( vsRenderTarget *target ) = 0;

	int		GetWidth() const { return m_width; }
	int		GetHeight() const { return m_height; }
	int		GetWidthPixels() const { return m_widthPixels; }
	int		GetHeightPixels() const { return m_heightPixels; }
	void	SetViewportWidthPixels( int width ) { m_widthPixels = width; }
	void	SetViewportHeightPixels( int height ) { m_heightPixels = height; }

	int		GetRefreshRate() const { return m_refreshRate; }

	virtual vsRenderTarget *GetMainRenderTarget() = 0;
	virtual vsRenderTarget *GetPresentTarget() = 0;

	const Settings& GetCurrentSettings() const { return m_currentSettings; }

	virtual vsImage*	Screenshot() = 0;
	virtual vsImage*	Screenshot_Async() = 0;
	virtual vsImage*	ScreenshotBack() = 0;
	virtual vsImage*	ScreenshotDepth() = 0;
	virtual vsImage*	ScreenshotAlpha() = 0;
};

#endif // VS_RENDERER_H
