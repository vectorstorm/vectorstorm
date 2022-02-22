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
	m_parent(nullptr),
	m_value(16)
{
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
	{
		m_texture[i] = nullptr;
		m_textureSet[i] = false;
	}
}

vsShaderValues::vsShaderValues( const vsShaderValues& other ):
	m_parent(nullptr),
	m_value(16)
{
	int valueCount = other.m_value.GetHashEntryCount();

	for ( int i = 0; i < valueCount; i++ )
	{
		const vsIntHashEntry<Value> *v = other.m_value.GetHashEntry(i);

		m_value[v->m_key] = v->m_item;
	}
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
	{
		m_texture[i] = other.m_texture[i];
		m_textureSet[i] = other.m_textureSet[i];
	}
}

void
vsShaderValues::SetUniformF( const vsString& name, float value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.f32 = value;
		m_value[id].type = Value::Type_Float;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformB( const vsString& name, bool value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.b = value;
		m_value[id].type = Value::Type_Bool;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformI( const vsString& name, int value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.i = value;
		m_value[id].type = Value::Type_Int;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformColor( const vsString& name, const vsColor& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.vec4[0] = value.r;
		m_value[id].u.vec4[1] = value.g;
		m_value[id].u.vec4[2] = value.b;
		m_value[id].u.vec4[3] = value.a;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec2( const vsString& name, const vsVector2D& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.vec4[0] = value.x;
		m_value[id].u.vec4[1] = value.y;
		m_value[id].u.vec4[2] = 0.0;
		m_value[id].u.vec4[3] = 0.0;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec3( const vsString& name, const vsVector3D& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.vec4[0] = value.x;
		m_value[id].u.vec4[1] = value.y;
		m_value[id].u.vec4[2] = value.z;
		m_value[id].u.vec4[3] = 0.0;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformVec4( const vsString& name, const vsVector4D& value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.vec4[0] = value.x;
		m_value[id].u.vec4[1] = value.y;
		m_value[id].u.vec4[2] = value.z;
		m_value[id].u.vec4[3] = value.w;
		m_value[id].type = Value::Type_Vec4;
		m_value[id].bound = false;
	}
}

bool
vsShaderValues::BindUniformF( const vsString& name, const float* value )
{
	{
		uint32_t id = vsShaderUniformRegistry::UID(name);
		m_value[id].u.bind = value;
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
		m_value[id].u.bind = value;
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
		m_value[id].u.bind = value;
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
		m_value[id].u.bind = value;
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
		m_value[id].u.bind = value;
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
		m_value[id].u.bind = value;
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
		m_value[id].u.bind = value;
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
		m_value[id].u.bind = value;
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
	return (m_value.FindItem(id) != nullptr) ||
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
		out = *(float*)v->u.bind;
	else
		out = v->u.f32;
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
		out = *(bool*)v->u.bind;
	else
		out = v->u.b;
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
		out = *(int*)v->u.bind;
	else
		out = v->u.i;
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
		out = *(vsVector2D*)v->u.bind;
	else
		out = *(vsVector2D*)v->u.vec4;
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
		out = *(vsVector3D*)v->u.bind;
	else
		out = *(vsVector3D*)v->u.vec4;
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
		out = *(vsVector4D*)v->u.bind;
	else
		out = *(vsVector4D*)v->u.vec4;
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
		out = *(vsMatrix4x4*)v->u.bind;
	// else
	// 	out = *(vsMatrix4x4*)v->mat4;
	return true;
}

vsTexture *
vsShaderValues::GetTextureOverride( int i )
{
	if ( m_textureSet[i] )
		return m_texture[i];
	else if ( m_parent )
		return m_parent->GetTextureOverride(i);

	return nullptr;
}

bool
vsShaderValues::HasTextureOverride( int i ) const
{
	if ( m_textureSet[i] )
		return true;
	else if ( m_parent )
		return m_parent->HasTextureOverride(i);
	return false;
}

const vsShaderValues&
vsShaderValues::operator=( const vsShaderValues& other )
{
	int valueCount = other.m_value.GetHashEntryCount();
	m_value.Clear();

	for ( int i = 0; i < valueCount; i++ )
	{
		const vsIntHashEntry<Value> *v = other.m_value.GetHashEntry(i);

		m_value[v->m_key] = v->m_item;
	}
	for ( int i = 0; i < MAX_TEXTURE_SLOTS; i++ )
	{
		m_texture[i] = other.m_texture[i];
		m_textureSet[i] = other.m_textureSet[i];
	}
	return *this;
}

bool
vsShaderValues::operator==( const vsShaderValues& other ) const
{
	if ( m_parent != other.m_parent )
		return false;

	// we should also check our texture bindings here

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
			if ( u.f32 != other.u.f32 )
				return false;
			break;
		case Value::Type_Bool:
			if ( u.b != other.u.b )
				return false;
			break;
		case Value::Type_Int:
			if ( u.i != other.u.i )
				return false;
			break;
		case Value::Type_Vec4:
			for ( int i = 0; i < 4; i++ )
			{
				if ( u.vec4[i] != other.u.vec4[i] )
					return false;
			}
			break;
		case Value::Type_Bind:
			// uhhhh.. should I be trying to check the value?
			if ( u.bind != other.u.bind )
				return false;
			break;
		case Value::Type_MAX:
			break;
	}
	return true;
}

