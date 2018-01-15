/*
 *  VS_Menu.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 29/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MENU_H
#define VS_MENU_H

#ifdef VS_DEFAULT_VIRTUAL_CONTROLLER

#include "VS/Graphics/VS_Font.h"
#include "VS/Graphics/VS_Sprite.h"

	/** vsSimpleMenu
	 * Utility class for rendering and interacting with menus.
	 * Instantiate and register as normal for a vsEntity-type class.
	 *
	 */

class vsSimpleMenuAction
{
public:

	enum Type
	{
		Type_None,
		Type_Select,
		Type_Cancel,
		Type_Left,
		Type_Right
	};

	Type	type;
	int		menuItemId;

	vsSimpleMenuAction() { Clear(); }

	void Clear() { type = Type_None; menuItemId = -1; }
	void Select(int id) { type = Type_Select; menuItemId = id; }
	void Cancel() { type = Type_Cancel; }
	void Left(int id) { type = Type_Left; menuItemId = id; }
	void Right(int id) { type = Type_Right; menuItemId = id; }
};

class vsSimpleMenu : public vsSprite
{
	typedef vsSprite Parent;

	vsSprite **				m_itemLabel;
	vsSprite **				m_itemValue;
	int						m_itemCount;

	int						m_highlightedId;	// currently highlightd item
	vsSimpleMenuAction			m_action;

	float					m_pulseTimer;	// the currently highlightd item pulses, to let us know which one is selected right now.

	float					m_letterSize;
	float					m_capSize;
	float					m_lineSpacing;


	void					ArrangeItems();

public:

							vsSimpleMenu(int count, float letterSize = 35.0f, float capSize = 45.0f, float lineSpacing = 20.0f);	// how many menu options in this menu?
	virtual					~vsSimpleMenu();

	void					Enter() { m_highlightedId = 0; }

	void					SetItemCount(int count);

	virtual void			Update(float timeStep);
	virtual void			Draw( vsRenderQueue *queue );

	int						GetHighlightedItem() { return m_highlightedId; }

	bool					WasActionTaken() { return (GetAction().type != vsSimpleMenuAction::Type_None); }
	const vsSimpleMenuAction &	GetAction() { return m_action; }

	void					SetItemLabel( int itemId, const vsString & label );
	void					SetItemValue( int itemId, const vsString & value );
};
#endif // VS_DEFAULT_VIRTUAL_CONTROLLER

#endif // VS_MENU_H

