//
//  VS_Task.h
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef VS_TASK_H
#define VS_TASK_H

struct SDL_Thread;

void vsTask_Init();

class vsTask
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
	vsTask( const vsString& name = vsEmptyString );
	virtual ~vsTask();

	void Start();
	bool IsDone() { return m_done; }

	static const vsString& GetCurrentThreadName();
};

#endif // VS_TASK_H

