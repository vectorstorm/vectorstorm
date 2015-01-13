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

static int	c_codeMaterialCount = 0;

vsMaterial *vsMaterial::White = NULL;

vsMaterial::vsMaterial():
	vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++)),
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

bool
vsMaterial::BindUniformF( int32_t id, float* value )
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
vsMaterial::BindUniformB( int32_t id, bool* value )
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
vsMaterial::BindUniformF( const vsString& name, float* value )
{
	int32_t id = UniformId(name);
	return BindUniformF(id,value);
}

bool
vsMaterial::BindUniformB( const vsString& name, bool* value )
{
	int32_t id = UniformId(name);
	return BindUniformB(id,value);
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

