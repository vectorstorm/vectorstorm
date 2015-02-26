/*
 *  SND_Sample.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 9/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_SoundSample.h"
#include "VS_SoundSystem.h"

#include "VS_File.h"

vsSoundSample::vsSoundSample(const vsString &filename_in)
{
#if !TARGET_OS_IPHONE
	const vsString &filename = vsFile::GetFullFilename(filename_in);
	m_chunk=Mix_LoadWAV(filename.c_str());
	if(!m_chunk) {
		vsLog("Mix_LoadWAV(\"%s\"): %s", filename.c_str(), Mix_GetError());
		// this might be a critical error...
	}
#endif
}

vsSoundSample::~vsSoundSample()
{
	vsSoundSystem::Instance()->CancelDeferredSounds(this);
}

void
vsSoundSample::Play()
{
	m_channel = vsSoundSystem::Instance()->PlaySound(this);
}

void
vsSoundSample::Stop()
{
	vsSoundSystem::Instance()->StopChannel(m_channel);
}

void
vsSoundSample::PlayDeferred(float fuse)
{
	vsSoundSystem::Instance()->PlaySoundDeferred(this, fuse);
}


