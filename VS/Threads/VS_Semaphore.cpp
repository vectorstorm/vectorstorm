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
	pthread_mutex_init( &m_semaphore.mutex, nullptr );
	pthread_mutex_lock( &m_semaphore.mutex );
    pthread_cond_init( &m_semaphore.cond, nullptr );
	m_value = initialValue;
	m_released = false;
	pthread_mutex_unlock( &m_semaphore.mutex );
}

vsSemaphore::~vsSemaphore()
{
	vsAssert(m_released, "Semaphore destroyed without being released?");
	pthread_mutex_lock(&m_semaphore.mutex);
	pthread_cond_destroy( &m_semaphore.cond );
	pthread_mutex_unlock(&m_semaphore.mutex);
	pthread_mutex_destroy( &m_semaphore.mutex );
}

bool
vsSemaphore::Wait()
{
	if ( m_released )
		return false;

	pthread_mutex_lock(&m_semaphore.mutex);
	while ( m_value == 0 && !m_released )
	{
		pthread_cond_wait( &m_semaphore.cond, &m_semaphore.mutex );
	}
	if ( !m_released )
	{
		m_value--;
	}

	pthread_mutex_unlock(&m_semaphore.mutex);

	return !m_released;
}

void
vsSemaphore::Post()
{
	pthread_mutex_lock(&m_semaphore.mutex);
	m_value++;
	pthread_cond_signal( &m_semaphore.cond );
	pthread_mutex_unlock(&m_semaphore.mutex);
}

void
vsSemaphore::Release()
{
	if ( !m_released )
	{
		m_released = true;
		pthread_cond_broadcast(&m_semaphore.cond);
	}
}

#else

vsSemaphore::vsSemaphore(unsigned int initialValue)
{
	m_semaphore = CreateSemaphore( nullptr, initialValue, 100000, nullptr );
	m_released = false;
}

vsSemaphore::~vsSemaphore()
{
	CloseHandle( m_semaphore );
}

bool
vsSemaphore::Wait()
{
	if ( m_released )
		return false;
	WaitForSingleObject( m_semaphore, INFINITE );
	return !m_released;
}

void
vsSemaphore::Post()
{
	ReleaseSemaphore( m_semaphore, 1, nullptr );
}

void
vsSemaphore::Release()
{
	m_released = true;
	ReleaseSemaphore( m_semaphore, 1000, nullptr );
}


#endif
