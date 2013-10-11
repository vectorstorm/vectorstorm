/*
 *  VS_RendererBloom.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 4/01/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDERERBLOOM_H
#define VS_RENDERERBLOOM_H

#include "VS_RenderScheme.h"
#include "VS_RenderTarget.h"
//#define NO_SDL_GLEXT
#include "VS_OpenGL.h"

struct vsViewport
{
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
};

#define FILTER_COUNT (6)

class vsRenderSchemeBloom : public vsRenderScheme
{
	typedef vsRenderScheme Parent;

	GLuint			m_combineProg;
	GLuint			m_filterProg;
	GLuint			m_overlayProg;
	GLuint			m_hipassProg;
	GLint m_locSource, m_locCoefficients, m_locOffsetX, m_locOffsetY, m_combineScene;
	GLint m_passInt[FILTER_COUNT];

	bool			m_antialias;

	vsRenderTarget	*m_window;
	vsRenderTarget	*m_scene;
	vsRenderTarget	*m_pass[FILTER_COUNT];
	vsRenderTarget	*m_pass2[FILTER_COUNT];

	typedef enum {HORIZONTAL, VERTICAL} Direction;


	void			Blur(vsRenderTarget **sources, vsRenderTarget **dests, int count, Direction dir);
	void			ClearSurface();

protected:
	GLuint			Compile(const char *vert, const char *frag, int vertLength = 0, int fragLength = 0 );
	void			DestroyShader(GLuint shader);

public:
	vsRenderSchemeBloom(vsRenderer *renderer);
	virtual			~vsRenderSchemeBloom();

#if defined(OVERLAYS_IN_SHADER)
	virtual void	SetOverlay( const vsOverlay &o );
#endif // OVERLAYS_IN_SHADER

	virtual void	PreRender( const vsRenderer::Settings &s );
	virtual void	RenderDisplayList( vsDisplayList *list );
	virtual void	PostRender();

	virtual bool	SupportsShaders() { return true; }

	virtual bool	PreRenderTarget( const vsRenderer::Settings &s, vsRenderTarget *target );
	virtual bool	PostRenderTarget( vsRenderTarget *target );

	virtual vsImage*	Screenshot();
	virtual vsImage*	ScreenshotDepth();
	virtual vsImage*	ScreenshotAlpha();

	void			CheckFBO();

	static bool		Supported(bool experimental = false);

	friend class vsShader;
};


#endif // VS_RENDERERBLOOM_H
