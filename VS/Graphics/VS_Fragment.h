/*
 *  VS_Fragment.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 1/08/10.
 *  Copyright 2010 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_FRAGMENT_H
#define VS_FRAGMENT_H

class vsRecord;
class vsDisplayList;

#include "VS/Graphics/VS_RenderBuffer.h"
#include "VS/Graphics/VS_Material.h"
#include "VS/Utils/VS_LinkedListStore.h"

class vsFragment
{
	vsMaterial *					m_material;
	vsDisplayList *					m_displayList;

	vsLinkedListStore<vsRenderBuffer>		m_bufferList;

public:

	static vsFragment *	Load( vsRecord *record );

	vsFragment();
	virtual ~vsFragment();

	void	SetMaterial( vsMaterial *material );
	void	SetMaterial( const vsString &name );
	void	SetDisplayList( vsDisplayList *list ) { m_displayList = list; }
	void	AddBuffer( vsRenderBuffer *buffer );
	void	Clear();

	vsMaterial *	GetMaterial() { return m_material; }
	vsDisplayList *	GetDisplayList() { return m_displayList; }

	void	Draw( vsDisplayList *list );
};

#endif // VS_FRAGMENT_H

