/*
 *  VS_Renderer.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDERER_OPENGL3_H
#define VS_RENDERER_OPENGL3_H

#include "VS/Graphics/VS_Texture.h"

#include "VS_Renderer.h"
#include "VS_Color.h"
#include "VS_Fog.h"
#include "VS_Material.h"
#include "VS_RendererState.h"
#include "VS_ShaderSuite.h"
#include "VS_ShaderOptions.h"
#include "VS_Texture.h"
#include "Math/VS_Transform.h"
#include "VS_OpenGL.h"
#include "VS_VertexArrayObject.h"

class vsCamera2D;
class vsDisplayList;
class vsMaterialInternal;
class vsOverlay;
class vsRenderBuffer;
class vsShaderValues;
class vsTransform2D;
class vsVector2D;
struct SDL_Surface;

#define MAX_STACK_LEVEL (30)
#define CHECK_GL_ERRORS

class vsRenderer_OpenGL3: public vsRenderer
{
	int					m_flags;
	vsShaderSuite		*m_defaultShaderSuite;

	vsVector3D           m_currentCameraPosition;
	Settings             m_currentSettings;

	vsMatrix4x4			*m_currentLocalToWorld;
	vsRenderBuffer		*m_currentLocalToWorldBuffer;
	int					m_currentLocalToWorldCount;
	vsMatrix4x4			m_currentWorldToView;
	vsMatrix4x4			m_currentViewToProjection;

	vsColor				m_currentColor;
	vsColor *			m_currentColors;
	vsRenderBuffer *	m_currentColorsBuffer;
	vsColor				m_currentFogColor;
	float				m_currentFogDensity;

	vsRenderTarget *     m_window;
	vsRenderTarget *     m_scene;
	vsRenderTarget *     m_currentRenderTarget;

    vsRendererState      m_state;
	vsVertexArrayObject *m_currentVAO;
	vsVertexArrayObject *m_nextVAO;

	vsVertexArrayObject m_defaultVAO;

	vsMaterial *         m_currentMaterial;
	vsMaterialInternal * m_currentMaterialInternal;
	vsShader *           m_currentShader;
	vsShaderValues *     m_currentShaderValues;

	vsVector3D *         m_currentVertexArray;
	vsVector3D *         m_currentNormalArray;
	vsVector2D *         m_currentTexelArray;
	vsColor *            m_currentColorArray;

	vsRenderBuffer *     m_currentVertexBuffer;
	vsRenderBuffer *     m_currentNormalBuffer;
	vsRenderBuffer *     m_currentTexelBuffer;
	vsRenderBuffer *     m_currentColorBuffer;

	vsArray<vsShaderOptions> m_optionsStack;

	vsBox2D m_currentViewportPixels;

	size_t m_lastShaderId;

#define MAX_LIGHTS (4)
	struct lightStatus
	{
		int type;
		vsVector3D position;
		vsColor ambient;
		vsColor diffuse;
		vsColor specular;
	};

	vsMatrix4x4          m_transformStack[MAX_STACK_LEVEL];
	int                  m_currentTransformStackLevel;
	int                  m_currentVertexArrayCount;
	int                  m_currentNormalArrayCount;
	int                  m_currentTexelArrayCount;
	int                  m_currentColorArrayCount;
	int                  m_lightCount;
	int                  m_bufferCount;
	lightStatus          m_lightStatus[MAX_LIGHTS];
	bool                 m_usingNormalArray;
	bool                 m_usingTexelArray;
	bool                 m_antialias;
	bool                 m_vsync;
	uint32_t			m_vao;	// temporary -- for our global VAO.
	// VAOs should really be integrated more nicely somewhere, but for now,
	// we'll treat our rendering like OpenGL2 and just continually reconfigure
	// a single global Vertex Array Object..

	WindowType m_windowType;

	void				FlushRenderState();
	virtual void		SetMaterialInternal(vsMaterialInternal *material);
	virtual void		SetMaterial(vsMaterial *material);
	//virtual void		SetDrawMode(vsDrawMode mode);

	void ResizeRenderTargetsToMatchWindow();

	void DetermineRefreshRate();
	void SetRenderTarget( vsRenderTarget *target );

public:

	vsRenderer_OpenGL3(int width, int height, int depth, int flags, int bufferCount);
	virtual ~vsRenderer_OpenGL3();
	void Deinit() override; // we're about to shut down

	static vsRenderer_OpenGL3* Instance() { return static_cast<vsRenderer_OpenGL3*>(vsRenderer::Instance()); }

	bool	CheckVideoMode();
	virtual void UpdateVideoMode(int width, int height, int depth, WindowType type, int bufferCount, bool antialias, bool vsync);
	virtual void NotifyResized(int width, int height);

	virtual void	ClearState();
	virtual void	RenderDisplayList( vsDisplayList *list );
	virtual void	Present();

	virtual vsRenderTarget *GetMainRenderTarget() { return m_scene; }
	virtual vsRenderTarget *GetPresentTarget() { return m_window; }

	// sets an OpenGL context on the calling thread, for the purposes
	// of loading data in the background.
	void	SetLoadingContext();
	void	ClearLoadingContext();

	bool	IsLoadingContext(); // returns true if we're on the loading context.

	// If we have the loading context on the current thread, we can
	// call "FenceLoadingContext" to block until the current context's
	// contents have all become available from the main thread.
	void	FenceLoadingContext();

	int		GetWidthPixels() const { return m_widthPixels; }
	int		GetHeightPixels() const { return m_heightPixels; }
	void	SetViewportWidthPixels( int width ) { m_widthPixels = width; }
	void	SetViewportHeightPixels( int height ) { m_heightPixels = height; }

	vsRendererState * GetState() { return &m_state; }

	const Settings& GetCurrentSettings() const { return m_currentSettings; }

	vsImage*	Screenshot();
	vsImage*	Screenshot_Async();
	vsImage*	ScreenshotBack();
	vsImage*	ScreenshotDepth();
	vsImage*	ScreenshotAlpha();

	static GLuint		Compile(const vsString& vert, const vsString& frag );
	static void			Compile(GLuint program, const vsString& vert, const vsString&frag, bool requireSuccess = true );
	static void			DestroyShader(GLuint shader);

	vsShader*	DefaultShaderFor( vsMaterialInternal *mat );

};

#endif // VS_RENDERER_OPENGL3_H

