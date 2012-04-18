//
//  VS_Semaphore.cpp
//  MMORPG2
//
//  Created by Trevor Powell on 3/08/11.
//  Copyright 2011 Trevor Powell. All rights reserved.
//

#include "VS_Semaphore.h"

#ifdef UNIX
// unix version of semaphore
vsSemaphore::vsSemaphore(unsigned int initialValue)
{
	pthread_mutex_init( &m_semaphore.mutex, NULL );
	pthread_mutex_lock( &m_semaphore.mutex );
    pthread_cond_init( &m_semaphore.cond, NULL );
	m_value = initialValue;
	pthread_mutex_unlock( &m_semaphore.mutex );
}

vsSemaphore::~vsSemaphore()
{
	pthread_mutex_lock(&m_semaphore.mutex);
	pthread_cond_destroy( &m_semaphore.cond );
	vsAssert( m_value == 0, "Semaphore not properly released before destruction" );
	pthread_mutex_unlock(&m_semaphore.mutex);
	pthread_mutex_destroy( &m_semaphore.mutex );
}

void
vsSemaphore::Wait()
{
	pthread_mutex_lock(&m_semaphore.mutex);
	while ( m_value == 0 )
	{
		pthread_cond_wait( &m_semaphore.cond, &m_semaphore.mutex );
	}
	m_value--;

	pthread_mutex_unlock(&m_semaphore.mutex);
}

void
vsSemaphore::Post()
{
	pthread_mutex_lock(&m_semaphore.mutex);
	m_value++;
	pthread_cond_signal( &m_semaphore.cond );
	pthread_mutex_unlock(&m_semaphore.mutex);
}

#else

vsSemaphore::vsSemaphore(unsigned int initialValue)
{
	m_semaphore = CreateSemaphore( NULL, initialValue, 100000, NULL );
}

vsSemaphore::~vsSemaphore()
{
	CloseHandle( m_semaphore );
}

void
vsSemaphore::Wait()
{
	WaitForSingleObject( m_semaphore, INFINITE );
}

void
vsSemaphore::Post()
{
	ReleaseSemaphore( m_semaphore, 1, NULL );
}


#endif
