/*
 *  VS_RenderSchemeFixedFunction.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_RenderScheme.h"

class vsRenderSchemeFixedFunction : public vsRenderScheme
{
	typedef vsRenderScheme Parent;

	unsigned int	m_offscreenTexture;
public:
					vsRenderSchemeFixedFunction( vsRenderer *renderer );
	virtual			~vsRenderSchemeFixedFunction();

	virtual void	PreRender( const vsRenderer::Settings &s );
	virtual void	RenderDisplayList( vsDisplayList *list );
	virtual void	PostRender();
};
