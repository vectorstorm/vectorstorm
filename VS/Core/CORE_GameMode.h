/*
 *  CORE_GameMode.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef CORE_GameMode_H
#define CORE_GameMode_H

#include "CORE_GameSystem.h"

class coreGameMode : public coreGameSystem
{
public:
			coreGameMode();
	virtual ~coreGameMode();
	
	virtual void	Init();		// called when this game mode should initialise its data and get ready to run
	virtual void	Deinit();	// called when this game mode needs to shut itself down and delete its data
	
	virtual void	Update( float timeStep );
};

#endif // CORE_GameMode_H
