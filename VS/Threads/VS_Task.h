//
//  VS_Task.h
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef VS_TASK_H
#define VS_TASK_H

#ifdef UNIX
#include <pthread.h>
typedef pthread_t thread_t;
#else
#include <windows.h>

typedef HANDLE thread_t;

#endif

void vsTask_Init();

class vsTask
{
	thread_t m_thread;
#ifdef _DEBUG
	vsString m_name;
#endif // DEBUG
	bool m_done;

#ifdef UNIX
	static void *DoStartThread(void* arg);
#else
	static DWORD WINAPI DoStartThread( LPVOID arg );
#endif

protected:

	// return 0 for 'no error'.
	virtual int Run() = 0;

public:

	// name is optional, and will be truncated to 16 characters.  In release
	// builds, it will be fully ignored.
	vsTask( const vsString& name = vsEmptyString );
	virtual ~vsTask();

	void Start();
	bool IsDone() { return m_done; }

	static int GetCurrentThreadId();
};

#endif // VS_TASK_H

