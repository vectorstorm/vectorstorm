/*
 *  VS_Renderer.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDERER_OPENGL2_H
#define VS_RENDERER_OPENGL2_H

#include "VS/Graphics/VS_Texture.h"

#include "VS_Renderer.h"
#include "VS_Color.h"
#include "VS_Fog.h"
#include "VS_Material.h"
#include "VS_RendererState.h"
#include "VS_Texture.h"
#include "Math/VS_Transform.h"
#include "VS_OpenGL.h"

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

class vsRenderer_OpenGL2: public vsRenderer
{
private:

	static GLuint			s_normalProg;
	static GLuint			s_litProg;
	static GLuint			s_normalTexProg;
	static GLuint			s_litTexProg;

	static GLuint			s_normalProgFogLoc;
	static GLuint			s_litProgFogLoc;
	static GLuint			s_normalTexProgFogLoc;
	static GLuint			s_litTexProgFogLoc;
	static bool				s_shadersBuilt;

	vsVector3D           m_currentCameraPosition;
	Settings             m_currentSettings;

	vsRenderTarget *     m_window;
	vsRenderTarget *     m_scene;

    vsRendererState      m_state;

	vsMaterialInternal * m_currentMaterial;
	vsShader *           m_currentShader;

	vsVector3D *         m_currentVertexArray;
	vsVector3D *         m_currentNormalArray;
	vsVector2D *         m_currentTexelArray;
	vsColor *            m_currentColorArray;

	vsRenderBuffer *     m_currentVertexBuffer;
	vsRenderBuffer *     m_currentNormalBuffer;
	vsRenderBuffer *     m_currentTexelBuffer;
	vsRenderBuffer *     m_currentColorBuffer;

	int                  m_currentTransformStackLevel;
	int                  m_currentVertexArrayCount;
	int                  m_currentNormalArrayCount;
	int                  m_currentTexelArrayCount;
	int                  m_currentColorArrayCount;
	int                  m_lightCount;
	bool                 m_inOverlay;
	bool                 m_usingNormalArray;
	bool                 m_usingTexelArray;
	bool                 m_antialias;

	void				SetCameraTransform( const vsTransform2D &t );
	void				SetCameraProjection( const vsMatrix4x4 &m );
	void				Set3DProjection( float fov, float nearPlane, float farPlane );

	virtual void		SetMaterial(vsMaterialInternal *material);
	//virtual void		SetDrawMode(vsDrawMode mode);

	void Resize();

	GLuint			Compile(const char *vert, const char *frag, int vertLength = 0, int fragLength = 0 );
	void			DestroyShader(GLuint shader);

public:

	enum
	{
		Flag_Fullscreen = BIT(0),
		Flag_VSync = BIT(1),
		Flag_Resizable = BIT(2)
	};
	vsRenderer_OpenGL2(int width, int height, int depth, int flags);
	virtual ~vsRenderer_OpenGL2();

	void	UpdateVideoMode(int width, int height, int depth, bool fullscreen);

	void	PreRender( const Settings &s );
	void	RenderDisplayList( vsDisplayList *list );
	void	RawRenderDisplayList( vsDisplayList *list );
	void	PostRender();

	bool	PreRenderTarget( const vsRenderer::Settings &s, vsRenderTarget *target );
	bool	PostRenderTarget( vsRenderTarget *target );

	int		GetWidthPixels() const { return m_widthPixels; }
	int		GetHeightPixels() const { return m_heightPixels; }
	void	SetViewportWidthPixels( int width ) { m_widthPixels = width; }
	void	SetViewportHeightPixels( int height ) { m_heightPixels = height; }

	vsRendererState * GetState() { return &m_state; }

	const Settings& GetCurrentSettings() const { return m_currentSettings; }

	vsImage*	Screenshot();
	vsImage*	ScreenshotDepth();
	vsImage*	ScreenshotAlpha();

#ifdef CHECK_GL_ERRORS
	void			CheckGLError(const char* string);
#else
	inline void			CheckGLError(const char* string) {}
#endif
};

#endif // VS_RENDERER_OPENGL2_H

