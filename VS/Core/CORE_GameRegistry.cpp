/*
 *  CORE_GameRegistry.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 25/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "CORE_GameRegistry.h"

coreGameEntry *					coreGameRegistry::s_mainMenu = NULL;
coreGameEntry *					coreGameRegistry::s_game[MAX_GAMES];
int								coreGameRegistry::s_gameEntries = 0;

coreGameEntry::coreGameEntry( const vsString & name, coreGame *game, bool mainGame )
{
	m_name = name;
	m_game = game;

	coreGameRegistry::RegisterGame(this, mainGame);
}


void
coreGameRegistry::RegisterGame( coreGameEntry *entry, bool mainGame )
{
	vsAssert( s_gameEntries < MAX_GAMES, "Too many games registered!" );	// if this assert fires, we need to increase the value of MAX_GAMES

	if ( mainGame )
	{
		vsAssert( s_mainMenu == NULL, "Too many main games registered!" );
		s_mainMenu = entry;
	}
	else
	{
		// insertion sort our entry into place.

		int index = 0;	// store which index the game should be sorted into

		for ( index = 0; index < s_gameEntries; index++ )
		{
			if ( entry->m_name < s_game[index]->m_name )
				break;
		}

		// push all later games further down our list
		for ( int i = s_gameEntries; i > index; i-- )
			s_game[i] = s_game[i-1];

		// store this game in its sorted location
		s_game[index] = entry;
		s_gameEntries++;
	}
}

coreGame *
coreGameRegistry::FindGame( const vsString &name )
{
	for ( int i = 0; i < s_gameEntries; i++ )
	{
		if ( name == s_game[i]->m_name )
			return s_game[i]->m_game;
	}

	return NULL;
}

coreGame *
coreGameRegistry::GetGame( int i )
{
	vsAssert( i >= 0 && i < s_gameEntries, "Illegal GameID requested!" );

	return s_game[i]->m_game;
}

const vsString &
coreGameRegistry::GetGameName( int i )
{
	vsAssert( i >= 0 && i < s_gameEntries, "Illegal GameID requested!" );

	return s_game[i]->m_name;
}

static vsString empty;

const vsString &
coreGameRegistry::GetGameName( coreGame *game )
{
	for ( int i = 0; i < s_gameEntries; i++ )
		if ( s_game[i]->m_game == game )
			return s_game[i]->m_name;

	// if we're passed NULL, or anything else we don't know about,
	// just report the name of our main game.
	return s_mainMenu->m_name;
}

const vsString &
coreGameRegistry::GetMainGameName()
{
	return s_mainMenu->m_name;
}

