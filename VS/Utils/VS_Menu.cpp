/*
 *  UT_Menu.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 29/12/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Menu.h"

#include "VS_DisplayList.h"
#include "VS_BuiltInFont.h"
#include "VS_Sprite.h"

#include "Core.h"
#include "CORE_Game.h"

#include "VS_Input.h"

#define PULSE_DURATION (2.0f)

#ifdef VS_DEFAULT_VIRTUAL_CONTROLLER

vsSimpleMenu::vsSimpleMenu(int count, float letterSize, float capSize, float lineSpacing):
	vsSprite(NULL),
	m_itemLabel(NULL),
	m_itemValue(NULL),
	m_itemCount(count),
	m_highlightedId(0),
	m_pulseTimer(0.f),
	m_letterSize(letterSize),
	m_capSize(capSize),
	m_lineSpacing(lineSpacing)
{
	m_itemLabel = new vsSprite*[count];
	m_itemValue = new vsSprite*[count];

	for ( int i = 0; i < count; i++ )
	{
		m_itemLabel[i] = NULL;
		m_itemValue[i] = NULL;
	}
}

vsSimpleMenu::~vsSimpleMenu()
{
	for ( int i = 0; i < m_itemCount; i++ )
	{
		if ( m_itemLabel[i] )
		{
			RemoveChild( m_itemLabel[i] );
			vsDelete(m_itemLabel[i]);
		}
		if ( m_itemValue[i] )
		{
			RemoveChild( m_itemValue[i] );
			vsDelete(m_itemValue[i]);
		}
	}

	vsDeleteArray( m_itemLabel );
	vsDeleteArray( m_itemValue );
}

void
vsSimpleMenu::Update(float timeStep)
{
	if ( !GetVisible() )
		return;

	// clear out any actions from the previous frame
	m_action.Clear();

	vsInput *input = core::GetGame()->GetInput();

	if ( input->WasPressed( CID_Down ) || input->WasPressed( CID_LDown ) )
	{
		m_highlightedId++;
		//core::GetGame()->GetSound()->PlaySound(m_tickSound);
		if ( m_highlightedId >= m_itemCount )
			m_highlightedId = 0;
	}
	if ( input->WasPressed( CID_Up ) || input->WasPressed( CID_LUp ) )
	{
		//core::GetGame()->GetSound()->PlaySound(m_tickSound);
		m_highlightedId--;
		if ( m_highlightedId < 0 )
			m_highlightedId = m_itemCount-1;
	}
	if ( input->WasPressed( CID_Left ) )
		m_action.Left(m_highlightedId);
	if ( input->WasPressed( CID_Right ) )
		m_action.Right(m_highlightedId);
	if ( input->WasPressed( CID_A ) )
		m_action.Select(m_highlightedId);
	else if ( input->WasPressed( CID_B ) )
		m_action.Cancel();


	m_pulseTimer += timeStep;
	if ( m_pulseTimer > PULSE_DURATION )
		m_pulseTimer -= PULSE_DURATION;

	float frac = m_pulseTimer / PULSE_DURATION;

	float pulseAmt = vsCos(TWOPI * frac);	// [ -1..1 ]
	pulseAmt = (pulseAmt * 0.5f) + 0.5f;	// [ 0..1 ]

	for ( int i = 0; i < m_itemCount; i++ )
	{
		if ( m_itemLabel[i] )
		{
			vsColor c = c_blue;

			if ( i == m_highlightedId )
			{
				vsColor lightBlue(0.5f,0.5f,1.0f,0.8f);
				c = vsInterpolate( pulseAmt, lightBlue, c_white );
			}

			m_itemLabel[i]->SetColor( c );
			if ( m_itemValue[i] )
				m_itemValue[i]->SetColor( c );
		}
	}
}

void
vsSimpleMenu::Draw( vsRenderQueue *queue )
{
	ArrangeItems();

	Parent::Draw(queue);
}

void
vsSimpleMenu::ArrangeItems()
{
	int line = 0;
	float maxWidth = 0.f;

	// first, figure out what our widest label is
	for ( int i = 0; i < m_itemCount; i++ )
	{
		if ( m_itemLabel[i] )
		{
			float width = m_itemLabel[i]->GetBoundingRadius();

			if ( width > maxWidth )
				maxWidth = width;
		}
	}

	// now move our items into place, with the values far enough over that they won't overlap any long labels.
	for ( int i = 0; i < m_itemCount; i++ )
	{
		if ( m_itemLabel[i] )
		{
			vsVector2D pos( 0, line * (m_capSize + m_lineSpacing) );
			vsVector2D vpos( maxWidth + 50.f, line * (m_capSize + m_lineSpacing) );
			m_itemLabel[i]->SetPosition(pos);

			if ( m_itemValue[i] )
				m_itemValue[i]->SetPosition(vpos);

			line++;
		}
	}
}


void
vsSimpleMenu::SetItemCount(int count)
{
	m_itemCount = count;
}

void
vsSimpleMenu::SetItemLabel( int itemId, const vsString & label )
{
	vsAssert(itemId < m_itemCount && itemId >= 0, "itemId out of bounds!");
	if ( m_itemLabel[itemId] )
	{
		RemoveChild( m_itemLabel[itemId] );
		delete m_itemLabel[itemId];
	}

	m_itemLabel[itemId] = new vsSprite(vsBuiltInFont::CreateString(label, m_letterSize, m_capSize));
	m_itemLabel[itemId]->SetColor(c_blue);
	AddChild( m_itemLabel[itemId] );
}

void
vsSimpleMenu::SetItemValue( int itemId, const vsString & value )
{
	vsAssert(itemId < m_itemCount && itemId >= 0, "itemId out of bounds!");
	if ( m_itemValue[itemId] )
		delete m_itemValue[itemId];

	m_itemValue[itemId] = new vsSprite(vsBuiltInFont::CreateString(value, m_letterSize, m_capSize));
	m_itemValue[itemId]->SetColor(c_blue);
	AddChild( m_itemValue[itemId] );
}

#endif // VS_DEFAULT_VIRTUAL_CONTROLLER
