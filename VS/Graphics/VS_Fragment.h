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
class vsVertexArrayObject;

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
	vsVertexArrayObject *			m_vao;

	vsArrayStore<vsRenderBuffer>	m_bufferList;
	bool m_visible;

	SimpleType m_simpleType;
	vsRenderBuffer *m_simpleVbo; // for simple renders, here's our vertex buffer (may legally be null)
	vsRenderBuffer *m_simpleIbo; // for simple renders, here's our index buffer. (required)

public:

	enum SimpleBufferOwnershipType
	{
		Owned_None = 0,
		Owned_VBO = BIT(0),
		Owned_IBO = BIT(1),
		Owned_Both = BIT(0)|BIT(1)
	};

	static vsFragment *	Load( vsRecord *record );

	vsFragment();
	virtual ~vsFragment();

	void	SetVisible(bool visible) { m_visible = visible; }
	void	SetMaterial( vsMaterial *material );
	void	SetMaterial( const vsString &name );
	void	SetDisplayList( vsDisplayList *list ) { m_displayList = list; m_simpleVbo = nullptr; m_simpleIbo = nullptr; }
	void	AddBuffer( vsRenderBuffer *buffer );
	void	Clear();

	vsVertexArrayObject *GetVAO() { return m_vao; }

	// A "Simple" fragment has no display list;  it just binds the vbo and
	// draws the ibo as a triangle list.
	void	SetSimple( vsRenderBuffer *vbo, vsRenderBuffer *ibo, SimpleType type, SimpleBufferOwnershipType ownershipType = Owned_Both );
	bool	IsSimple() const { return m_simpleIbo; }
	SimpleType GetSimpleType() const { return m_simpleType; }
	vsRenderBuffer * GetSimpleVBO() { return m_simpleVbo; }
	vsRenderBuffer * GetSimpleIBO() { return m_simpleIbo; }

	bool			IsVisible() const { return m_visible; }
	vsMaterial *	GetMaterial() { return m_material; }
	vsDisplayList *	GetDisplayList() { return m_displayList; }
	const vsDisplayList *	GetDisplayList() const { return m_displayList; }
	vsBox3D GetBoundingBox() const;
	int	GetTriangles(vsArray<struct vsDisplayList::Triangle>& result) const;

	void	Draw( vsDisplayList *list );
};

#endif // VS_FRAGMENT_H

