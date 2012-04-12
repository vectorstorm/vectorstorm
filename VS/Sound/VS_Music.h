/*
 *  SND_Music.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 9/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef SND_MUSIC_H
#define SND_MUSIC_H

#if !TARGET_OS_IPHONE
#if __APPLE_CC__
#include <SDL_mixer/SDL_mixer.h>
#else
#include "SDL_mixer.h"
#endif
#endif

class vsMusic
{
#if !TARGET_OS_IPHONE
	Mix_Music *		m_music;
#endif

	unsigned long	m_startTick;

	bool			m_looping;
	bool			m_playing;

public:

	vsMusic( const vsString &filename_in, bool looping = true );
	~vsMusic();

	void	Start();
	void	Stop();

	void	FadeOut(float time);

	float	GetTime();	 // how far through this music have we gotten?

	void	Rewind();
	void	GoToTime(float time);

	bool	IsPlaying();
	bool	IsActuallyPlaying();

	friend class vsSoundSystem;
};

#endif // SND_MUSIC_H
