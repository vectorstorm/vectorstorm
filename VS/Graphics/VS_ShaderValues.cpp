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
	m_value(16)
{
}

void
vsShaderValues::SetUniformF( const vsString& id, float value )
{
	{
		m_value[id].f32 = value;
		m_value[id].bound = false;
	}
}

void
vsShaderValues::SetUniformB( const vsString& id, bool value )
{
	{
		m_value[id].b = value;
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
		m_value[id].bound = false;
	}
}

bool
vsShaderValues::BindUniformF( const vsString& id, const float* value )
{
	{
		m_value[id].bind = value;
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
		m_value[id].bound = true;
		return true;
	}
	return false;
}

bool
vsShaderValues::Has( const vsString& name )
{
	return m_value.FindItem(name) != NULL;
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

bool
vsShaderValues::UniformF( const vsString& id, float& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
		return false;
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
		return false;
	if ( v->bound )
		out = *(bool*)v->bind;
	else
		out = v->b;
	return true;
}

bool
vsShaderValues::UniformVec4( const vsString& id, vsVector4D& out )
{
	Value* v = m_value.FindItem(id);
	if ( !v )
		return false;
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
		return false;
	if ( v->bound )
		out = *(vsMatrix4x4*)v->bind;
	// else
	// 	out = *(vsMatrix4x4*)v->mat4;
	return true;
}
