/*
 *  VS_Light.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_LIGHT_H
#define VS_LIGHT_H

#include "VS/Graphics/VS_Color.h"
#include "VS/Math/VS_Vector.h"

class vsLight
{
public:
	enum Type
	{
		Type_Ambient,
		Type_Directional,
		Type_Point,
//		Type_Spot,
		TYPE_MAX
	};

	vsVector3D	m_position;
	vsVector3D	m_direction;

	vsColor		m_color;
	vsColor		m_ambient;
	vsColor		m_specular;

	Type		m_type;


				vsLight();
	virtual		~vsLight();

	void				SetType( Type t ) { m_type = t; }
	void				SetPosition( const vsVector3D &pos ) { m_position = pos; }
	void				SetDirection( const vsVector3D &dir ) { m_direction = dir; }
	void				SetColor( const vsColor &color ) { m_color = color; }
	void				SetAmbientColor( const vsColor &ambient ) { m_ambient = ambient; }
	void				SetSpecularColor( const vsColor &specular ) { m_specular = specular; }

	Type				GetType() const { return m_type; }
	const vsVector3D &	GetPosition() const { return m_position; }
	const vsVector3D &	GetDirection() const { return m_direction; }
	const vsColor &		GetColor() const { return m_color; }
	const vsColor &		GetAmbientColor() const { return m_ambient; }
	const vsColor &		GetSpecularColor() const { return m_specular; }
};

#endif // VS_LIGHT_H

