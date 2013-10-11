/*
 *  SND_System.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 8/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef SND_SYSTEM_H
#define SND_SYSTEM_H

#include "Core/CORE_GameSystem.h"
#include "VS/Utils/VS_Singleton.h"

class vsMusic;
class vsSoundSample;

struct sndDeferredSample
{
	vsSoundSample *	m_sample;
	float		m_fuse;		// how long until the sample gets played?
};

#define MAX_DEFERRED_SAMPLES (10)

class vsSoundSystem : public coreGameSystem
{
	static vsSoundSystem *	s_instance;

	int		m_channelCount;
	int		m_maxChannelsInUse;
	int		m_channelsInUse;

	sndDeferredSample	m_deferredSample[MAX_DEFERRED_SAMPLES];

public:

	static void ChannelFinished( int channel );

	vsSoundSystem();
	~vsSoundSystem();

	void Init();
	void Deinit();

	void InitVolume();

	virtual void	Update( float timeStep );

	void	PlayMusic( vsMusic *music );
	void	StopMusic();

	int		PlaySound( vsSoundSample *sound );
	void	StopChannel( int channel );
	void	PlaySoundDeferred( vsSoundSample *sound, float fuse );
	void	CancelDeferredSounds( vsSoundSample *sound );

	static vsSoundSystem *	Instance() { return s_instance; }
};

#endif // SND_SYSTEM_H
