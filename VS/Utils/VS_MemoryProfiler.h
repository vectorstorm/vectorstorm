/*
 *  VS_MemoryProfiler.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 07/09/2024
 *  Copyright 2024 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_MEMORYPROFILER_H
#define VS_MEMORYPROFILER_H

namespace vsMemoryProfiler
{
	enum Type
	{
		Type_MainFramebuffer,
		Type_Buffer,
		Type_RenderTarget,
		Type_Texture,
		Type_MAX
	};

	void Startup();
	void Shutdown();

	void Add( Type t, uint64_t amt );
	void Remove( Type t, uint64_t amt );

	void Trace();
};

#endif // VS_MEMORYPROFILER_H

