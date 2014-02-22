/*
 *  VS_RenderPipelineStageBlit.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 22/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RenderPipelineStageBlit.h"
#include "VS_DisplayList.h"

vsRenderPipelineStageBlit::vsRenderPipelineStageBlit(vsRenderTarget *from, vsRenderTarget *to):
	m_from(from),
	m_to(to)
{
}

void
vsRenderPipelineStageBlit::Draw( vsDisplayList *list )
{
	list->BlitRenderTarget( m_from, m_to );
}
