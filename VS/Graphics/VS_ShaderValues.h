/*
 *  VS_ShaderValues.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 28/08/2015
 *  Copyright 2015 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SHADERVALUES_H
#define VS_SHADERVALUES_H

#include "VS/Utils/VS_HashTable.h"
#include "VS/Utils/VS_IntHashTable.h"
#include "VS/Utils/VS_String.h"

class vsColor;
class vsShader;
class vsVector3D;
class vsVector4D;
class vsMatrix4x4;

class vsShaderValues
{
	struct Value
	{
		enum Type
		{
			Type_Float,
			Type_Bool,
			Type_Int,
			Type_Vec4,
			Type_Bind,
			Type_MAX
		};
		union
		{
			float f32;
			bool b;
			int i;
			float vec4[4];
			const void* bind;
		} u;
		vsString name;
		Type type;
		bool bound;

		Value(): type(Type_MAX), bound (false) {}
		Value( const Value& other ){
			name = other.name;
			bound = other.bound;
			type = other.type;
			// our union contains only POD data types so technically it's safe
			// to just memcpy its data across from one object to the other.
			memcpy(&u, &other.u, sizeof(u)); // ugh.
		}
		bool operator==( const struct Value& other ) const;
		bool operator!=( const struct Value& other ) const { return !(*this == other); }

	};

	vsShaderValues *m_parent;
	vsIntHashTable<Value> m_value;
public:

	vsShaderValues();
	vsShaderValues( const vsShaderValues& other );

	// a parent object will handle any uniforms which we don't set ourselves.
	void SetParent( vsShaderValues *parent ) { m_parent = parent; }

	void SetUniformF( const vsString& name, float value );
	void SetUniformB( const vsString& name, bool value );
	void SetUniformI( const vsString& name, int value );
	void SetUniformColor( const vsString& name, const vsColor& value );
	void SetUniformVec2( const vsString& name, const vsVector2D& value );
	void SetUniformVec3( const vsString& name, const vsVector3D& value );
	void SetUniformVec4( const vsString& name, const vsVector4D& value );
	bool BindUniformF( const vsString& name, const float* value );
	bool BindUniformB( const vsString& name, const bool* value );
	bool BindUniformI( const vsString& name, const int* value );
	bool BindUniformColor( const vsString& name, const vsColor* value );
	bool BindUniformVec2( const vsString& name, const vsVector2D* value );
	bool BindUniformVec3( const vsString& name, const vsVector3D* value );
	bool BindUniformVec4( const vsString& name, const vsVector4D* value );
	bool BindUniformMat4( const vsString& name, const vsMatrix4x4* value );
	bool Has( const vsString& name ) const;
	bool UniformF( uint32_t uid, float& out ) const;
	bool UniformB( uint32_t uid, bool& out ) const;
	bool UniformI( uint32_t uid, int& out ) const;
	bool UniformVec2( uint32_t uid, vsVector2D& out ) const;
	bool UniformVec3( uint32_t uid, vsVector3D& out ) const;
	bool UniformVec4( uint32_t uid, vsVector4D& out ) const;
	bool UniformMat4( uint32_t uid, vsMatrix4x4& out ) const;

	bool operator==( const vsShaderValues& other ) const;
	bool operator!=( const vsShaderValues& other ) const { return ! (*this == other); }
};

#endif // VS_SHADERVALUES_H

