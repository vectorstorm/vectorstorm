/*
 *  VS_GraphicsMemoryProfiler.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 07/09/2024
 *  Copyright 2024 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_GraphicsMemoryProfiler.h"
#include <inttypes.h>
#include <atomic>

#include "VS_OpenGL.h"

namespace
{
	std::atomic<uint64_t> m_used[vsGraphicsMemoryProfiler::Type_MAX];

	vsString FormatKB(float kb)
	{
		if ( kb < 1024 )
			return vsFormatString("%0.2f KB", kb);
		else if ( kb < 1024 * 1024 )
		{
			float mb = kb / (1024.f );
			return vsFormatString("%0.2f MB", (float)mb);
		}

		float gb = kb / (1024.f * 1024.f );
		return vsFormatString("%0.2f GB", (float)gb);
	}
	vsString Format(uint64_t bytes)
	{
		float kb = (float)bytes / 1024.f;
		return FormatKB(kb);
	}
};


void vsGraphicsMemoryProfiler::Startup()
{
	for ( int i = 0; i < Type_MAX; i++ )
		m_used[i] = 0L;
}

void vsGraphicsMemoryProfiler::Shutdown()
{
	for ( int i = 0; i < Type_MAX; i++ )
		m_used[i] = 0L;
}

void
vsGraphicsMemoryProfiler::Add( Type t, uint64_t amt )
{
	m_used[t] += amt;
}

void
vsGraphicsMemoryProfiler::Remove( Type t, uint64_t amt )
{
	vsAssert( m_used[t] >= amt, "Removing too much memory from vsGraphicsMemoryProfiler" );
	m_used[t] -= amt;
}


void
vsGraphicsMemoryProfiler::Trace()
{
	vsLog("== vsGraphicsMemoryProfiler Data");
	vsLog("  Main Framebuffer: %s", Format(m_used[vsGraphicsMemoryProfiler::Type_MainFramebuffer]));
	vsLog("           Buffers: %s", Format(m_used[vsGraphicsMemoryProfiler::Type_Buffer]));
	vsLog("    Render Targets: %s", Format(m_used[vsGraphicsMemoryProfiler::Type_RenderTarget]));
	vsLog("          Textures: %s", Format(m_used[vsGraphicsMemoryProfiler::Type_Texture]));
	vsLog("== END vsGraphicsMemoryProfiler Data");

	// int value = 0;
	// this value gets returned in KB
		// glGetIntegerv( GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &value );
		// vsLog("Dedicated MB: %s", FormatKB(value) );
		// // glGetIntegerv( GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_VIDMEM_NVX, &value );
		// // vsLog("Total MB: %d", Format(value*1024) );
		// glGetIntegerv( GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &value );
		// vsLog("Available MB: %s", FormatKB(value) );

}

