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

class vsDisplayList;
class vsCamera2D;
class vsShader;
class vsShaderSuite;

enum RenderMode
{
	RenderMode_Opaque,
	RenderMode_Additive
};

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

protected:
	Settings			m_currentSettings;
	RenderMode			m_defaultRenderMode;

	int					m_width;
	int					m_height;

	int					m_viewportWidth;
	int					m_viewportHeight;
public:

	vsRenderer() {m_defaultRenderMode = RenderMode_Additive;}
	virtual ~vsRenderer() {Deinit();}

	virtual void		Init(int width, int height, int depth, bool fullscreen) {UNUSED(width); UNUSED(height); UNUSED(depth); UNUSED(fullscreen);}
	virtual void		InitPhaseTwo(int width, int height, int depth, bool fullscreen) {m_width = width; m_height = height; UNUSED(depth); UNUSED(fullscreen);}
	virtual void		Deinit() {}

	virtual bool		SupportsShaders() { return false; }

	virtual void		PreRender( const Settings &s ) { m_currentSettings = s; }
	virtual void		RenderDisplayList( vsDisplayList *list ) = 0;
	virtual void		RawRenderDisplayList( vsDisplayList *list ) = 0;
	virtual void		PostRender() {}

	virtual bool		PreRenderTarget( const Settings &s, vsRenderTarget *target ) { return false; }
	virtual bool		PostRenderTarget( vsRenderTarget *target ) { return false; }

	virtual vsImage*	Screenshot() = 0;
	virtual vsImage*	ScreenshotDepth() = 0;
	virtual vsImage*	ScreenshotAlpha() = 0;
	void				SetDefaultRenderMode( RenderMode mode ) { m_defaultRenderMode = mode; }
};

#endif // VS_RENDERER_H
