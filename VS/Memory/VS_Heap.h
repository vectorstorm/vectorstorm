/*
 *  MEM_Heap.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 8/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef MEM_HEAP_H
#define MEM_HEAP_H

#include "VS/Threads/VS_Spinlock.h"


class memBlock
{
public:

	void *		m_start;
	void *		m_end;
	size_t		m_size;
	size_t		m_sizeRequested;
	int			m_blockId;

	char		m_filename[128];
	int			m_line;

	char		m_allocType;

	bool		m_used;

	memBlock *	m_next;
	memBlock *	m_prev;

	memBlock *	m_nextBlock;
	memBlock *	m_prevBlock;

	memBlock();

	void Extract();
	void Append( memBlock *block );

	void ExtractBlock();
	void AppendBlock( memBlock *block );
};

enum
{
	Type_Heap,
	Type_Static,
	Type_Malloc,
	Type_New,
	Type_NewArray
};

class vsHeap
{
#define MAX_ALLOCATIONS (4096)

	vsString m_name;
	void *	m_startOfMemory;
	void *	m_endOfMemory;
	size_t	m_memorySize;

	size_t	m_memoryUsed;
	size_t	m_highWaterMark;
	size_t	m_totalAllocations;

	int		m_leakMark;

	memBlock	m_blockList;
	memBlock	m_unusedBlockList;
//	memBlock	m_blockStore[MAX_ALLOCATIONS];

	static vsHeap * s_current;

	memBlock *	FindFreeMemBlockOfSize(size_t size);
//	memBlock *	GetUnusedMemBlock();
	vsSpinlock m_lock;

public:
	vsHeap(vsString name, size_t bufferSize);
	vsHeap(vsString name, void *buffer, int bufferSize);
	~vsHeap();

	static vsHeap *	GetCurrent() { return s_current; }
	bool				Contains(void *p) { return (p >= m_startOfMemory && p < m_endOfMemory); }

	void *	Alloc(size_t size, const char *fileName, int line, int allocType);	// Assumes mutex is already locked
	void	Free(void *ptr, int allocType); // Assumes mutex is already locked

	static void	Push( vsHeap *newCurrent );	// push a new allocator context
	static void	Pop( vsHeap *oldCurrent = nullptr );							// pop it off.

	void	PrintStatus();
	void	PrintBlockList();
	void	CheckForLeaks();
	void	TraceMemoryBlocks();

	void	SetMarkForLeakTesting() { m_leakMark = (int)m_totalAllocations; }
};


#endif // MEM_HEAP_H
