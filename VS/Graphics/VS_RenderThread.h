/*
 *  VS_RenderThread.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 11/12/2017
 *  Copyright 2017 Trevor Powell.  All rights reserved.
 *
 */

#include "Utils/VS_Task.h"

#ifndef VS_RENDERTHREAD_H
#define VS_RENDERTHREAD_H

class vsRenderThread: public vsTask
{
	static vsRenderThread *s_instance;
protected:
	virtual int Run();
public:

	vsRenderThread();
	virtual ~vsRenderThread();

	static vsRenderThread* Instance() { return s_instance; }
};

#endif // VS_RENDERTHREAD_H

