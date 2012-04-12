/*
 *  CORE_Game.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef CORE_GAME_H
#define CORE_GAME_H


enum GameSystem
{
	GameSystem_Timer,			// this system caps our frame rate
	GameSystem_Input,			// this system reads input devices
	GameSystem_Collision,		// this system performs collision tests
	GameSystem_Sound,			// this system performs sound mixing
	GameSystem_MAX
};

class coreGameMode;
class coreGameSystem;
class vsInput;
class vsCollisionSystem;
class vsSoundSystem;
class vsTimerSystem;

class coreGame
{
	static coreGameSystem	*s_system[GameSystem_MAX];		// store the game systems shared by client games.

	coreGameSystem			*m_system[GameSystem_MAX];		// An alternate game system execution order can be requested by client games, by filling out this array.
	int						m_systemCount;					// how many game systems are actually active for this game.

	coreGameMode			*m_currentMode;					// optional;  the current game mode active, if this coreGame uses them.

	unsigned long			m_framesRendered;
	unsigned long			m_startTicks;

	float					m_timeStep;
	bool					m_exit;
	bool					m_firstFrame;

	//static coreGame			*s_instance;

	virtual void			ConfigureGameSystems();

	void					InitGameSystems();
	void					DeinitGameSystems();

protected:
	int						m_sceneCount;					// how many scenes each particular game wishes to use.

public:
							coreGame();
	virtual					~coreGame();

	void					SetGameMode( coreGameMode *newMode );

	static	void			CreateGameSystems();

	virtual void			Init();
	virtual void			Deinit();

	void					StartTimer();	// start our internal timing, after init
	void					StopTimer();	// finish our internal timing, before deinit

	void					Go();

	virtual void			Update( float timeStep ) {UNUSED(timeStep);}

	void					SetExit() { m_exit = true; }
	void					SetTimeStep(float timeStep) { m_timeStep = timeStep; }

	vsInput *				GetInput();
	vsCollisionSystem *				GetCollision();
	vsSoundSystem *				GetSound();
	vsTimerSystem *				GetTimer();
};

#endif // CORE_GAME_H
