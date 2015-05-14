/*
 *  MEM_Heap.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 8/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Heap.h"

#ifdef MSVC
#define strncpy strncpy_s
#endif

vsHeap *g_globalHeap;

vsHeap * vsHeap::s_current = NULL;

#define MAX_HEAP_STACK (4)
static vsHeap *	s_stack[MAX_HEAP_STACK] = {NULL,NULL,NULL,NULL};

#undef new
#undef malloc
#undef free

vsHeap::vsHeap(vsString name, size_t size):
	m_name(name)
{
#ifdef VS_OVERLOAD_ALLOCATORS
	if ( s_current )
		m_startOfMemory = s_current->Alloc(size, __FILE__, __LINE__, Type_Heap);
	else
	{
		m_startOfMemory = malloc(size);
		s_current = this;
		Push(this);
	}

	m_endOfMemory = (void *)((char *)m_startOfMemory + size);
	m_memorySize = size;

	m_memoryUsed = 0;
	m_highWaterMark = 0;
	m_totalAllocations = 0;

	m_leakMark = 0;

	//	for ( int i = 0; i < MAX_ALLOCATIONS; i++ )
	//	{
	//		m_unusedBlockList.Append( &m_blockStore[i] );
	//	}

	memBlock *iniBlock = (memBlock *)m_startOfMemory;
	iniBlock->m_start = m_startOfMemory;
	iniBlock->m_end = m_endOfMemory;
	iniBlock->m_size = m_memorySize;
	iniBlock->m_sizeRequested = m_memorySize;
	iniBlock->m_used = false;
	iniBlock->m_next = NULL;
	iniBlock->m_prev = NULL;
	iniBlock->m_nextBlock = NULL;
	iniBlock->m_prevBlock = NULL;

	m_unusedBlockList.Append( iniBlock );
#endif // VS_OVERLOAD_ALLOCATORS
}

vsHeap::~vsHeap()
{
	if ( s_current == this )
	{
	}
}

void
vsHeap::Push( vsHeap *newCurrent )
{
	vsAssert( s_stack[MAX_HEAP_STACK-1] == NULL, "Overflowed stack of vsHeaps!" );

	for ( int i = MAX_HEAP_STACK-1; i > 0; i-- )
		s_stack[i] = s_stack[i-1];
	s_stack[0] = newCurrent;

	s_current = s_stack[0];
}

void
vsHeap::Pop( vsHeap *oldCurrent /* == NULL */ )
{
	//vsAssert( s_stack[1] != NULL, "Underflowed stack of vsHeaps!" );

	// Optional safety measure:
	//
	// if someone has specified which vsHeap they expect to be popping off,
	// then validate that it's actually the heap they expect it to be.
	if ( oldCurrent != NULL )
		vsAssert( s_stack[0] == oldCurrent, "Expected oldCurrent!" );

	for ( int i = 0; i < MAX_HEAP_STACK-1; i++ )
		s_stack[i] = s_stack[i+1];
	s_stack[MAX_HEAP_STACK-1] = NULL;

	s_current = s_stack[0];
}

memBlock *
vsHeap::FindFreeMemBlockOfSize( size_t size )
{
	memBlock *block = m_unusedBlockList.m_next;
	size_t largestBlockSize = 0;
	bool foundMemBlockForAlloc = false;

	while(block)
	{
		if ( block->m_used == false && block->m_size >= size )
		{
			return block;
		}

		if ( block->m_used == false && block->m_size > largestBlockSize )
			largestBlockSize = block->m_size;

		block = block->m_next;
	}

#ifdef _WIN32
	vsLog("Unable to find block of size %lu in heap of size %lu.  Largest block available is %lu.", size, m_memorySize, largestBlockSize);
#else
	vsLog("Unable to find block of size %zu in heap of size %zu.  Largest block available is %zu.", size, m_memorySize, largestBlockSize);
#endif
	vsAssert( foundMemBlockForAlloc, "Out of memory!" );	// if this breaks, we're out of memory, or are suffering from memory fragmentation!
	return NULL;
}

