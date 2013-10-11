/*
 *  VS_RendererSimple.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDERER_SIMPLE_H
#define VS_RENDERER_SIMPLE_H

#include "VS_Renderer.h"

#include "VS_Color.h"
#include "VS_Fog.h"
#include "VS_Material.h"
#include "VS_RendererState.h"
#include "VS_Texture.h"
#include "VS_Transform.h"

class vsDisplayList;
class vsOverlay;
class vsMaterialInternal;
class vsTransform2D;
class vsVector2D;
class vsRenderBuffer;
class vsShader;
struct SDL_Surface;

//#define CHECK_GL_ERRORS

class vsRendererSimple : public vsRenderer
{
    typedef vsRenderer Parent;
#define MAX_VERTS_IN_ARRAY (100)
#define MAX_STACK_LEVEL (30)

	vsDisplayList *		m_shaderList;

	vsTexture *			m_currentTexture;
	vsVector3D			m_currentCameraPosition;

protected:
    vsRendererState		m_state;

	vsMaterialInternal *m_currentMaterial;

	vsVector3D *		m_currentVertexArray;
	vsVector3D *		m_currentNormalArray;
	vsVector2D *		m_currentTexelArray;
	vsColor *			m_currentColorArray;

    vsShader *          m_currentShader;

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

	vsRendererSimple();
	virtual ~vsRendererSimple();

	virtual void	Init(int width, int height, int depth, bool fullscreen);
	virtual void	Deinit();

	virtual void	SetOverlay( const vsOverlay & ) {}

	virtual void	PreRender( const Settings &s );
	virtual void	RenderDisplayList( vsDisplayList *list );
	virtual void	RawRenderDisplayList( vsDisplayList *list );
	virtual void	PostRender();

	virtual vsImage*	Screenshot();
	virtual vsImage*	ScreenshotDepth();
	virtual vsImage*	ScreenshotAlpha();

#ifdef CHECK_GL_ERRORS
	void			CheckGLError(const char* string);
#else
	inline void			CheckGLError(const char* string) {}
#endif
};

#endif // VS_RENDERER_SIMPLE_H
