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
#include "VS/Graphics/VS_DisplayList.h"
#include "VS/Utils/VS_ArrayStore.h"

class vsFragment
{
public:
	enum SimpleType
	{
		SimpleType_TriangleList,
		SimpleType_TriangleFan,
		SimpleType_TriangleStrip,
	};
private:


	vsMaterial *					m_material;
	vsDisplayList *					m_displayList;

	vsArrayStore<vsRenderBuffer>	m_bufferList;
	bool m_visible;

	SimpleType m_simpleType;
	vsRenderBuffer *m_vbo;
	vsRenderBuffer *m_ibo;

public:

	static vsFragment *	Load( vsRecord *record );

	vsFragment();
	virtual ~vsFragment();

	void	SetVisible(bool visible) { m_visible = visible; }
	void	SetMaterial( vsMaterial *material );
	void	SetMaterial( const vsString &name );
	void	SetDisplayList( vsDisplayList *list ) { m_displayList = list; m_vbo = NULL; m_ibo = NULL; }
	void	AddBuffer( vsRenderBuffer *buffer );
	void	Clear();

	// A "Simple" fragment has no display list;  it just binds the vbo and
	// draws the ibo as a triangle list.
	void	SetSimple( vsRenderBuffer *vbo, vsRenderBuffer *ibo, SimpleType type );
	bool	IsSimple() const { return m_vbo && m_ibo; }
	SimpleType GetSimpleType() const { return m_simpleType; }
	vsRenderBuffer * GetSimpleVBO() { return m_vbo; }
	vsRenderBuffer * GetSimpleIBO() { return m_ibo; }

	bool			IsVisible() const { return m_visible; }
	vsMaterial *	GetMaterial() { return m_material; }
	vsDisplayList *	GetDisplayList() { return m_displayList; }
	const vsDisplayList *	GetDisplayList() const { return m_displayList; }
	int	GetTriangles(vsArray<struct vsDisplayList::Triangle>& result) const;

	void	Draw( vsDisplayList *list );
};

#endif // VS_FRAGMENT_H

