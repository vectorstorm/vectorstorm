/*
 *  SND_Music.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 9/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Music.h"
#include "VS_File.h"

#include "VS_TimerSystem.h"

#if !TARGET_OS_IPHONE
#include "SDL.h"
#endif

int music_buffer_pos=-1;
int decrypted_bytes =-1;

vsMusic::vsMusic( const vsString &filename_in, bool looping )
{
#if !TARGET_OS_IPHONE
	const vsString &filename = vsFile::GetFullFilename(filename_in);

	m_music=Mix_LoadMUS(filename.c_str());
	if(!m_music) {
		vsLog("Mix_LoadMUS(\"%s\"): %s", filename.c_str(), Mix_GetError());
		// this might be a critical error...
	}
#endif
	m_playing = false;
	m_looping = looping;
}

vsMusic::~vsMusic()
{
	Stop();
#if !TARGET_OS_IPHONE
	Mix_FreeMusic(m_music);
#endif
}

void
vsMusic::Start()
{
#if !TARGET_OS_IPHONE
	int loops = (m_looping)?-1:1;
	if ( m_playing )
		Mix_HaltMusic();
	if(Mix_PlayMusic(m_music, loops)!=-1)
	{
		m_playing = true;

		Mix_RewindMusic();	// this is stupid, but it's the only way I've found to
		Mix_RewindMusic();	// make SDL_Mixer start playing at a reliable place and time.
		Mix_RewindMusic();	// Why three calls?  I don't know.  But it works, as a hack.  :(
							// (Without this, there was up to a 1 second uncertainty in when music actually started)

		m_startTick = vsTimerSystem::Instance()->GetMicroseconds();
	}
	else
		vsLog("Mix_PlayMusic: %s", Mix_GetError()); // well, there's no music, but most games don't break without music, so don't assert.
#endif
}

void
vsMusic::Stop()
{
	if ( m_playing )
	{
#if !TARGET_OS_IPHONE
		Mix_HaltMusic();
#endif
		m_playing = false;
	}
}

void
vsMusic::FadeOut( float time )
{
#if !TARGET_OS_IPHONE
	Mix_FadeOutMusic((int)(time * 1000.f));
#endif
}

float
vsMusic::GetTime()
{
	float result = 0.f;
	if ( m_playing )
	{
		unsigned long deltaMicros = vsTimerSystem::Instance()->GetMicroseconds() - m_startTick;

		result = deltaMicros / 1000000.0f;
	}

	return result;
}

void
vsMusic::Rewind()
{
	if ( m_playing )
	{
#if !TARGET_OS_IPHONE
		Mix_RewindMusic();
#endif
		m_startTick = vsTimerSystem::Instance()->GetMicroseconds();
	}
}

void
vsMusic::GoToTime(float time)
{
	UNUSED(time);
	if ( m_playing )
	{
		Stop();
		//bool isMp3 = (Mix_GetMusicType(m_music) == MUS_MP3);
		//if ( isMp3 )
		//	Rewind();
		//Mix_SetMusicPosition(time);
#if !TARGET_OS_IPHONE
		int loops = (m_looping)?-1:1;
		Mix_FadeInMusicPos( m_music, loops, 0, time );
#endif
		m_playing = true;
	}
}

bool
vsMusic::IsPlaying()
{
	return m_playing;
}

bool
vsMusic::IsActuallyPlaying()
{
#if !TARGET_OS_IPHONE
	return (m_playing && Mix_PlayingMusic());
#else
	return false;
#endif
}

