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
	DrawMode_Multiply,

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
			int b;
			float vec4[4];
			const void* bind;
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
	void SetUniformI( int32_t id, int value );
	void SetUniformColor( int32_t id, const vsColor& value );
	void SetUniformVec3( int32_t id, const vsVector3D& value );
	void SetUniformVec4( int32_t id, const vsVector4D& value );
	bool BindUniformF( int32_t id, const float* value );
	bool BindUniformB( int32_t id, const bool* value );
	bool BindUniformI( int32_t id, const int* value );
	bool BindUniformColor( int32_t id, const vsColor* value );
	bool BindUniformVec3( int32_t id, const vsVector3D* value );
	bool BindUniformVec4( int32_t id, const vsVector4D* value );
	bool BindUniformMat4( int32_t id, const vsMatrix4x4* value );
	void SetUniformF( const vsString& name, float value );
	void SetUniformB( const vsString& name, bool value );
	void SetUniformColor( const vsString& name, const vsColor& value );
	void SetUniformVec3( const vsString& name, const vsVector3D& value );
	void SetUniformVec4( const vsString& name, const vsVector4D& value );
	bool BindUniformF( const vsString& name, const float* value );
	bool BindUniformB( const vsString& name, const bool* value );
	bool BindUniformColor( const vsString& name, const vsColor* value );
	bool BindUniformVec3( const vsString& name, const vsVector3D* value );
	bool BindUniformVec4( const vsString& name, const vsVector4D* value );
	bool BindUniformMat4( const vsString& name, const vsMatrix4x4* value );
	float UniformF( int32_t id );
	bool UniformB( int32_t id );
	int UniformI( int32_t id );
	vsVector4D UniformVec4( int32_t id );
	vsMatrix4x4 UniformMat4( int32_t id );

	bool operator==(const vsMaterial &b) const { return (m_resource == b.m_resource); }
	bool operator!=(const vsMaterial &b) const { return !((*this)==b); }

	static vsMaterial *	White;
};

#endif // VS_MATERIAL_H

