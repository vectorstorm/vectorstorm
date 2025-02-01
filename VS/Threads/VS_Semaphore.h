//
//  VS_Semaphore.h
//  MMORPG2
//
//  Created by Trevor Powell on 3/08/11.
//  Copyright 2011 Trevor Powell. All rights reserved.
//

#ifndef VS_SEMAPHORE_H
#define VS_SEMAPHORE_H

#include "SDL2/SDL.h"
typedef SDL_sem* semaphore_t;

class vsSemaphore
{
	SDL_sem*		m_semaphore;
	bool m_released;
public:

	vsSemaphore(unsigned int initialValue);
	~vsSemaphore();

	// Wait until the semaphore's value is greater than zero,  until the
	// semaphore has been released.  Returns 'false' if the semaphore has been
	// released, or true if the semaphore's value is greater than zero.  Threads
	// should use this 'false' result as a signal to exit, for an orderly
	// system shutdown.
	bool Wait();

	// increment value of semaphore by one.
	void Post();

	// Release() must be called before destroying the vsSemaphore!
	//
	// This function completely releases the semaphore;  Wait() will return
	// immediately with a 'false' result for all threads waiting now on this
	// semaphore object now or in the future (even if the semaphore's value is
	// greater than 0).
	void Release();
};


#endif	//VS_SEMAPHORE_H
