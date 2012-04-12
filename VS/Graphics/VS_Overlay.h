/*
 *  VS_Overlay.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 3/02/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_OVERLAY_H
#define VS_OVERLAY_H

#include "VS/Graphics/VS_Color.h"
#include "VS/Math/VS_Vector.h"

class vsOverlay
{
public:
	static vsOverlay Zero;

	enum Type
	{
		None,
		Constant,
		Linear,
		LinearLocal,
		TYPE_MAX
	};
	
	Type			m_type;

	vsVector2D		m_a;
	vsVector2D		m_direction;
	float			m_distance;
	vsColor			m_aColor;
	vsColor			m_bColor;
	
	
	
	vsOverlay();
	
	Type	GetType() const { return m_type; }
	
	void	Clear();
	
	bool	IsLocal() const { return (m_type == Constant || m_type == LinearLocal); }
	
	void	SetNone() { Clear(); }
	void	SetConstant( const vsColor &color );
	void	SetLinear( const vsVector2D &a, const vsColor &aColor, const vsVector2D &b, const vsColor &bColor );
	void	SetLinearLocal( const vsVector2D &a, const vsColor &aColor, const vsVector2D &b, const vsColor &bColor );
	
	vsColor	GetColorForPointColor( const vsVector2D &pos, const vsColor &c ) const;
};

#endif // VS_OVERLAY_H

