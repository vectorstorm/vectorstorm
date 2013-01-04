/*
 *  VS_Debug.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/05/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_DEBUG_H
#define VS_DEBUG_H


#if defined(ENABLE_ASSERTS)
#include "VS_String.h"
#include "VS_Log.h"

#define vsAssert(x,y) if(!(x)){ vsFailedAssert(#x, y,__FILE__, __LINE__); }

void vsFailedAssert( const vsString &conditionStr, const vsString &msg, const char *file, int line );

class vsErrorContext
{
public:
    vsErrorContext(const char *name, const char *data);
	~vsErrorContext();
};

#else
#define vsAssert(x,y) {}
#endif


#endif // VS_DEBUG_H

