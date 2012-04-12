/*
 *  CORE_GameSystem.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 18/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef CORE_GAMESYSTEM_H
#define CORE_GAMESYSTEM_H

class coreGameSystem
{
	bool	m_active;
	
public:
						coreGameSystem();
	virtual				~coreGameSystem();
	
	virtual void		Init();
	virtual void		Deinit();
	
	virtual void		Update(float timeStep);
	virtual void		PostUpdate(float timeStep);
	
	void				SetActive( bool active ) { m_active = active; }
	void				Activate( bool active = true ) { SetActive(active); }
	void				Deactivate() { Activate(false); }
	bool				IsActive() { return m_active; }
};

#endif // CORE_GAMESYSTEM_H

