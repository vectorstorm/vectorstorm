/*
 *  COL_Sprite.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/04/07.
 *  Copyright 2007 PanicKitten Softworks. All rights reserved.
 *
 */

#ifndef COL_SPRITE_H
#define COL_SPRITE_H

#include "VS/Physics/VS_CollisionObject.h"
#include "VS/Graphics/VS_Sprite.h"

class vsCollisionSprite : public vsSprite, public vsCollisionResponder
{
	typedef vsSprite Parent;
protected:
	colObject *		m_colObject;
public:
	static vsCollisionSprite * Load(const vsString &filename, int colFlags, int testFlags);

	vsCollisionSprite( vsDisplayList *list, int colFlags, int testFlags );
	virtual ~vsCollisionSprite();

	void			SetCollisionsActive(bool active) { m_vsCollisionObject->SetCollisionsActive(active); }

	void			Teleport();
	void			SetDestroyed() { m_colObject->SetDestroyed(); }

	virtual bool	CollisionCallback( const colEvent &collision );
	virtual void	DestroyCallback();

	virtual void	Update( float timeStep );
};


#endif // COL_SPRITE_H
