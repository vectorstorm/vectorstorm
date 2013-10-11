/*
 *  VS_RenderScheme.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11-10-2013.
 *  Copyright 2013 Trevor Powell.  All rights reserved.
 *
 */
#ifndef VS_RENDERSCHEME_H
#define VS_RENDERSCHEME_H

#include "VS_Renderer.h"

class vsRenderScheme
{
protected:
	vsRenderer *	m_renderer;
public:
	vsRenderScheme( vsRenderer *renderer );
	virtual ~vsRenderScheme() {}

	virtual void	SetMaterial( vsMaterialInternal *material ) {}

	virtual void	PreRender( const vsRenderer::Settings &s ) {}
	virtual void	RenderDisplayList( vsDisplayList *list ) {}
	virtual void	PostRender() {}
};

#endif /* VS_RENDERSCHEME_H */

