//
//  VS_Mutex.cpp
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "VS_Mutex.h"

#ifdef __APPLE__
// Apple version of vsMutex.
//
// On OS X, pthread mutexes are very heavy, and impact our performance to a silly
// extent.  And since we're a game (and probably not even multithreaded the vast
// majority of the time), we don't ever actually hold mutexes for more than a
// brief moment.  So let's just go ahead and use a spinlock, instead.  This keeps
// us from needing to talk to the kernel, and makes every "mutex" operation
// substantially faster.
//
vsMutex::vsMutex():
	m_mutex(0)
{
}

vsMutex::~vsMutex()
{
}

bool
vsMutex::TryLock()
{
	return ( OSSpinLockTry( &m_mutex ) );
}

void
vsMutex::Lock()
{
	OSSpinLockLock( &m_mutex );
}

void
vsMutex::Unlock()
{
	OSSpinLockUnlock( &m_mutex );
}

#elif defined(UNIX)
// Unix version of vsMutex

#include <pthread.h>

vsMutex::vsMutex()
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_DEFAULT );

    pthread_mutex_init( &m_mutex, &attr );
}

vsMutex::~vsMutex()
{
    pthread_mutex_destroy( &m_mutex );
}

bool
vsMutex::TryLock()
{
	return ( pthread_mutex_trylock( &m_mutex ) == 0 );
}

void
vsMutex::Lock()
{
    int result = pthread_mutex_lock( &m_mutex );
    vsAssert(result == 0, "Error %d in pthread_mutex_lock");
}

void
vsMutex::Unlock()
{
    int result = pthread_mutex_unlock( &m_mutex );
    vsAssert(result == 0, "Error %d in pthread_mutex_lock");
}

#else   // !UNIX
// Win32 version of vsMutex

vsMutex::vsMutex()
{
	m_mutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

}

vsMutex::~vsMutex()
{
	CloseHandle(m_mutex);
}

void
vsMutex::Lock()
{
	WaitForSingleObject( m_mutex, INFINITE );
}

void
vsMutex::Unlock()
{
	ReleaseMutex( m_mutex );
}


#endif
