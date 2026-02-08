/*
 *  Core.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 25/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef CORE_H
#define CORE_H

class coreGame;
class vsPreferences;
class vsHeap;

namespace core
{
	void			Init( size_t mainMemorySize );
	void			Deinit();

	void			SetGame( coreGame *game );		// this sets the game which will be activated NEXT FRAME.
	coreGame *	GetGame( );						// get the current game
	const vsString &		GetMainGameName( );				// get the name of the main game
	const vsString &		GetGameName( );					// get the name of the current game

	void PreGoOneFrame();

	void			Go();
	void			GoOneFrame( float timeStep );

	void			SetExitToMenu();	// exit to the main menu
	void			SetExit();			// exit program entirely

	bool			IsExitRequested();			// exit program entirely

	void SetAllowExit(bool allow); // if set to 'false', we'll defer exiting until it's set back to true
};

#endif //CORE_H
