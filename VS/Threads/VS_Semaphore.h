//
//  VS_Semaphore.h
//  MMORPG2
//
//  Created by Trevor Powell on 3/08/11.
//  Copyright 2011 Trevor Powell. All rights reserved.
//

#ifndef VS_SEMAPHORE_H
#define VS_SEMAPHORE_H

#ifdef UNIX
#include "VS_Mutex.h"
#include <pthread.h>
struct semaphore_t
{
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
};
#else
#include <windows.h>
typedef HANDLE semaphore_t;
#undef PlaySound  // yay, Windows.

#endif

class vsSemaphore
{
	semaphore_t		m_semaphore;
	int				m_value;
public:

    vsSemaphore(unsigned int initialValue);
    ~vsSemaphore();

    void Wait();    // wait until there's some value here.
    void Post();    // increment value of semaphore.

	int PeekValue() { return m_value; }
};


#endif	//VS_SEMAPHORE_H