void *
vsHeap::Alloc(size_t size_requested, const char *file, int line, int allocType)
{
	m_mutex.Lock();

	size_t size = size_requested;
	size += sizeof( memBlock ) + sizeof( unsigned long );		// we need to allocate enough space for our new 'memBlock' header, and some bytes on the end.
	size = (size+31) & 0xffffffe0;								// round 'size' up to the nearest 32 bytes, to force alignment.

	memBlock *block = FindFreeMemBlockOfSize(size);

	void * end = (void *)((char *)block->m_start + size);

	if ( block->m_size > (size_t)(size + sizeof( memBlock )) )	// enough extra unused space to allocate another memblock out of it?
	{
		// block is larger than we asked for;  we need to split it.
		memBlock *split = (memBlock *)end;

		split->m_start = end;
		split->m_end = block->m_end;
		split->m_size = block->m_size - size;
		split->m_used = false;

		block->AppendBlock(split);

		block->m_end = end;
		block->m_size = size;

		m_unusedBlockList.Append(split);
		//block->Append(split);
	}
	block->Extract();
	m_blockList.Append(block);

	if ( file )
	{
		strncpy( block->m_filename, file, 128 );
	}
	block->m_line = line;
	block->m_blockId = m_totalAllocations++;
	block->m_allocType = allocType;
	block->m_sizeRequested = size_requested;

	block->m_used = true;
	void *result = (void *)((char *)block->m_start + sizeof(memBlock));

	m_memoryUsed += block->m_size;
	if ( m_memoryUsed > m_highWaterMark )
	{
		m_highWaterMark = m_memoryUsed;
		/*if ( m_highWaterMark > 1024 * 1024 )
		  TraceMemoryBlocks();*/
	}

	if ( allocType != Type_Heap )
	{
		// overwrite everything in the user area, to make it really obvious what
		// memory hasn't been initialised by the user.  (No need to do this if this
		// memory is just being used within another vsHeap)

		memset(result, 0xcdcdcdcd, block->m_size - sizeof( memBlock ) );
	}

	// write a special code just past the end of what the user thinks we're
	// giving them.  We'll check that the user hasn't written over our special
	// code when they free the block, as that would indicate that they've overwritten
	// an array or somesuch.
	void * safety = (void *)((char *)block->m_end - sizeof(unsigned long));
	unsigned long *safetyLong = (unsigned long *)safety;
	*safetyLong = 0xeeeeeeee;

	m_mutex.Unlock();

	return result;
}

void
vsHeap::Free(void *p, int allocType)
{
	m_mutex.Lock();

	p = (void *)((char *)p - sizeof(memBlock));	// adjust pointer to point to the start of its memBlock header

	memBlock *block = (memBlock *)p;
	//memBlock *block = m_blockList.m_next;
	//bool foundBlockToFree = false;

	//while ( block )
	//{
	//	if ( block->m_start == p )
	{
		// make sure the user hasn't overwritten our code past the end of their memory block.
		void * safety = (void *)((char *)block->m_end - sizeof(unsigned long));
		unsigned long *safetyLong = (unsigned long *)safety;
		vsAssert( *safetyLong == 0xeeeeeeee, "Buffer overflow detected!" );	// if we hit this assert, someone has overwritten the bounds of this memory buffer!

		if( block->m_allocType != allocType )
		{
			const char *allocFunction[] =
			{
				"vsHeap constructor",
				"Static alloc",
				"malloc",
				"new",
				"new []"
			};
			const char *freeFunction[] =
			{
				"vsHeap destructor",
				"None",
				"free",
				"delete",
				"delete []"
			};
			vsLog("Error:  Allocation from %s line %d was allocated using %s", block->m_filename, block->m_line, allocFunction[(int)block->m_allocType]);
			vsLog("Error:   but was freed using %s;  should have been %s!", freeFunction[allocType], freeFunction[(int)block->m_allocType]);
		}

		// check if we can merge together with the prev or the next block.
		void *	userArea = (void *)((char *)block->m_start + sizeof(memBlock));
		size_t	userSize = block->m_size - sizeof(memBlock);
		memset(userArea, 0xdddddddd, userSize );
		block->m_used = false;
		m_memoryUsed -= block->m_size;

		memBlock *nextBlock = block->m_nextBlock;
		memBlock *prevBlock = block->m_prevBlock;

		if ( nextBlock && !nextBlock->m_used )
		{
			//memBlock *nextBlock = block->m_next;

			// next block isn't being used;  let's merge it into us!
			block->m_end = nextBlock->m_end;
			block->m_size += nextBlock->m_size;

			nextBlock->ExtractBlock();
			nextBlock->Extract();
		}
		if ( prevBlock && prevBlock != &m_unusedBlockList && !prevBlock->m_used )
		{
			// previous block isn't being used;  let's merge ourself into it!
			prevBlock->m_end = block->m_end;
			prevBlock->m_size += block->m_size;
			block->ExtractBlock();
			block->Extract();
			m_mutex.Unlock();
			return;
		}

		block->Extract();
		m_unusedBlockList.Append(block);
		//foundBlockToFree = true;
		m_mutex.Unlock();
		return;
	}
	//	block = block->m_next;
	//}

	//vsAssert(foundBlockToFree, "Tried to free a block we didn't allocate!");	// tried to free a block we didn't know about!

	m_mutex.Unlock();
}

