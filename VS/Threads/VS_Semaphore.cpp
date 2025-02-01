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

	if ( 0 == SDL_SemWait( m_semaphore ) )
		return true;

	vsLog("Semaphore wait failed??");
	return false;
}

void
vsSemaphore::Post()
{
	int result = SDL_SemPost( m_semaphore );

	if ( 0 != result )
		vsLog("Error in SDL_SemPost!");
}

void
vsSemaphore::Release()
{
	// bah.  I wish it was possible to tell it to increment the semaphore value
	// by a constant amount instead of having to do it one by one.
	m_released = true;
	for ( int i = 0; i < 1000; i++ )
		SDL_SemPost( m_semaphore );
}

