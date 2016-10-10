//
//  VS_Spinlock.cpp
//  VectorStorm
//
//  Created by Trevor Powell on 10/10/16.
//  Copyright 2016 VectorStorm Pty Ltd. All rights reserved.
//

#include "VS_Spinlock.h"

vsSpinlock::vsSpinlock()
{
	m_lock = 0;

}

vsSpinlock::~vsSpinlock()
{
}

void
vsSpinlock::Lock()
{
	SDL_AtomicLock(&m_lock);
}

void
vsSpinlock::Unlock()
{
	SDL_AtomicUnlock(&m_lock);
}


bool
vsSpinlock::TryLock()
{
	return SDL_TRUE == SDL_AtomicTryLock(&m_lock);
}

