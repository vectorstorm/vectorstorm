/*
 *  VS_Overlay.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/02/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Overlay.h"

vsOverlay vsOverlay::Zero;

vsOverlay::vsOverlay()
{
	Clear();
}

void
vsOverlay::Clear()
{
	m_type = None;
	m_a = vsVector2D::Zero;
	m_direction = vsVector2D::Zero;
	m_distance = 0.f;
	m_aColor = vsColor::PureWhite;
	m_bColor = vsColor::PureWhite;
}

void
vsOverlay::SetConstant( const vsColor &c )
{
	m_type = Constant;
	m_aColor = m_bColor = c;
}

void
vsOverlay::SetLinear( const vsVector2D &a, const vsColor &aColor, const vsVector2D &b, const vsColor &bColor )
{
	m_type = Linear;
	m_a = a;
	m_direction = b - a;
	m_distance = m_direction.Length();
	m_direction.Normalise();
	
	m_aColor = aColor;
	m_bColor = bColor;
}

void
vsOverlay::SetLinearLocal( const vsVector2D &a, const vsColor &aColor, const vsVector2D &b, const vsColor &bColor )
{
	m_type = LinearLocal;
	m_a = a;
	m_direction = b - a;
	m_distance = m_direction.Length();
	m_direction.Normalise();
	
	m_aColor = aColor;
	m_bColor = bColor;
}


vsColor
vsOverlay::GetColorForPointColor( const vsVector2D &pos, const vsColor &c ) const
{
	vsColor result = c;
	
	switch( m_type )
	{
		case None:
			break;
		case Constant:
			result = c * m_aColor;
			break;
		case Linear:
		case LinearLocal:
		{
			vsVector2D offset = pos - m_a;
			float projection = offset.Dot(m_direction);
			projection = vsClamp(projection/m_distance,0.f,1.f);
			
			vsColor interpColor = vsInterpolate( projection, m_aColor, m_bColor );
			result = c * interpColor;
			break;
		}
		default:
			break;
	}
	
	return result;
}

