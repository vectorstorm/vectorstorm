//
//  VS_Semaphore.cpp
//  MMORPG2
//
//  Created by Trevor Powell on 3/08/11.
//  Copyright 2011 Trevor Powell. All rights reserved.
//

#include "VS_Semaphore.h"

vsSemaphore::vsSemaphore(unsigned int initialValue):
	m_semaphore( SDL_CreateSemaphore(initialValue) ),
	m_released( false )
{
}

vsSemaphore::~vsSemaphore()
{
	SDL_DestroySemaphore(m_semaphore);
}

bool
vsSemaphore::Wait()
{
	if ( m_released )
		return false;

	SDL_WaitSemaphore( m_semaphore );
	return true;
}

void
vsSemaphore::Post()
{
	SDL_SignalSemaphore( m_semaphore );
}

void
vsSemaphore::Release()
{
	// bah.  I wish it was possible to tell it to increment the semaphore value
	// by a constant amount instead of having to do it one by one.
	m_released = true;
	for ( int i = 0; i < 1000; i++ )
		SDL_SignalSemaphore( m_semaphore );
}

