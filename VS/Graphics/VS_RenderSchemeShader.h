/*
 *  VS_RenderSchemeShader.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 6/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDERSCHEMESHADER_H

#include "VS_RenderSchemeBloom.h"

// for now, derive off vsRendererBloom.  TODO:  Stop using vsRendererBloom, and make its bloom pass something
// which can be called explicitly from game code, or something like that.

class vsRenderSchemeShader : public vsRenderSchemeBloom, public vsSingleton<vsRenderSchemeShader>
{
	typedef vsRenderSchemeBloom Parent;

	static GLuint			s_normalProg;
	static GLuint			s_litProg;
	static GLuint			s_normalTexProg;
	static GLuint			s_litTexProg;

	static GLuint			s_normalProgFogLoc;
	static GLuint			s_litProgFogLoc;
	static GLuint			s_normalTexProgFogLoc;
	static GLuint			s_litTexProgFogLoc;

	static bool				s_shadersBuilt;

	vsMaterialInternal *	m_currentMaterial;
	vsShader *				m_currentShader;

	virtual void			SetMaterial(vsMaterialInternal *material);

public:
					vsRenderSchemeShader( vsRenderer *renderer );
	virtual			~vsRenderSchemeShader();

	static bool		Supported(bool experimental = false);
};

#endif // VS_RENDERERSHADER_H