void
vsHeap::PrintStatus()
{
#ifdef VS_OVERLOAD_ALLOCATORS
	vsLog(" >> MEMORY STATUS");

	size_t bytesFree = m_memorySize - m_memoryUsed;
	size_t largestBlock = 0;

	memBlock *block = m_unusedBlockList.m_next;

	while ( block )
	{
		if ( !block->m_used && block->m_size > largestBlock )
			largestBlock = block->m_size;

		block = block->m_next;
	}

#ifdef _WIN32
	vsLog(" >> Heap current usage %lu / %lu (%0.2f%% usage)", m_memoryUsed, m_memorySize, 100.0f*m_memoryUsed/m_memorySize);
	vsLog(" >> Heap highwater usage %lu / %lu (%0.2f%% usage)", m_highWaterMark, m_memorySize, 100.0f*m_highWaterMark/m_memorySize);
	vsLog(" >> Heap largest free block %lu / %lu bytes free (%0.2f%% fragmentation)", largestBlock, bytesFree, 100.0f - (100.0f*largestBlock / bytesFree) );
#else
	vsLog(" >> Heap current usage %zu / %zu (%0.2f%% usage)", m_memoryUsed, m_memorySize, 100.0f*m_memoryUsed/m_memorySize);
	vsLog(" >> Heap highwater usage %zu / %zu (%0.2f%% usage)", m_highWaterMark, m_memorySize, 100.0f*m_highWaterMark/m_memorySize);
	vsLog(" >> Heap largest free block %zu / %zu bytes free (%0.2f%% fragmentation)", largestBlock, bytesFree, 100.0f - (100.0f*largestBlock / bytesFree) );
#endif
#endif // VS_OVERLOAD_ALLOCATORS

}

void
vsHeap::CheckForLeaks()
{
	m_mutex.Lock();
	bool foundLeak = false;
	memBlock *block = m_blockList.m_next;

	while ( block )
	{
		if ( block->m_used && block->m_blockId > m_leakMark )
		{
			if ( !foundLeak )
			{
				vsLog("\nERROR:  LEAKS DETECTED!\n-------------------\nLeaked blocks follow:\n");
				foundLeak = true;
			}
			vsLog("[%s:%d] %s:%d : %d bytes", m_name.c_str(), block->m_blockId, block->m_filename, block->m_line, block->m_sizeRequested);
		}
		block = block->m_next;
	}
	m_mutex.Unlock();

	vsAssert(!foundLeak, "Memory leaks found!  Details to stdout.");
}

void
vsHeap::TraceMemoryBlocks()
{
	memBlock *block = m_blockList.m_next;

	while ( block )
	{
		if ( block->m_used )
		{
#ifdef _WIN32
			vsLog("[%d] %s:%d : %lu bytes", block->m_blockId, block->m_filename, block->m_line, block->m_sizeRequested);
#else
			vsLog("[%d] %s:%d : %zu bytes", block->m_blockId, block->m_filename, block->m_line, block->m_sizeRequested);
#endif
		}
		block = block->m_next;
	}
}

