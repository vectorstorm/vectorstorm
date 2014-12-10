/*
 *  CORE_GameRegistry.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 25/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */


#ifndef CORE_GAMEREGISTRY_H
#define CORE_GAMEREGISTRY_H

class coreGame;

#define REGISTER_GAME(name,game)  static coreGameEntry thisEntry(vsString(name),new game, false)
#define REGISTER_MAINGAME(name,game) static coreGameEntry thisEntry(vsString(name),new game, true)

class coreGameEntry
{
#define MAX_GAME_NAME_LENGTH (32)
public:
	vsString	m_name;
	coreGame *	m_game;

	coreGameEntry( const vsString &name, coreGame *game, bool mainGame );
};

class coreGameRegistry
{
#define MAX_GAMES (10)

	static	coreGameEntry *	s_mainMenu;

	static	coreGameEntry *	s_game[MAX_GAMES];
	static  int				s_gameEntries;
public:

	static coreGame *	GetMainMenu() { return s_mainMenu->m_game; }

	static void			RegisterGame( coreGameEntry *entry, bool mainGame );
	static coreGame *	FindGame( const vsString &name );
	static coreGame *	GetGame(int i);
	static const vsString &	GetGameName(int i);
	static const vsString &	GetGameName( coreGame *game );
	static const vsString &	GetMainGameName();

	static int			GetGameCount() { return s_gameEntries; }
};

#endif // CORE_GAMEREGISTRY_H
