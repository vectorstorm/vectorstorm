//
//  VS_Mutex.h
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 VectorStorm Pty Ltd. All rights reserved.
//

#ifndef VS_MUTEX_H
#define VS_MUTEX_H

// for the moment, we don't ever actually want to use mutexes;
// we're a video game!  Let's use spinlocks instead.

#include "SDL2/SDL.h"
typedef SDL_mutex* mutex_t;

class vsMutex
{
    mutex_t   m_mutex;

public:
    vsMutex();
    ~vsMutex();

	bool TryLock();
    void Lock();
    void Unlock();
};

class vsScopedLock

{
	vsMutex& m_mutex;
public:
	vsScopedLock( vsMutex& m ):
		m_mutex(m)
	{
		m_mutex.Lock();
	}

	~vsScopedLock()
	{
		m_mutex.Unlock();
	}
};

#endif // VS_MUTEX_H

