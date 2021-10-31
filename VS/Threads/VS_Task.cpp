//
//  VS_Task.cpp
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "VS_Task.h"

#ifdef UNIX
void *vsTask::DoStartThread(void* arg)
#else
DWORD WINAPI vsTask::DoStartThread( LPVOID arg )
#endif
{
#ifdef UNIX
	intptr_t result;

#else
	DWORD result;
#endif
	vsTask *task = (vsTask*)arg;

#if defined(_DEBUG)
	if ( task->m_name.size() > 16 )
		task->m_name.resize(16);

#if defined(__APPLE_CC__)
	pthread_setname_np( task->m_name.c_str() );
#elif defined(UNIX)
	pthread_setname_np( pthread_self(), task->m_name.c_str() );
#endif

#endif

	task->m_done = false;
	result = task->Run();
	task->m_done = true;
	task->m_thread = 0L;

#ifdef UNIX
	return (void*)result;
#else
	return result;
#endif
}

vsTask::vsTask( const vsString& name ):
	m_thread(0),
#ifdef _DEBUG
	m_name(name),
#endif // _DEBUG
	m_done(false)
{
}

vsTask::~vsTask()
{
	if ( !m_done && m_thread != 0 )
	{
#ifdef UNIX
		//		pthread_kill( m_thread, SIGKILL );
#else
		CloseHandle(m_thread);
#endif
		m_thread = 0;
	}
}

void
vsTask::Start()
{
#ifdef UNIX
	pthread_create( &m_thread, nullptr, &vsTask::DoStartThread, (void*)this );

#else
	m_thread = CreateThread(
			nullptr,                   // default security attributes
			0,                      // use default stack size
			&vsTask::DoStartThread, // thread function name
			(void*)this,			// argument to thread function
			0,                      // use default creation flags
			nullptr);					// returns the thread identifier


#endif
}
