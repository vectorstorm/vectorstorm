/*
 *  VS_RendererPretty.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 17/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_RendererSimple.h"

class vsRendererPretty : public vsRendererSimple
{
	typedef vsRendererSimple Parent;
	
	unsigned int	m_offscreenTexture;
public:
					vsRendererPretty();
	virtual			~vsRendererPretty();
	
	virtual void	PreRender();
	virtual void	RenderDisplayList( vsDisplayList *list );
	virtual void	PostRender();
};
