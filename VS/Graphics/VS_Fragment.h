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

	vsRenderBuffer *				m_soleVertexBuffer;
	vsLinkedListStore<vsRenderBuffer>		m_bufferList;

public:

	static vsFragment *	Load( vsRecord *record );

	vsFragment();
	~vsFragment();

	void	SetMaterial( vsMaterial *material );
	void	SetMaterial( const vsString &name );
	void	SetDisplayList( vsDisplayList *list ) { m_displayList = list; }

	/**
	 * SetSoleVertexBuffer states that the passed render buffer is the only one
	 * containing vertex data which will be bound for the attached display list.
	 *
	 * If this is set on a fragment, then the vertex buffer will be bound before
	 * the display list is executed.
	 *
	 * VectorStorm rendering can then batch up multiple fragments which use the
	 * same material and the same soleVertexBuffers, to try to minimise buffer
	 * state changes.
	 *
	 * The passed buffer does NOT become owned by the fragment, and must be
	 * cleaned up by someone else.  (This is required, or else multiple
	 * fragments each using the same buffer would result in multiple deletions
	 * of the one buffer)
	 */
	void	SetSoleVertexBuffer( vsRenderBuffer *buffer );

	/**
	 * Adds a buffer which becomes owned by this fragment.  When the fragment is
	 * destroyed, it will automatically destroy the passed buffer.
	 */
	void	AddBuffer( vsRenderBuffer *buffer );

	vsMaterial *	GetMaterial() { return m_material; }
	vsDisplayList *	GetDisplayList() { return m_displayList; }
	vsRenderBuffer *GetSoleVertexBuffer() { return m_soleVertexBuffer; }

	void	Draw( vsDisplayList *list );
};

#endif // VS_FRAGMENT_H

