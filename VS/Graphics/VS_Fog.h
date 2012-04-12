/*
 *  VS_Fog.h
 *  MMORPG2
 *
 *  Created by Trevor Powell on 14/09/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_FOG_H
#define VS_FOG_H

#include "VS_Color.h"

class vsFog
{
	vsColor		m_fogColor;
	float		m_density;
	float		m_start;
	float		m_end;
	
	bool		m_linear;

public:
						vsFog();
	
	void				SetLinear( const vsColor &color, float start, float end );
	void				SetExponential( const vsColor &color, float density );
	
	bool				IsLinear() const { return m_linear; }
	bool				IsExponential() const { return !m_linear; }
	
	const vsColor &		GetColor() const	{ return m_fogColor; }
	float				GetDensity() const	{ return m_density; }
	float				GetStart() const	{ return m_start; }
	float				GetEnd() const		{ return m_end; }
};

#endif // VS_FOG_H

