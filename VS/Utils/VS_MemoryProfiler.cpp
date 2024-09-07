/*
 *  VS_MemoryProfiler.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 07/09/2024
 *  Copyright 2024 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_MemoryProfiler.h"
#include <inttypes.h>

namespace
{
	uint64_t m_used[vsMemoryProfiler::Type_MAX];

	vsString Format(uint64_t v)
	{
		float megabytes = v / (1024 * 1024);

		return vsFormatString("%0.2f", megabytes);
	}
};


void vsMemoryProfiler::Startup()
{
	for ( int i = 0; i < Type_MAX; i++ )
		m_used[i] = 0L;
}

void vsMemoryProfiler::Shutdown()
{
	for ( int i = 0; i < Type_MAX; i++ )
		m_used[i] = 0L;
}

void
vsMemoryProfiler::Add( Type t, uint64_t amt )
{
	m_used[t] += amt;
}

void
vsMemoryProfiler::Remove( Type t, uint64_t amt )
{
	vsAssert( m_used[t] >= amt, "Removing too much memory from vsMemoryProfiler" );
	m_used[t] -= amt;
}


void
vsMemoryProfiler::Trace()
{
	vsLog("== vsMemoryProfiler Data");
	vsLog("  Main Framebuffer: %" PRIu64 " MB", Format(m_used[vsMemoryProfiler::Type_MainFramebuffer]));
	vsLog("           Buffers: %" PRIu64 " MB", Format(m_used[vsMemoryProfiler::Type_Buffer]));
	vsLog("    Render Targets: %" PRIu64 " MB", Format(m_used[vsMemoryProfiler::Type_RenderTarget]));
	vsLog("          Textures: %" PRIu64 " MB", Format(m_used[vsMemoryProfiler::Type_Texture]));
	vsLog("== END vsMemoryProfiler Data");
}

