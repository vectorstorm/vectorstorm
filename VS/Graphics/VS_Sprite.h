/*
 *  VS_Sprite.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_SPRITE_H
#define VS_SPRITE_H

#include "VS/Graphics/VS_Color.h"
#include "VS/Graphics/VS_DisplayList.h"
#include "VS/Graphics/VS_Entity.h"
#include "VS/Graphics/VS_Fragment.h"
#include "VS/Graphics/VS_Material.h"

#include "VS/Math/VS_Transform.h"
#include "VS/Math/VS_Box.h"

#include "VS/Utils/VS_ArrayStore.h"



class vsOverlay;

class vsSprite : public vsEntity
{
	typedef vsEntity Parent;

protected:
	vsDisplayList		*m_displayList;			// old-style rendering

	vsArrayStore<vsFragment>	m_fragment;	// new-style rendering

	vsBox2D				m_boundingBox;
	float				m_boundingRadius;
	vsColor				m_color;

	vsOverlay *			m_overlay;

	vsMaterial *		m_material;

	bool				m_useColor;
	bool				m_boundingBoxLocked;

public:
	vsTransform2D		m_transform;

	static vsSprite *	Load(const vsString &filename);

						vsSprite( vsDisplayList *displayList = NULL );
	virtual				~vsSprite();

	void				LoadFrom( vsRecord *record );

	void				SetColor( vsColor c ) { m_color = c; m_useColor = true; }
	void				SetOverlay( vsOverlay *o ) { m_overlay = o; }
	void				SetMaterial( const vsString &name ) { vsDelete( m_material ); m_material = new vsMaterial(name); }
	void				SetMaterial( vsMaterial *mat ) { vsDelete( m_material ); m_material = mat; }

	virtual void		SetPosition( const vsVector2D &pos ) { m_transform.SetTranslation( pos ); }
	const vsVector2D &	GetPosition() { return m_transform.GetTranslation(); }
	virtual void		SetAngle( const vsAngle &angle ) { m_transform.SetAngle( angle ); }
	const vsAngle &		GetAngle() { return m_transform.GetAngle(); }
	void				SetScale( float scale ) { m_transform.SetScale( vsVector2D(scale,scale) ); }
	void				SetScale( const vsVector2D &scale ) { m_transform.SetScale( scale ); }
	const vsVector2D &	GetScale() { return m_transform.GetScale(); }

	vsVector2D			GetWorldPosition();	// try to get our world position.  Note that this will only work if all of our parents only adjust the draw transformation using vsSprite built-in transforms!

	void				SetDisplayList( vsDisplayList *list );
	void				AddFragment( vsFragment *fragment );
	void				DestroyFragment( vsFragment *fragment );
	void				ClearFragments();
	vsFragment *		GetFragment(int i) { return m_fragment[i]; }
	size_t				GetFragmentCount() { return m_fragment.ItemCount(); }

	void				SetBoundingBox( const vsBox2D &box ) { m_boundingBox = box; }
	const vsBox2D &		GetBoundingBox() { return m_boundingBox; }
	void				BuildBoundingBox();
	void				CalculateBoundingRadius();
	float				GetBoundingRadius() { return m_boundingRadius; }

	virtual vsEntity *	FindEntityAtPosition(const vsVector2D &pos);	// 'pos' is in parent coordinates!  Returns true if we swallowed the 'click' action.
	virtual bool		ContainsLocalPoint(const vsVector2D &pos);

	void				Rotate( float angle );

	virtual bool		OnScreen(const vsTransform2D & cameraTrans);

	virtual void		Draw( vsRenderQueue *queue );
};

#endif // VS_SPRITE_H
