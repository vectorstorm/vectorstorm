/*
 *  SND_Sample.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 9/04/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef SND_SAMPLE_H
#define SND_SAMPLE_H

#if !TARGET_OS_IPHONE
#include <SDL2/SDL_mixer.h>
#endif

class vsSoundSample
{
#if !TARGET_OS_IPHONE
	Mix_Chunk *	m_chunk;
#endif

	int			m_channel;

public:

	vsSoundSample( const vsString &filename_in );
	~vsSoundSample();

	void		Play();
	void		Stop();	// stops me if I'm playing.
	void		PlayDeferred( float fuse );

	friend class vsSoundSystem;
};

#endif // SND_SAMPLE_H
