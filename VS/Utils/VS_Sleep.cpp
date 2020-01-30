/*
 *  VS_Sleep.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 30/01/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Sleep.h"

#ifdef _WIN32
#else
#include <time.h>
// #include <unistd.h>
#endif

void vsSleep( int milliseconds )
{
#ifdef _WIN32
	Sleep(milliseconds);
#else
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;

	nanosleep(&ts, NULL);
#endif
}
