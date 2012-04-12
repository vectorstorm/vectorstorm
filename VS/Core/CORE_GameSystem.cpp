/*
 *  CORE_GameSystem.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "CORE_GameSystem.h"

coreGameSystem::coreGameSystem():
	m_active(false)
{
}

coreGameSystem::~coreGameSystem()
{
}

void
coreGameSystem::Init()
{
}

void
coreGameSystem::Deinit()
{
}

void
coreGameSystem::Update( float timeStep )
{
	UNUSED(timeStep);
}

void
coreGameSystem::PostUpdate( float timeStep )
{
	UNUSED(timeStep);
}
