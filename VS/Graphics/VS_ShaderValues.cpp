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
#include "VS_ShaderUniformRegistry.h"
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
		const vsIntHashEntry<Value> *v = other.m_value.GetHashEntry(i);

		m_value[v->m_key] = v->m_item;
	}
}

void
vsShaderValues::SetUniformF( const vsString& name, float value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].f32 = value;
		m_value[id].type = Value::Type_Float;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformB( const vsString& name, bool value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].b = value;
		m_value[id].type = Value::Type_Bool;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformI( const vsString& name, int value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].i = value;
		m_value[id].type = Value::Type_Int;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformColor( const vsString& name, const vsColor& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].vec4[0] = value.r;
		m_value[id].vec4[1] = value.g;
		m_value[id].vec4[2] = value.b;
		m_value[id].vec4[3] = value.a;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec2( const vsString& name, const vsVector2D& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].vec4[0] = value.x;
		m_value[id].vec4[1] = value.y;
		m_value[id].vec4[2] = 0.0;
		m_value[id].vec4[3] = 0.0;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec3( const vsString& name, const vsVector3D& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].vec4[0] = value.x;
		m_value[id].vec4[1] = value.y;
		m_value[id].vec4[2] = value.z;
		m_value[id].vec4[3] = 0.0;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec4( const vsString& name, const vsVector4D& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].vec4[0] = value.x;
		m_value[id].vec4[1] = value.y;
		m_value[id].vec4[2] = value.z;
		m_value[id].vec4[3] = value.w;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

bool
vsShaderValues::BindUniformF( const vsString& name, const float* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformB( const vsString& name, const bool* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformI( const vsString& name, const int* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformColor( const vsString& name, const vsColor* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformVec2( const vsString& name, const vsVector2D* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformVec3( const vsString& name, const vsVector3D* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformVec4( const vsString& name, const vsVector4D* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::BindUniformMat4( const vsString& name, const vsMatrix4x4* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].bind = value;
		m_value[id].type = Value::Type_Bind;
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::Has( const vsString& name ) const
{
	uint32_t id = vsShaderUniformRegistry::UID(name);
	return (m_value.FindItem(id) != NULL) ||
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
vsShaderValues::UniformF( uint32_t uid, float& out ) const
{
	const Value* v = m_value.FindItem(uid);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformF(uid,out);
		return false;
	}
	if ( v->bound )
		out = *(float*)v->bind;
	else
		out = v->f32;
	return true;
}

bool
vsShaderValues::UniformB( uint32_t uid, bool& out ) const
{
	const Value* v = m_value.FindItem(uid);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformB(uid,out);
		return false;
	}
	if ( v->bound )
		out = *(bool*)v->bind;
	else
		out = v->b;
	return true;
}

bool
vsShaderValues::UniformI( uint32_t uid, int& out ) const
{
	const Value* v = m_value.FindItem(uid);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformI(uid,out);
		return false;
	}
	if ( v->bound )
		out = *(int*)v->bind;
	else
		out = v->i;
	return true;
}

bool
vsShaderValues::UniformVec2( uint32_t uid, vsVector2D& out ) const
{
	const Value* v = m_value.FindItem(uid);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformVec2(uid,out);
		return false;
	}
	if ( v->bound )
		out = *(vsVector2D*)v->bind;
	else
		out = *(vsVector2D*)v->vec4;
	return true;
}

bool
vsShaderValues::UniformVec3( uint32_t uid, vsVector3D& out ) const
{
	const Value* v = m_value.FindItem(uid);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformVec3(uid,out);
		return false;
	}
	if ( v->bound )
		out = *(vsVector3D*)v->bind;
	else
		out = *(vsVector3D*)v->vec4;
	return true;
}

bool
vsShaderValues::UniformVec4( uint32_t uid, vsVector4D& out ) const
{
	const Value* v = m_value.FindItem(uid);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformVec4(uid,out);
		return false;
	}
	if ( v->bound )
		out = *(vsVector4D*)v->bind;
	else
		out = *(vsVector4D*)v->vec4;
	return true;
}

bool
vsShaderValues::UniformMat4( uint32_t uid, vsMatrix4x4& out ) const
{
	const Value* v = m_value.FindItem(uid);
	if ( !v )
	{
		if ( m_parent )
			return m_parent->UniformMat4(uid,out);
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

	return m_value == other.m_value;
	// now check the values.

	// int valueCount = m_value.GetHashEntryCount();
	// if ( other.m_value.GetHashEntryCount() != valueCount )
	// 	return false;
    //
	// for ( int i = 0; i < valueCount; i++ )
	// {
	// 	const vsHashEntry<Value> *v = m_value.GetHashEntry(i);
	// 	const vsHashEntry<Value> *ov = other.m_value.GetHashEntry(i);
	// 	// keys aren't of matching types?  Fail.
	// 	if ( v->m_item.type != ov->m_item.type )
	// 		return false;
	// 	// keys aren't of matching name?  Fail.
	// 	if ( v->m_key != ov->m_key )
	// 		return false;
    //
	// 	switch ( v->m_item.type )
	// 	{
	// 		case Value::Type_Float:
	// 			if ( v->m_item.f32 != ov->m_item.f32 )
	// 				return false;
	// 			break;
	// 		case Value::Type_Bool:
	// 			if ( v->m_item.b != ov->m_item.b )
	// 				return false;
	// 			break;
	// 		case Value::Type_Int:
	// 			if ( v->m_item.i != ov->m_item.i )
	// 				return false;
	// 			break;
	// 		case Value::Type_Vec4:
	// 			if ( v->m_item.vec4 != ov->m_item.vec4 )
	// 				return false;
	// 			break;
	// 		case Value::Type_Bind:
	// 			// uhhhh.. should I be trying to check the value?
	// 			if ( v->m_item.bind != ov->m_item.bind )
	// 				return false;
	// 			break;
	// 		case Value::Type_MAX:
	// 			break;
	// 	}
	// }
    //
	// return true;
}

bool
vsShaderValues::Value::operator==( const struct Value& other ) const
{
	if ( type != other.type )
		return false;

	switch ( type )
	{
		case Value::Type_Float:
			if ( f32 != other.f32 )
				return false;
			break;
		case Value::Type_Bool:
			if ( b != other.b )
				return false;
			break;
		case Value::Type_Int:
			if ( i != other.i )
				return false;
			break;
		case Value::Type_Vec4:
			if ( vec4 != other.vec4 )
				return false;
			break;
		case Value::Type_Bind:
			// uhhhh.. should I be trying to check the value?
			if ( bind != other.bind )
				return false;
			break;
		case Value::Type_MAX:
			break;
	}
	return true;
}

