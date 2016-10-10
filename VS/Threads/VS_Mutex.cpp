//
//  VS_Mutex.cpp
//  VectorStorm
//
//  Created by Trevor Powell on 29/03/11.
//  Copyright 2011 VectorStorm Pty Ltd. All rights reserved.
//

#include "VS_Mutex.h"

vsMutex::vsMutex()
{
	m_mutex = SDL_CreateMutex();

}

vsMutex::~vsMutex()
{
	SDL_DestroyMutex(m_mutex);
}

void
vsMutex::Lock()
{
	SDL_LockMutex(m_mutex);
}

void
vsMutex::Unlock()
{
	SDL_UnlockMutex(m_mutex);
}

bool
vsMutex::TryLock()
{
	return SDL_TRUE == SDL_TryLockMutex(m_mutex);
}

