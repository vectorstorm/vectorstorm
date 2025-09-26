#ifndef __TRACYPRINT_HPP__
#define __TRACYPRINT_HPP__

// tracy doesn't care about these warnings, so exclude them.
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wunused-variable"

#ifdef TRACY_VERBOSE
#  include <stdio.h>
#  define TracyDebug(...) fprintf( stderr, __VA_ARGS__ );
#else
#  define TracyDebug(...)
#endif

#endif