void
vsHeap::PrintBlockList()
{
	memBlock *block = m_blockList.m_next;

	while ( block )
	{
#ifdef _WIN32
		vsLog("[%d] %s:%d : %lu bytes", block->m_blockId, block->m_filename, block->m_line, block->m_sizeRequested);
#else
		vsLog("[%d] %s:%d : %zu bytes", block->m_blockId, block->m_filename, block->m_line, block->m_sizeRequested);
#endif
		block = block->m_next;
	}
}

memBlock::memBlock():
	m_start(0),
	m_end(0),
	m_size(0),
	m_used(false),
	m_next(NULL),
	m_prev(NULL)
{
}


void
memBlock::Append( memBlock *block )
{
	block->m_next = m_next;
	block->m_prev = this;

	if ( m_next )
		m_next->m_prev = block;
	m_next = block;
}

void
memBlock::Extract()
{
	if ( m_next )
		m_next->m_prev = m_prev;
	if ( m_prev )
		m_prev->m_next = m_next;

	m_next = m_prev = NULL;
}

void
memBlock::AppendBlock( memBlock *block )
{
	block->m_nextBlock = m_nextBlock;
	block->m_prevBlock = this;

	if ( m_nextBlock )
		m_nextBlock->m_prevBlock = block;
	m_nextBlock = block;
}

void
memBlock::ExtractBlock()
{
	if ( m_nextBlock )
		m_nextBlock->m_prevBlock = m_prevBlock;
	if ( m_prevBlock )
		m_prevBlock->m_nextBlock = m_nextBlock;

	m_nextBlock = m_prevBlock = NULL;
}

void * MyMalloc(size_t size, const char *fileName, int lineNumber, int allocType)
{
	void * result;

	if ( vsHeap::GetCurrent() )
	{
		result = vsHeap::GetCurrent()->Alloc(size, fileName, lineNumber, allocType);
		// vsLog("Allocating %s:%d, 0x%x", fileName, lineNumber, (unsigned int)result);
		return result;
	}
	return malloc(size);
}

void MyFree(void *p, int allocType)
{
	// vsAssert( (int)p != 0xcdcdcdcd, "Tried to free an uninitialised pointer?");
	// vsLog("Deallocating 0x%x", (unsigned int)p);
	if ( vsHeap::GetCurrent() )
	{
		if ( vsHeap::GetCurrent()->Contains(p) )
		{
			return vsHeap::GetCurrent()->Free(p, allocType);
		}
		else
		{
			// check parent heaps!
			for ( int i = 1; i < MAX_HEAP_STACK; i++ )
			{
				if ( s_stack[i] && s_stack[i]->Contains(p) )
				{
					return s_stack[i]->Free(p, allocType);
				}
			}
		}
	}
	else
	{
		free(p);
	}
}

#ifdef VS_OVERLOAD_ALLOCATORS
void* operator new(std::size_t n, const char* file, size_t line)
{
	if (n == 0) n = 1;

	void* result = MyMalloc(n, file, line, Type_New);
	return result;
}

// Array regular new
void* operator new[](std::size_t n, const char*file, size_t line)
{
	void* result = MyMalloc(n, file, line, Type_NewArray);
	return result;
}

// Scalar regular delete (doesn't throw either)
void operator delete(void* p) throw()
{
	MyFree(p, Type_New);
}

// Scalar nothrow delete
void operator delete(void* p, std::nothrow_t const&) throw()
{
	::operator delete(p);
}

// Array regular delete
void operator delete[](void* p) throw()
{
	MyFree(p, Type_NewArray);
}

void operator delete(void* p, const char* file, size_t line)
{
	::operator delete(p);
}

void operator delete[](void* p, const char* file, size_t line)
{
	::operator delete[](p);
}



// Array nothrow delete
void operator delete[](void* p, std::nothrow_t const&) throw()
{
	::operator delete[](p);
}

#endif //VS_OVERLOAD_ALLOCATORS
