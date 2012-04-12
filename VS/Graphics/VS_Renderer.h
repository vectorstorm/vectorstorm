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

enum RenderMode
{
	RenderMode_Opaque,
	RenderMode_Additive
};

class vsRenderer
{
protected:
	RenderMode			m_defaultRenderMode;
	
	int					m_width;
	int					m_height;
public:
	vsRenderer() {m_defaultRenderMode = RenderMode_Additive;}
	virtual ~vsRenderer() {Deinit();}
	
	virtual void		Init(int width, int height, int depth, bool fullscreen) {UNUSED(width); UNUSED(height); UNUSED(depth); UNUSED(fullscreen);}
	virtual void		InitPhaseTwo(int width, int height, int depth, bool fullscreen) {m_width = width; m_height = height; UNUSED(depth); UNUSED(fullscreen);}
	virtual void		Deinit() {}
	
	virtual void		PreRender() {}
	virtual void		RenderDisplayList( vsDisplayList *list ) = 0;
	virtual void		RawRenderDisplayList( vsDisplayList *list ) = 0;
	virtual void		PostRender() {}
	
	virtual vsImage*	Screenshot() = 0;
	virtual vsImage*	ScreenshotDepth() = 0;
	virtual vsImage*	ScreenshotAlpha() = 0;
	void				SetDefaultRenderMode( RenderMode mode ) { m_defaultRenderMode = mode; }
};

#endif // VS_RENDERER_H
