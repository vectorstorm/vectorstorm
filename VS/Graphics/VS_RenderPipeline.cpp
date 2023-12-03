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
#include "VS_RenderTarget.h"
#include "VS_Renderer.h"

vsRenderPipeline::vsRenderPipeline( int maxStageCount ):
	m_stage( new vsRenderPipelineStage*[maxStageCount] ),
	m_stageCount(maxStageCount)
{
	for ( int i = 0; i < m_stageCount; i++ )
		m_stage[i] = nullptr;
}

vsRenderPipeline::~vsRenderPipeline()
{
	for ( int i = 0; i < m_stageCount; i++ )
		vsDelete(m_stage[i]);
	vsDeleteArray(m_stage);
}

vsRenderTarget *
vsRenderPipeline::RequestRenderTarget( const RenderTargetRequest& request, vsRenderPipelineStage *stage )
{
	// OKAY!  Let's start by checking all our existing registrations and see
	// whether any of them will match this request.

	for ( vsArrayStore<RenderTargetRegistration>::Iterator it = m_target.Begin();
			it != m_target.End();
			it++ )
	{
		RenderTargetRegistration *reg = *it;
		if ( !reg->IsUsedByStage(stage) && reg->Matches(request) )
		{
			// we've found an existing render target which isn't used by this
			// stage, and which matches our request.

			reg->SetUsedByStage(stage);
			return reg->GetRenderTarget();
		}
	}

	// If we've gotten here, we haven't found a matching already-existing render
	// target.  So let's create a new one!

	vsSurface::Settings settings;
	if ( request.type == RenderTargetRequest::Type_AbsoluteSize )
	{
		settings.width = request.width;
		settings.height = request.height;
	}
	else
	{
		settings.width = vsRenderer::Instance()->GetWidthPixels() >> request.mipmapLevel;
		settings.height = vsRenderer::Instance()->GetHeightPixels() >> request.mipmapLevel;
	}
	settings.depth = request.depth;
	settings.bufferSettings[0].linear = request.linear;
	settings.bufferSettings[0].channels = request.channels;
	settings.bufferSettings[0].format = request.format;
	settings.mipMaps = request.mipmaps;
	settings.stencil = request.stencil;
	vsRenderTarget::Type type = vsRenderTarget::Type_Texture;
	if ( request.antialias )
		type = vsRenderTarget::Type_Multisample;
	vsRenderTarget *newTarget = new vsRenderTarget(type, settings);

	RenderTargetRegistration *reg = new RenderTargetRegistration(newTarget, request);
	reg->SetUsedByStage( stage );
	m_target.AddItem(reg);

	return newTarget;
}

void
vsRenderPipeline::ReleaseRenderTarget( vsRenderTarget *target, vsRenderPipelineStage *stage )
{
	for ( vsArrayStore<RenderTargetRegistration>::Iterator it = m_target.Begin();
			it != m_target.End();
			it++ )
	{
		RenderTargetRegistration *reg = *it;

		if ( reg->GetRenderTarget() == target && reg->IsUsedByStage(stage) )
		{
			reg->SetUsedByStage(stage,false);
		}
	}
}

void
vsRenderPipeline::SetStage( int stageId, vsRenderPipelineStage *stage )
{
	// TODO:  Support changing stages.  The only bit not handled right now is
	// cleaning up no-longer-in-use render targets so that they can be reused
	// or deallocated.  Fixing this will probably involve setting up reference
	// counting so the pipeline can figure out which stages are using which
	// render targets.
	vsAssert( m_stage[stageId] == nullptr, "Resetting a pipeline stage isn't supported" );
	m_stage[stageId] = stage;

	stage->PreparePipeline(this);
}

void
vsRenderPipeline::Draw( vsDisplayList *list )
{
	for ( int i = 0; i < m_stageCount; i++ )
	{
		if ( m_stage[i] && m_stage[i]->IsEnabled() )
			m_stage[i]->Draw(list);
	}
}

void
vsRenderPipeline::PostDraw()
{
	for ( int i = 0; i < m_stageCount; i++ )
	{
		if ( m_stage[i] && m_stage[i]->IsEnabled() )
			m_stage[i]->PostDraw();
	}
}

void
vsRenderPipeline::Prepare()
{
	for ( int i = 0; i < m_stageCount; i++ )
	{
		if ( m_stage[i] )
			m_stage[i]->PreparePipeline(this);
	}

	for ( int i = 0; i < m_target.ItemCount(); )
	{
		RenderTargetRegistration *reg = m_target[i];
		if ( !reg->IsUsedByAnyStage() )
		{
			// we should get rid of this render target!
			m_target.RemoveItem( reg );
		}
		else
			i++;
	}
}

