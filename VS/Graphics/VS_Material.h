/*
 *  VS_Material.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_MATERIAL_H
#define VS_MATERIAL_H

#include "VS/Utils/VS_Cache.h"

#include "VS_Color.h"
#include "VS_Texture.h"

enum vsDrawMode
{
	DrawMode_Absolute,
	DrawMode_Normal,
	DrawMode_Lit,
	DrawMode_Add,
	DrawMode_Subtract,

	DRAWMODE_MAX
};

class vsMaterialInternal;

class vsMaterial : public vsCacheReference<vsMaterialInternal>
{
	struct Value
	{
		union
		{
			float f32;
			bool b;
			void* bind;
		};
		bool bound;
	};
	Value		*m_uniformValue;
	int m_uniformCount;

protected:
	vsMaterial();
	void SetupParameters();
public:

	vsMaterial( const vsString &name );
	vsMaterial( vsMaterial *other );
	virtual ~vsMaterial();

	int32_t UniformId( const vsString& name );
	void SetUniformF( int32_t id, float value );
	void SetUniformB( int32_t id, bool value );
	bool BindUniformF( int32_t id, float* value );
	bool BindUniformB( int32_t id, bool* value );
	float UniformF( int32_t id );
	bool UniformB( int32_t id );

	bool operator==(const vsMaterial &b) const { return (m_resource == b.m_resource); }
	bool operator!=(const vsMaterial &b) const { return !((*this)==b); }

	static vsMaterial *	White;
};

#endif // VS_MATERIAL_H

