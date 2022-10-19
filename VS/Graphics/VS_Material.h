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
#include "VS_ShaderValues.h"
#include "VS_ShaderOptions.h"
#include "VS_MaterialInternal.h"

class vsMaterialInternal;

class vsMaterial : public vsCacheReference<vsMaterialInternal>
{
	vsShaderValues m_values;
	vsShaderOptions m_options;

	// struct Value
	// {
	// 	union
	// 	{
	// 		float f32;
	// 		int b;
	// 		float vec4[4];
	// 		const void* bind;
	// 	};
	// 	bool bound;
	// };
	// Value		*m_uniformValue;
	// int m_uniformCount;

protected:
	vsMaterial();
	void SetupParameters();
public:

	vsMaterial( const vsString &name );
	vsMaterial( vsMaterial *other );
	virtual ~vsMaterial();

	// int32_t UniformId( const vsString& name );
	// void SetUniformF( int32_t id, float value );
	// void SetUniformB( int32_t id, bool value );
	// void SetUniformI( int32_t id, int value );
	// void SetUniformColor( int32_t id, const vsColor& value );
	// void SetUniformVec2( int32_t id, const vsVector2D& value );
	// void SetUniformVec3( int32_t id, const vsVector3D& value );
	// void SetUniformVec4( int32_t id, const vsVector4D& value );
	// bool BindUniformF( int32_t id, const float* value );
	// bool BindUniformB( int32_t id, const bool* value );
	// bool BindUniformI( int32_t id, const int* value );
	// bool BindUniformColor( int32_t id, const vsColor* value );
	// bool BindUniformVec3( int32_t id, const vsVector3D* value );
	// bool BindUniformVec4( int32_t id, const vsVector4D* value );
	// bool BindUniformMat4( int32_t id, const vsMatrix4x4* value );
	void SetUniformI( const vsString& name, int value );
	void SetUniformF( const vsString& name, float value );
	void SetUniformB( const vsString& name, bool value );
	void SetUniformColor( const vsString& name, const vsColor& value );
	void SetUniformVec2( const vsString& name, const vsVector2D& value );
	void SetUniformVec3( const vsString& name, const vsVector3D& value );
	void SetUniformVec4( const vsString& name, const vsVector4D& value );
	bool BindUniformF( const vsString& name, const float* value );
	bool BindUniformB( const vsString& name, const bool* value );
	bool BindUniformI( const vsString& name, const int* value );
	bool BindUniformColor( const vsString& name, const vsColor* value );
	bool BindUniformVec3( const vsString& name, const vsVector3D* value );
	bool BindUniformVec4( const vsString& name, const vsVector4D* value );
	bool BindUniformMat4( const vsString& name, const vsMatrix4x4* value );
	// float UniformF( int32_t id );
	// bool UniformB( int32_t id );
	// int UniformI( int32_t id );
	// vsVector4D UniformVec4( int32_t id );
	// vsMatrix4x4 UniformMat4( int32_t id );

	bool operator==(const vsMaterial &b) const { return (m_resource == b.m_resource); }
	bool operator!=(const vsMaterial &b) const { return !((*this)==b); }

	bool MatchesForBatching( vsMaterial *other ) const;

	vsShaderValues* GetShaderValues() { return &m_values; }
	vsShaderOptions* GetShaderOptions() { return &m_options; }

	// shader 'bits' control shader features.  They may be set to 'true' or 'false',
	// or may be unset.  (Clearing a shader bit returns it to 'unset').  These bits
	// may also be set via the 'PushShaderOptions' and 'PopShaderOptions' commands
	// in a display list.  Note that bits set via 'PushShaderOptions' will ALWAYS
	// override any shader bits set on the material.
	void SetShaderBit(uint8_t bitId, bool bitValue);
	void ClearShaderBit(uint8_t bitId);

	static vsMaterial *	White;
};

#endif // VS_MATERIAL_H

