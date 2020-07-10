/*
 *  VS_Entity.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_ENTITY_H
#define VS_ENTITY_H

class vsDisplayList;
class vsLayer;
class vsRenderQueue;
class vsScene;
class vsSceneDraw;

#include "VS/Math/VS_Transform.h"

class vsEntity
{
protected:

	vsString		m_name;

	vsEntity *		m_next;
	vsEntity *		m_prev;

	vsEntity *		m_parent;
	vsEntity *		m_child;

	vsScene *		m_scene; // pointer to our scene IFF we're placed directly on the scene

	bool			m_visible;
	bool			m_clickable;

	bool			m_processing;
	bool			m_extractQueued;

	void			DrawChildren( vsRenderQueue *queue );

	void			DoExtract();

public:
		vsEntity();
	virtual ~vsEntity();

	virtual void	Update( float timeStep );
	virtual void	Draw( vsRenderQueue *queue );
	virtual void	DynamicDraw( vsRenderQueue *queue ) {UNUSED(queue);}

	virtual bool	OnScreen(const vsTransform2D & /*cameraTrans*/) const { return true; }

	void			RegisterOnScene(int scene);
	void			RegisterOnScene(vsScene *scene);
#if defined(_DEBUG)
	void			RegisterOnDebugScene();
#endif // _DEBUG
	void			Unregister() { Extract(); }

	void			SetVisible(bool visible) { m_visible = visible; }
	bool			GetVisible() const { return m_visible; }
	bool			IsVisible() const { return GetVisible(); }

	void			SetClickable(bool clickable) { m_clickable = clickable; }

	vsEntity *		GetNext() const { return m_next; }
	vsEntity *		GetPrev() const { return m_prev; }
	vsEntity *		GetParent() const { return m_parent; }
	vsScene *		GetScene() const { return m_scene; }

	void			Append( vsEntity *sprite );
	void			Prepend( vsEntity *sprite );
	void			AddChild( vsEntity *sprite );
	void			RemoveChild( vsEntity *sprite );

	vsEntity *		FirstChild() { return m_child; }
	vsEntity *		Sibling() { return m_next; }

	void			SetName( const vsString &name ) { m_name = name; }
	const vsString&	GetName() const { return m_name; }
	vsEntity *		Find( const vsString &name ) const;
	virtual vsEntity *	FindEntityAtPosition(const vsVector2D &pos) const;	// 'pos' is in parent coordinates!  Returns true if we swallowed the 'click' action.

	void			MoveToTop();
	void			MoveToBottom();

	void			Extract();
	void			QueueExtract();
};

#endif // VS_ENTITY_H
