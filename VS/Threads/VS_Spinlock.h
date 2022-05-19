//
//  VS_Spinlock.h
//  VectorStorm
//
//  Created by Trevor Powell on 10/10/16.
//  Copyright 2016 VectorStorm Pty Ltd. All rights reserved.
//

#ifndef VS_SPINLOCK_H
#define VS_SPINLOCK_H

// for the moment, we don't ever actually want to use spinlockes;
// we're a video game!  Let's use spinlocks instead.

#include "SDL.h"
typedef SDL_SpinLock spinlock_t;

class vsSpinlock
{
    spinlock_t   m_lock;

public:
    vsSpinlock();
    ~vsSpinlock();

    void Lock();
    void Unlock();
    bool TryLock();
};

#endif // VS_SPINLOCK_H

