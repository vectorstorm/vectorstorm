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
#else
#include <windows.h>

typedef HANDLE pthread_t;

#endif

class vsTask
{
    pthread_t   m_thread;
    bool        m_done;

#ifdef UNIX
	static void *DoStartThread(void* arg);
#else
	static DWORD WINAPI DoStartThread( LPVOID arg );
#endif
protected:

    // return 0 for 'no error'.
    virtual int Run() = 0;

public:

            vsTask();
    virtual ~vsTask();

    void    Start();

    bool    IsDone() { return m_done; }

};

#endif // VS_TASK_H

