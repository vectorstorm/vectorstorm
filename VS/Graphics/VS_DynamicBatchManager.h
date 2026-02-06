/*
 *  VS_DynamicBatchManager.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 10/07/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_DYNAMICBATCHMANAGER_H
#define VS_DYNAMICBATCHMANAGER_H

#include "VS/Utils/VS_Array.h"
#include "VS/Utils/VS_Pool.h"
class vsDynamicBatch;

class vsDynamicBatchManager
{
	static vsDynamicBatchManager *	s_instance;

	vsPool<vsDynamicBatch> m_unusedBatches;
	vsArray<vsDynamicBatch*> m_usedBatches;

	void ResetBatches();
public:
	static vsDynamicBatchManager* Instance() { return s_instance; }
	static bool Exists() { return s_instance != nullptr; }

	vsDynamicBatchManager();
	~vsDynamicBatchManager();

	vsDynamicBatch * GetNewBatch();
	void FrameRendered();
};


#endif // VS_DYNAMICBATCHMANAGER_H

