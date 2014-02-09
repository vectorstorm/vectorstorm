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

	// do any operations which need to be performed after a resize
	virtual void	Resize() {}

	virtual void	SetMaterial( vsMaterialInternal *material ) {}

	virtual void	PreRender( const vsRenderer::Settings &s ) {}
	virtual void	RenderDisplayList( vsDisplayList *list ) {}
	virtual void	PostRender() {}

	virtual bool	PreRenderTarget( const vsRenderer::Settings &s, vsRenderTarget *target ) { return true;}
	virtual bool	PostRenderTarget( vsRenderTarget *target ) { return true; }
};

#endif /* VS_RENDERSCHEME_H */

