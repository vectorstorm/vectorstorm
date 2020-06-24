/*
 *  VS_ShaderValues.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 28/08/2015
 *  Copyright 2015 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_ShaderValues.h"
#include "VS_Shader.h"
#include "VS_OpenGL.h"
#include "VS_Matrix.h"

vsShaderValues::vsShaderValues():
	m_parent(NULL),
	m_value(16)
{
}

vsShaderValues::vsShaderValues( const vsShaderValues& other ):
	m_parent(NULL),
	m_value(16)
{
	int valueCount = other.m_value.GetHashEntryCount();

	for ( int i = 0; i < valueCount; i++ )
	{
		const vsHashEntry<Value> *v = other.m_value.GetHashEntry(i);

		m_value[v->m_key] = v->m_item;
	}
}

void
vsShaderValues::SetUniformF( const vsString& id, float value )
{
	{
		m_value[id].f32 = value;
		m_value[id].type = Value::Type_Float;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformB( const vsString& id, bool value )
{
	{
		m_value[id].b = value;
		m_value[id].type = Value::Type_Bool;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformI( const vsString& id, int value )
{
	{
		m_value[id].i = value;
		m_value[id].type = Value::Type_Int;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformColor( const vsString& id, const vsColor& value )
{
	{
		m_value[id].vec4[0] = value.r;
		m_value[id].vec4[1] = value.g;
		m_value[id].vec4[2] = value.b;
		m_value[id].vec4[3] = value.a;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec2( const vsString& id, const vsVector2D& value )
{
	{
		m_value[id].vec4[0] = value.x;
		m_value[id].vec4[1] = value.y;
		m_value[id].vec4[2] = 0.0;
		m_value[id].vec4[3] = 0.0;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec3( const vsString& id, const vsVector3D& value )
{
	{
		m_value[id].vec4[0] = value.x;
		m_value[id].vec4[1] = value.y;
		m_value[id].vec4[2] = value.z;
		m_value[id].vec4[3] = 0.0;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec4( const vsString& id, const vsVector4D& value )
{
	{
		m_value[id].vec4[0] = value.x;
		m_value[id].vec4[1] = value.y;
		m_value[id].vec4[2] = value.z;
		m_value[id].vec4[3] = value.w;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

bool
vsShaderValues::BindUniformF( const vsString& id, const float* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformB( const vsString& id, const bool* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformI( const vsString& id, const int* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformColor( const vsString& id, const vsColor* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformVec2( const vsString& id, const vsVector2D* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformVec3( const vsString& id, const vsVector3D* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformVec4( const vsString& id, const vsVector4D* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformMat4( const vsString& id, const vsMatrix4x4* value )
{
	{
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::Has( const vsString& name )
{
	return (m_value.FindItem(name) != NULL) ||
		( m_parent && m_parent->Has(name) );
}

// float
// vsShaderValues::UniformF( const vsString& id )
// {
// 	Value* v = m_value.FindItem(id);
// 	if ( !v )
// 		return 0.f;
// 	if ( v->bound )
// 		return *(float*)v->bind;
// 	else
// 		return v->f32;
// }
//
// bool
// vsShaderValues::UniformB( const vsString& id )
// {
// 	Value* v = m_value.FindItem(id);
// 	if ( !v )
// 		return false;
// 	if ( v->bound )
// 		return *(bool*)v->bind;
// 	else
// 		return v->b;
// }
//

// vsVector4D
// vsShaderValues::UniformVec4( const vsString& id )
// {
// 	Value* v = m_value.FindItem(id);
// 	if ( !v )
// 		return vsVector4D();
// 	if ( v->bound )
// 		return *(vsVector4D*)v->bind;
// 	else
// 		return *(vsVector4D*)v->vec4;
// }
//
// vsVector3D
// vsShaderValues::UniformVec3( const vsString& id )
// {
// 	return vsVector3D( UniformVec4(id) );
// }
//
// vsVector2D
// vsShaderValues::UniformVec2( const vsString& id )
// {
// 	return vsVector2D( UniformVec4(id) );
// }
//
bool
vsShaderValues::UniformF( const vsString& id, float& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformF(id,out);
		return false;
	}
	if ( v->bound )
		out = *(float*)v->bind;
	else
		out = v->f32;
	return true;
}

bool
vsShaderValues::UniformB( const vsString& id, bool& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformB(id,out);
		return false;
	}
	if ( v->bound )
		out = *(bool*)v->bind;
	else
		out = v->b;
	return true;
}

bool
vsShaderValues::UniformI( const vsString& id, int& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformI(id,out);
		return false;
	}
	if ( v->bound )
		out = *(int*)v->bind;
	else
		out = v->i;
	return true;
}

bool
vsShaderValues::UniformVec2( const vsString& id, vsVector2D& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformVec2(id,out);
		return false;
	}
	if ( v->bound )
		out = *(vsVector2D*)v->bind;
	else
		out = *(vsVector2D*)v->vec4;
	return true;
}

bool
vsShaderValues::UniformVec3( const vsString& id, vsVector3D& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformVec3(id,out);
		return false;
	}
	if ( v->bound )
		out = *(vsVector3D*)v->bind;
	else
		out = *(vsVector3D*)v->vec4;
	return true;
}

bool
vsShaderValues::UniformVec4( const vsString& id, vsVector4D& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformVec4(id,out);
		return false;
	}
	if ( v->bound )
		out = *(vsVector4D*)v->bind;
	else
		out = *(vsVector4D*)v->vec4;
	return true;
}

bool
vsShaderValues::UniformMat4( const vsString& id, vsMatrix4x4& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformMat4(id,out);
		return false;
	}
	if ( v->bound )
		out = *(vsMatrix4x4*)v->bind;
	// else
	// 	out = *(vsMatrix4x4*)v->mat4;
	return true;
}

bool
vsShaderValues::operator==( const vsShaderValues& other ) const
{
	if ( m_parent != other.m_parent )
		return false;

	// now check the values.

	int valueCount = m_value.GetHashEntryCount();

	for ( int i = 0; i < valueCount; i++ )
	{
		const vsHashEntry<Value> *v = m_value.GetHashEntry(i);
		const Value *ov = other.m_value.FindItem(v->m_key);
		// no matching key?  Fail.
		if ( !ov )
			return false;
		// keys aren't of matching types?  Fail.
		if ( v->m_item.type != ov->type )
			return false;

		switch ( v->m_item.type )
		{
			case Value::Type_Float:
				if ( v->m_item.f32 != ov->f32 )
					return false;
				break;
			case Value::Type_Bool:
				if ( v->m_item.b != ov->b )
					return false;
				break;
			case Value::Type_Int:
				if ( v->m_item.i != ov->i )
					return false;
				break;
			case Value::Type_Vec4:
				if ( v->m_item.vec4 != ov->vec4 )
					return false;
				break;
			case Value::Type_Bind:
				// uhhhh.. should I be trying to check the value?
				if ( v->m_item.bind != ov->bind )
					return false;
				break;
			case Value::Type_MAX:
				break;
		}
	}

	return true;
}

