//
//  VS_Task.cpp
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "VS_Task.h"
#include <SDL2/SDL_thread.h>
#include <map>

namespace
{
	std::map<SDL_threadID, int> s_threadTable;
	int s_nextId = 0;
};

void vsTask_Init()
{
	// register the main thread so we recognise it in future
	SDL_threadID id = SDL_ThreadID();
	s_threadTable[id] = s_nextId++;
}

int vsTask::DoStartThread(void* arg)
{
	int result = 0;
	vsTask *task = (vsTask*)arg;

	SDL_threadID id = SDL_ThreadID();
	s_threadTable[id] = s_nextId++;

	task->m_done = false;
	result = task->Run();
	task->m_done = true;
	task->m_thread = 0L;

	return result;
}

vsTask::vsTask( const vsString& name ):
	m_thread(0),
	m_name(name),
	m_done(false)
{
}

vsTask::~vsTask()
{
	if ( !m_done && m_thread != 0 )
	{
		SDL_DetachThread(m_thread);
		m_thread = 0;
	}
}

void
vsTask::Start()
{
	m_thread = SDL_CreateThread( DoStartThread, m_name.c_str(), (void*)this );
}

int
vsTask::GetCurrentThreadId()
{
	SDL_threadID id = SDL_ThreadID();
	if ( s_threadTable.find(id) != s_threadTable.end() )
	{
		return s_threadTable[id];
	}
	return -1;
}

