//
//  VS_Thread.h
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef VS_THREAD_H
#define VS_THREAD_H

struct SDL_Thread;

void vsThread_Init();
void vsThread_Deinit();

class vsThread
{
	SDL_Thread *m_thread;
	vsString m_name;
	bool m_done;

	static int DoStartThread(void* arg);

protected:

	// return 0 for 'no error'.
	virtual int Run() = 0;

public:

	// name is optional, and will be truncated to 16 characters.  In release
	// builds, it will be fully ignored.
	vsThread( const vsString& name = vsEmptyString );
	virtual ~vsThread();

	void Start();
	bool IsDone() { return m_done; }

	static const vsString& GetCurrentThreadName();
	static bool IsMainThread();
};

#endif // VS_THREAD_H

