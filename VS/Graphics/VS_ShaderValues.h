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
		union
		{
			float f32;
			bool b;
			float vec4[4];
			const void* bind;
		};
		bool bound;

		Value(): bound (false) {}
	};

	vsShaderValues *m_parent;
	vsHashTable<Value> m_value;
public:

	vsShaderValues();

	// a parent object will handle any uniforms which we don't set ourselves.
	void SetParent( vsShaderValues *parent ) { m_parent = parent; }

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
	bool Has( const vsString& name );
	bool UniformF( const vsString& name, float& out );
	bool UniformB( const vsString& name, bool& out );
	bool UniformVec4( const vsString& name, vsVector4D& out );
	bool UniformMat4( const vsString& name, vsMatrix4x4& out );
};

#endif // VS_SHADERVALUES_H

