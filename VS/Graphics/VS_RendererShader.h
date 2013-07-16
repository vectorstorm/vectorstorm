/*
 *  VS_RendererShader.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 6/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_RENDERERSHADER_H
#define VS_RENDERERSHADER_H

#include "VS_RendererBloom.h"

// for now, derive off vsRendererBloom.  TODO:  Stop using vsRendererBloom, and make its bloom pass something
// which can be called explicitly from game code, or something like that.

class vsRendererShader : public vsRendererBloom, public vsSingleton<vsRendererShader>
{
	typedef vsRendererBloom Parent;

	static GLuint			s_addProg;
	static GLuint			s_normalProg;
	static GLuint			s_litProg;
	static GLuint			s_normalTexProg;
	static GLuint			s_litTexProg;

	static bool				s_shadersBuilt;

	virtual void			SetMaterial(vsMaterialInternal *material);

public:
					vsRendererShader();
	virtual			~vsRendererShader();

	virtual void	InitPhaseTwo(int width, int height, int depth, bool fullscreen);
	virtual void	Deinit();

	static bool		Supported(bool experimental = false);
};

#endif // VS_RENDERERSHADER_H

