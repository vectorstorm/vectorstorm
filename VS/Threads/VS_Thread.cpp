//
//  VS_Thread.cpp
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "VS_Thread.h"
#include "VS_Mutex.h"
#include <SDL3/SDL_thread.h>
#include <map>


namespace
{
	vsMutex tableLock;

	SDL_ThreadID s_mainThread;
	std::map<SDL_ThreadID, vsString> s_threadTable;
	vsString unknownName = "unk";
};

void vsThread_Init()
{
	// register the main thread so we recognise it in future
	SDL_ThreadID id = SDL_GetCurrentThreadID();
	s_mainThread = id;

	tableLock.Lock();
		s_threadTable[id] = "main";
	tableLock.Unlock();
}

void vsThread_Deinit()
{
	tableLock.Lock();
		s_threadTable.clear();
	tableLock.Unlock();
}

int vsThread::DoStartThread(void* arg)
{
	int result = 0;
	vsThread *thread = (vsThread*)arg;

	SDL_ThreadID id = SDL_GetCurrentThreadID();
	{
		tableLock.Lock();
		s_threadTable[id] = thread->m_name;
		tableLock.Unlock();
	}

	thread->m_done = false;
	result = thread->Run();
	thread->m_done = true;

	return result;
}

vsThread::vsThread( const vsString& name ):
	m_thread(0),
	m_name(name),
	m_done(false)
{
}

vsThread::~vsThread()
{
	if ( m_thread != 0 )
	{
		// SDL_DetachThread(m_thread);
		int status = 0;
		SDL_WaitThread(m_thread, &status);
		vsLog("SDL_WaitThread: status %d", status);
		m_thread = 0;
	}
}

void
vsThread::Start()
{
	m_thread = SDL_CreateThread( DoStartThread, m_name.c_str(), (void*)this );
}

const vsString&
vsThread::GetCurrentThreadName()
{
	SDL_ThreadID id = SDL_GetCurrentThreadID();
	if ( s_threadTable.find(id) != s_threadTable.end() )
	{
		return s_threadTable[id];
	}
	return unknownName;
}

bool
vsThread::IsMainThread()
{
	SDL_ThreadID id = SDL_GetCurrentThreadID();
	return (id == s_mainThread);
}

