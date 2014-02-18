/*
 *  VS_RenderPipeline.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RenderPipeline.h"
#include "VS_RenderPipelineStage.h"

vsRenderPipeline::vsRenderPipeline( int maxStageCount ):
	m_stage( new vsRenderPipelineStage*[maxStageCount] ),
	m_stageCount(maxStageCount)
{
	for ( int i = 0; i < m_stageCount; i++ )
		m_stage[i] = NULL;
}

vsRenderPipeline::~vsRenderPipeline()
{
	for ( int i = 0; i < m_stageCount; i++ )
		vsDelete(m_stage[i]);
	vsDeleteArray(m_stage);
}

void
vsRenderPipeline::SetStage( int stageId, vsRenderPipelineStage *stage )
{
	vsDelete( m_stage[stageId] );
	m_stage[stageId] = stage;
}

void
vsRenderPipeline::Draw( vsDisplayList *list )
{
	for ( int i = 0; i < m_stageCount; i++ )
	{
		if ( m_stage[i] )
			m_stage[i]->Draw(list);
	}
}
