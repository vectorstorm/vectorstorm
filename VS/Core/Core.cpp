/*
 *  Core.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 25/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "Core.h"
#include "CORE_Game.h"
#include "CORE_GameRegistry.h"

// TODO:  Need this #include so that the 'main' function below gets correctly macroed for SDLMain.o linking.
//  this is really ugly;  we don't want to have SDL stuff anywhere in game code, so I eventually need to
//  figure out how to push this somewhere into vsSystem.  Maybe vsSystem has 'main', and that kickstarts Core?
//  except that's really just as bad.. the engine shouldn't be starting the gameside code.  Hm..  maybe trickery
//  with the prefix file or something, to at least get the SDL #include into platform or middleware #ifdefs or
//  something? :/
#if !TARGET_OS_IPHONE
#include <SDL2/SDL.h>
#endif

#include "Memory/VS_Heap.h"

#include "VS/Utils/VS_Preferences.h"
#include "VS/Math/VS_Random.h"
#include "VS/Graphics/VS_Screen.h"
#include "VS/Utils/VS_System.h"



coreGame		*	core::s_game = NULL;
coreGame		*	core::s_nextGame = NULL;
vsHeap			*	core::s_gameHeap = NULL;
bool				core::s_exit = false;
bool				core::s_allowExit = true;

// hack variable, to make the game run fullscreen on my particular laptop.
// TODO:  This should really be built into an 'options' screen somewhere,
//   which should also set the fullscreen resolution properly.
//bool		gFullScreen = false;

void
core::Init( size_t mainMemorySize )
{
	vsSystem::Instance()->Init();

	s_gameHeap = new vsHeap("game",mainMemorySize);
}

void
core::Deinit()
{
	vsSystem::Instance()->Deinit();

	vsDelete(s_gameHeap);
}

/**
 * core::SetGame
 *
 *   Marks a different game as becoming active.  The transition to the new game won't happen immediately,
 *   but will be delayed until the next frame's update cycle.  This is to protect against any weirdnesses
 *   which could be caused by a game transition occurring during the middle of an update or drawing operation.
 */

void
core::SetGame( coreGame *game )
{
	s_nextGame = game;
}

/**
 * core::GetGame
 *
 *  Returns the currently active game.  Note that if this is called immediately after 'SetGame', it will
 *  not return the newly set game;  there is a one-update delay between a game being set and when it actually
 *  becomes active!
 */
coreGame *
core::GetGame()
{
	return s_game;
}

/**
 *  core::Go
 *
 *    This is our high-level game-running logic.  It brings new games in and out, as requested.
 *    It also initiates memory validation checks between games, and prints out game statistics at that time.
 */

void
core::Go()
{
	// Create the basic game systems which are shared among all games.  (Physics, input, etc)
	coreGame::CreateGameSystems();

	// In theory, everything persistant has now been allocated.  Anything allocated
	// from now on will be allocated by an individual game inside our special s_gameHeap,
	// and will need to be deallocated by that game before it exits.  Tell our game-only
	// vsHeap that it's at its position for leak testing.
	vsHeap::Push(s_gameHeap);
	s_gameHeap->SetMarkForLeakTesting();

	// 's_exit' is set when a game requests the whole VectorStorm application exit.
	// Usually this is done by the coreGame watching for the 'esc' button, but
	// any game can also set this value manually, by calling core::SetExit().
	while ( !s_exit || !s_allowExit )
	{
		if ( s_nextGame )	// if we've marked a game to switch to
		{
			if ( s_game )		// if we're already running a game
			{
				s_game->StopTimer();	// stop gathering game stats first, so we don't
				s_game->Deinit();		// penalise a game's average FPS for how long their Deinit() takes.

				s_gameHeap->CheckForLeaks();		// verify that game actually deleted everything it allocated
			}

			s_gameHeap->PrintStatus();			// print the current memory stats to our log

			s_game = s_nextGame;							// activate the new game, and start its profiling timers.
			s_game->Init();
			s_game->StartTimer();

			s_nextGame = NULL;
		}
		s_game->Go();		// run a frame of the current game.
	}

	if ( s_game )		// if we're already running a game
	{
		s_game->StopTimer();	// stop gathering game stats first, so we don't
		s_game->Deinit();		// penalise a game's average FPS for how long their Deinit() takes.

		s_gameHeap->CheckForLeaks();		// verify that game actually deleted everything it allocated
	}

	s_gameHeap->PrintStatus();	// print the current memory stats to our log

	vsHeap::Pop(s_gameHeap);	// pop our gameHeap back off the stack.

	coreGame::DestroyGameSystems();
}

void
core::PreGoOneFrame()
{
	// Create the basic game systems which are shared among all games.  (Physics, input, etc)
	coreGame::CreateGameSystems();

	// In theory, everything persistant has now been allocated.  Anything allocated
	// from now on will be allocated by an individual game inside our special s_gameHeap,
	// and will need to be deallocated by that game before it exits.  Tell our game-only
	// vsHeap that it's at its position for leak testing.
	vsHeap::Push(s_gameHeap);
	s_gameHeap->SetMarkForLeakTesting();

}

void
core::GoOneFrame( float timeStep )
{
	// 's_exit' is set when a game requests the whole VectorStorm application exit.
	// Usually this is done by the coreGame watching for the 'esc' button, but
	// any game can also set this value manually, by calling core::SetExit().
	//	while ( !s_exit )
	{
		if ( s_nextGame )	// if we've marked a game to switch to
		{
			if ( s_game )		// if we're already running a game
			{
				s_game->StopTimer();	// stop gathering game stats first, so we don't
				s_game->Deinit();		// penalise a game's average FPS for how long their Deinit() takes.

				s_gameHeap->CheckForLeaks();		// verify that game actually deleted everything it allocated
			}

			s_gameHeap->PrintStatus();			// print the current memory stats to our log

			s_game = s_nextGame;							// activate the new game, and start its profiling timers.
			s_game->Init();
			s_game->StartTimer();

			s_nextGame = NULL;
		}
		s_game->SetTimeStep( timeStep );
		s_game->Go();		// run a frame of the current game.
	}
}


/**
 * core::SetExitToMenu()
 *
 *   Called by client games to request that VectorStorm return to the game selection menu.
 *     (TODO:  Shouldn't be special case checking for the Main Menu setting this;  the
 *      Main Menu should call the correct function for what it actually wants to do)
 *
 *   The exit to the main menu will be executed AFTER the currently executing update cycle
 *   completes.
 */

void
core::SetExitToMenu()
{
	if ( s_game == coreGameRegistry::GetMainMenu() )	// if we're already at the main menu, then go ahead and exit
		SetExit();
	else
		SetGame( coreGameRegistry::GetMainMenu() );
}

/**
 * core::SetExit()
 *
 *   Called by client games to request that VectorStorm quit.
 *
 *   Quitting will occur AFTER the currently executing update cycle completes.
 */

void
core::SetExit()
{
	s_exit = true;
}

void
core::SetAllowExit(bool allow)
{
	s_allowExit = allow;
}

const vsString &
core::GetMainGameName()
{
	return coreGameRegistry::GetMainGameName();
}

/**
 * core::GetGameName()
 *
 *   Returns the name of the currently active game as a const vsString reference.
 */

const vsString &
core::GetGameName()
{
	return coreGameRegistry::GetGameName(s_game);
}
