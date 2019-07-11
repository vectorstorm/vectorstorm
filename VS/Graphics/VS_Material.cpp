/*
 *  VS_Material.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16/03/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Material.h"
#include "VS_MaterialInternal.h"
#include "VS_Shader.h"

#include "VS_Texture.h"
#include "VS_OpenGL.h"

vsMaterial *vsMaterial::White = NULL;

vsMaterial::vsMaterial():
	vsCacheReference<vsMaterialInternal>( new vsMaterialInternal ),
	m_uniformValue(NULL),
	m_uniformCount(0)
{
	SetupParameters();
}

vsMaterial::vsMaterial( const vsString &name ):
	vsCacheReference<vsMaterialInternal>(name),
	m_uniformValue(NULL),
	m_uniformCount(0)
{
	SetupParameters();
}

vsMaterial::vsMaterial( vsMaterial *other ):
	vsCacheReference<vsMaterialInternal>(other->GetResource()->GetName()),
	m_uniformValue(NULL),
	m_uniformCount(0)
{
	SetupParameters();
	vsAssert( m_uniformCount == other->m_uniformCount, "Shader has changed??" );
	memcpy(m_uniformValue, other->m_uniformValue, sizeof(Value)*m_uniformCount);
}

vsMaterial::~vsMaterial()
{
	vsDeleteArray( m_uniformValue );
}

void
vsMaterial::SetupParameters()
{
	vsDeleteArray( m_uniformValue );
	if ( GetResource()->m_shader )
	{
		m_uniformCount = GetResource()->m_shader->GetUniformCount();
		m_uniformValue = new Value[m_uniformCount];
		memset(m_uniformValue, 0, sizeof(Value) * m_uniformCount);

		// Grab some default values from the shader.
		for ( int i = 0; i < m_uniformCount; i++ )
		{
			const vsShader::Uniform *u = GetResource()->m_shader->GetUniform(i);

			if ( u->type == GL_SAMPLER_2D )
				SetUniformI( i, u->b );
			if ( u->type == GL_SAMPLER_BUFFER )
				SetUniformI( i, u->b );
		}

		// Do some generic setup.
		SetUniformF( "alphaRef", GetResource()->m_alphaRef );
		SetUniformB( "fog", GetResource()->m_fog );
		BindUniformB( "glow", &GetResource()->m_glow );
		BindUniformF( "glowFactor", &GetResource()->m_glowFactor );

	}
}

int32_t
vsMaterial::UniformId( const vsString& name )
{
	return GetResource()->m_shader->GetUniformId(name);
}

void
vsMaterial::SetUniformF( int32_t id, float value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT )
	{
		m_uniformValue[id].f32 = value;
		m_uniformValue[id].bound = false;
	}
}

void
vsMaterial::SetUniformB( int32_t id, bool value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_BOOL )
	{
		m_uniformValue[id].b = value;
		m_uniformValue[id].bound = false;
	}
}

void
vsMaterial::SetUniformI( int32_t id, int value )
{
	if ( id >= 0 && id < m_uniformCount )
	{
		m_uniformValue[id].b = value;
		m_uniformValue[id].bound = false;
	}
}

void
vsMaterial::SetUniformColor( int32_t id, const vsColor& value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT_VEC4 )
	{
		m_uniformValue[id].vec4[0] = value.r;
		m_uniformValue[id].vec4[1] = value.g;
		m_uniformValue[id].vec4[2] = value.b;
		m_uniformValue[id].vec4[3] = value.a;
		m_uniformValue[id].bound = false;
	}
}

void
vsMaterial::SetUniformVec3( int32_t id, const vsVector3D& value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT_VEC3 )
	{
		m_uniformValue[id].vec4[0] = value.x;
		m_uniformValue[id].vec4[1] = value.y;
		m_uniformValue[id].vec4[2] = value.z;
		m_uniformValue[id].vec4[3] = 0.0;
		m_uniformValue[id].bound = false;
	}
}

void
vsMaterial::SetUniformVec4( int32_t id, const vsVector4D& value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT_VEC4 )
	{
		m_uniformValue[id].vec4[0] = value.x;
		m_uniformValue[id].vec4[1] = value.y;
		m_uniformValue[id].vec4[2] = value.z;
		m_uniformValue[id].vec4[3] = value.w;
		m_uniformValue[id].bound = false;
	}
}

void
vsMaterial::SetUniformI( const vsString& name, int value )
{
	int32_t id = UniformId(name);
	return SetUniformI(id,value);
}

void
vsMaterial::SetUniformF( const vsString& name, float value )
{
	int32_t id = UniformId(name);
	return SetUniformF(id,value);
}

void
vsMaterial::SetUniformColor( const vsString& name, const vsColor& value )
{
	int32_t id = UniformId(name);
	return SetUniformColor(id,value);
}

void
vsMaterial::SetUniformVec3( const vsString& name, const vsVector3D& value )
{
	int32_t id = UniformId(name);
	return SetUniformVec3(id,value);
}

void
vsMaterial::SetUniformVec4( const vsString& name, const vsVector4D& value )
{
	int32_t id = UniformId(name);
	return SetUniformVec4(id,value);
}

void
vsMaterial::SetUniformB( const vsString& name, bool value )
{
	int32_t id = UniformId(name);
	return SetUniformB(id,value);
}

bool
vsMaterial::BindUniformF( int32_t id, const float* value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT )
	{
		m_uniformValue[id].bind = value;
		m_uniformValue[id].bound = true;
		return true;
	}
	return false;
}

bool
vsMaterial::BindUniformB( int32_t id, const bool* value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_BOOL )
	{
		m_uniformValue[id].bind = value;
		m_uniformValue[id].bound = true;
		return true;
	}
	return false;
}

bool
vsMaterial::BindUniformI( int32_t id, const int* value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_INT )
	{
		m_uniformValue[id].bind = value;
		m_uniformValue[id].bound = true;
		return true;
	}
	return false;
}

bool
vsMaterial::BindUniformColor( int32_t id, const vsColor* value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT_VEC4 )
	{
		m_uniformValue[id].bind = value;
		m_uniformValue[id].bound = true;
		return true;
	}
	return false;
}

bool
vsMaterial::BindUniformVec3( int32_t id, const vsVector3D* value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT_VEC3 )
	{
		m_uniformValue[id].bind = value;
		m_uniformValue[id].bound = true;
		return true;
	}
	return false;
}

bool
vsMaterial::BindUniformVec4( int32_t id, const vsVector4D* value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT_VEC4 )
	{
		m_uniformValue[id].bind = value;
		m_uniformValue[id].bound = true;
		return true;
	}
	return false;
}

bool
vsMaterial::BindUniformMat4( int32_t id, const vsMatrix4x4* value )
{
	if ( id >= 0 && id < m_uniformCount && GetResource()->m_shader->GetUniform(id)->type == GL_FLOAT_MAT4 )
	{
		m_uniformValue[id].bind = value;
		m_uniformValue[id].bound = true;
		return true;
	}
	return false;
}

bool
vsMaterial::BindUniformF( const vsString& name, const float* value )
{
	int32_t id = UniformId(name);
	return BindUniformF(id,value);
}

bool
vsMaterial::BindUniformB( const vsString& name, const bool* value )
{
	int32_t id = UniformId(name);
	return BindUniformB(id,value);
}

bool
vsMaterial::BindUniformI( const vsString& name, const int* value )
{
	int32_t id = UniformId(name);
	return BindUniformI(id,value);
}

bool
vsMaterial::BindUniformColor( const vsString& name, const vsColor* value )
{
	int32_t id = UniformId(name);
	return BindUniformColor(id,value);
}

bool
vsMaterial::BindUniformVec3( const vsString& name, const vsVector3D* value )
{
	int32_t id = UniformId(name);
	return BindUniformVec3(id,value);
}

bool
vsMaterial::BindUniformVec4( const vsString& name, const vsVector4D* value )
{
	int32_t id = UniformId(name);
	return BindUniformVec4(id,value);
}

bool
vsMaterial::BindUniformMat4( const vsString& name, const vsMatrix4x4* value )
{
	int32_t id = UniformId(name);
	return BindUniformMat4(id, value);
}

float
vsMaterial::UniformF( int32_t id )
{
	if ( id >= m_uniformCount )
		return 0.f;
	if ( m_uniformValue[id].bound )
		return *(float*)m_uniformValue[id].bind;
	else
		return m_uniformValue[id].f32;
}

bool
vsMaterial::UniformB( int32_t id )
{
	if ( id >= m_uniformCount )
		return false;
	if ( m_uniformValue[id].bound )
		return *(bool*)m_uniformValue[id].bind;
	else
		return m_uniformValue[id].b;
}

int
vsMaterial::UniformI( int32_t id )
{
	if ( id >= m_uniformCount )
		return 0;
	if ( m_uniformValue[id].bound )
		return *(int*)m_uniformValue[id].bind;
	else
		return m_uniformValue[id].b;
}

vsVector4D
vsMaterial::UniformVec4( int32_t id )
{
	if ( id >= m_uniformCount )
		return vsVector4D();
	if ( m_uniformValue[id].bound )
		return *(vsVector4D*)m_uniformValue[id].bind;
	else
		return *(vsVector4D*)m_uniformValue[id].vec4;
}

vsMatrix4x4
vsMaterial::UniformMat4( int32_t id )
{
	if ( id >= m_uniformCount )
		return vsMatrix4x4();
	if ( m_uniformValue[id].bound )
		return *(vsMatrix4x4*)m_uniformValue[id].bind;

	return vsMatrix4x4();
}

bool
vsMaterial::MatchesForBatching( vsMaterial *other ) const
{
	if ( GetResource() != other->GetResource() )
		return false;

	//shouldn't happen?
	if ( m_uniformCount != other->m_uniformCount )
		return false;

	for (int i = 0; i < m_uniformCount; i++)
	{
		Value &v = m_uniformValue[i];
		Value &ov = other->m_uniformValue[i];

		if ( v.bound != ov.bound )
			return false;
		if ( v.bound )
		{
			if ( v.bind != ov.bind )
				return false;
		}
		else
		{
			switch( GetResource()->m_shader->GetUniform(i)->type )
			{
				case GL_FLOAT:
					if ( v.f32 != ov.f32 )
						return false;
					break;
				case GL_INT:
				case GL_BOOL:
				case GL_SAMPLER_2D:
				case GL_UNSIGNED_INT_SAMPLER_BUFFER:
				case GL_INT_SAMPLER_BUFFER:
				case GL_SAMPLER_BUFFER:
					if ( v.b != ov.b )
						return false;
					break;
				case GL_FLOAT_VEC3:
				case GL_FLOAT_VEC4:
					if ( v.vec4[0] != ov.vec4[0] ||
							v.vec4[1] != ov.vec4[1] ||
							v.vec4[2] != ov.vec4[2] ||
							v.vec4[3] != ov.vec4[3] )
						return false;
					break;
			}
		}
	}
	return true;
}
