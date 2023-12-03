/*
 *  VS_RenderPipeline.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_RENDERPIPELINE_H
#define VS_RENDERPIPELINE_H

class vsDisplayList;
class vsRenderPipelineStage;

#include "Utils/VS_Array.h"
#include "Utils/VS_ArrayStore.h"
#include "VS_RenderTarget.h"

struct RenderTargetRequest
{
	enum Type
	{
		Type_AbsoluteSize,
		Type_MipmapLevel
	};
	Type type;

	// absolute size fields
	int width;
	int height;

	// mipmaplevel fields
	int mipmapLevel; // 0 == same dimensions as main render buffer.  1 == half dimensions, etc.

	// general fields
	bool depth;     // includes depth buffer
	bool stencil;   // has a stencil buffer
	bool linear;    // linear sampling
	vsSurface::Channels channels;
	vsSurface::Format format;
	bool mipmaps;   // automatic mipmaps of the contents
	bool antialias; // antialiasing enabled
	bool share;     // this target can be shared with other stages

	RenderTargetRequest():
		type(Type_AbsoluteSize),
		width(128),
		height(128),
		mipmapLevel(0),
		depth(false),
		stencil(false),
		linear(false),
		channels(vsSurface::Channels_RGBA),
		format(vsSurface::Format_Byte),
		mipmaps(false),
		antialias(false),
		share(true)
	{
	}

	bool operator==(const RenderTargetRequest& o) const
	{
		return (type == o.type &&
			width == o.width &&
			height == o.height &&
			mipmapLevel == o.mipmapLevel &&
			depth == o.depth &&
			stencil == o.stencil &&
			linear == o.linear &&
			channels == o.channels &&
			format == o.format &&
			mipmaps == o.mipmaps &&
			antialias == o.antialias &&
			share == o.share);
	}
};

class RenderTargetRegistration
{
	vsRenderTarget *target;
	vsArray<vsRenderPipelineStage*> user;
	RenderTargetRequest request;

public:

	RenderTargetRegistration():
		target(nullptr),
		user(),
		request()
	{
	}

	RenderTargetRegistration( vsRenderTarget *t, const RenderTargetRequest& req ):
		target(t),
		user(),
		request(req)
	{
	}

	~RenderTargetRegistration()
	{
		vsDelete(target);
	}

	bool Matches( const RenderTargetRequest& req ) const
	{
		return request.share && (request == req);
	}

	bool IsUsedByStage( vsRenderPipelineStage *stage ) const
	{
		return user.Contains(stage);
	}

	bool IsUsedByAnyStage() const
	{
		return !user.IsEmpty();
	}

	void SetUsedByStage( vsRenderPipelineStage *stage, bool used=true )
	{
		if ( used )
		{
			user.AddItem(stage);
		}
		else
		{
			user.RemoveItem(stage);
		}
	}

	vsRenderTarget *GetRenderTarget() { return target; }
};

class vsRenderPipeline
{
	vsRenderPipelineStage ** m_stage;
	int m_stageCount;

	vsArrayStore<RenderTargetRegistration> m_target;

public:

	vsRenderPipeline( int maxStageCount );
	~vsRenderPipeline();

	vsRenderTarget *RequestRenderTarget( const RenderTargetRequest& request, vsRenderPipelineStage *stage );
	void ReleaseRenderTarget( vsRenderTarget *target, vsRenderPipelineStage *stage );

	void SetStage( int stageId, vsRenderPipelineStage *stage );
	void Draw( vsDisplayList *list );
	void PostDraw(); // called after everything is submitted to GPU

	void Prepare(); // prepare all stages again
};

#endif // VS_RENDERPIPELINE_H

