/*
 *  VS_DynamicBatchManager.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 10/07/2019
 *  Copyright 2019 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_DynamicBatchManager.h"
#include "VS_DynamicBatch.h"

vsDynamicBatchManager * vsDynamicBatchManager::s_instance = NULL;

vsDynamicBatchManager::vsDynamicBatchManager():
	m_unusedBatches(50, vsPool<vsDynamicBatch>::Type_Expandable)
{
	vsAssert(s_instance == NULL, "Multiple vsDynamicBatchManagers created??");

	s_instance = this;
}

vsDynamicBatchManager::~vsDynamicBatchManager()
{
	ResetBatches();

	vsAssert(s_instance == this, "vsDynamicBatchManager instance isn't me??");
}

vsDynamicBatch *
vsDynamicBatchManager::GetNewBatch()
{
	vsDynamicBatch *result = m_unusedBatches.Borrow();
	m_usedBatches.AddItem(result);

	return result;
}

void
vsDynamicBatchManager::FrameRendered()
{
	ResetBatches();
}

void
vsDynamicBatchManager::ResetBatches()
{
	for (int i = 0; i < m_usedBatches.ItemCount(); i++)
	{
		m_usedBatches[i]->Reset();
		m_unusedBatches.Return(m_usedBatches[i]);
	}
	m_usedBatches.Clear();
}

