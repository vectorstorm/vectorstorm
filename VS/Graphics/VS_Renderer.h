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
class vsRenderScheme;
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

#define MAX_VERTS_IN_ARRAY (100)
#define MAX_STACK_LEVEL (30)

	vsDisplayList *		m_shaderList;

	vsTexture *			m_currentTexture;
	vsVector3D			m_currentCameraPosition;

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

	vsRenderScheme *	m_scheme;

    vsRendererState		m_state;

	vsMaterialInternal *m_currentMaterial;

	vsVector3D *		m_currentVertexArray;
	vsVector3D *		m_currentNormalArray;
	vsVector2D *		m_currentTexelArray;
	vsColor *			m_currentColorArray;

	vsRenderBuffer *	m_currentVertexBuffer;
	vsRenderBuffer *	m_currentNormalBuffer;
	vsRenderBuffer *	m_currentTexelBuffer;
	vsRenderBuffer *	m_currentColorBuffer;

	vsColor				m_overlayColorArray[MAX_VERTS_IN_ARRAY];
	vsTransform2D		m_transformStack[MAX_STACK_LEVEL];
	//vsMaterial*			m_materialStack[MAX_STACK_LEVEL];
	int					m_currentTransformStackLevel;
	int					m_currentMaterialStackLevel;
	int					m_currentVertexArrayCount;
	int					m_currentNormalArrayCount;
	int					m_currentTexelArrayCount;
	int					m_currentColorArrayCount;
	int					m_lightCount;
	bool				m_inOverlay;
	bool				m_usingNormalArray;
	bool				m_usingTexelArray;
	bool				m_usingColorArray;

	void				SetCameraTransform( const vsTransform2D &t );
	void				SetCameraProjection( const vsMatrix4x4 &m );
	void				Set3DProjection( float fov, float nearPlane, float farPlane );

	virtual void		SetMaterial(vsMaterialInternal *material);
	//virtual void		SetDrawMode(vsDrawMode mode);

public:

	vsRenderer(int width, int height, int depth, bool fullscreen, bool vsync);
	virtual ~vsRenderer();

	void	UpdateVideoMode(int width, int height, int depth, bool fullscreen);

	void	PreRender( const Settings &s );
	void	RenderDisplayList( vsDisplayList *list );
	void	RawRenderDisplayList( vsDisplayList *list );
	void	PostRender();

	bool	PreRenderTarget( const vsRenderer::Settings &s, vsRenderTarget *target );
	bool	PostRenderTarget( vsRenderTarget *target );

	bool	SupportsShaders() { return false; }

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

#endif // VS_RENDERER_H
