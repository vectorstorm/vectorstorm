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

static int	c_codeMaterialCount = 0;

vsMaterial *vsMaterial::White = NULL;

vsMaterial::vsMaterial():
	vsCacheReference<vsMaterialInternal>(vsFormatString("CodeMaterial%02d", c_codeMaterialCount++)),
	m_uniformValue(NULL)
{
	SetupParameters();
}

vsMaterial::vsMaterial( const vsString &name ):
	vsCacheReference<vsMaterialInternal>(name),
	m_uniformValue(NULL)
{
	SetupParameters();
}

vsMaterial::vsMaterial( vsMaterial *other ):
	vsCacheReference<vsMaterialInternal>(other->GetResource()->GetName()),
	m_uniformValue(NULL)
{
	SetupParameters();
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
		m_uniformValue = new Value[ GetResource()->m_shader->GetUniformCount() ];
		memset(m_uniformValue, 0, sizeof(Value) * GetResource()->m_shader->GetUniformCount());
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
	if ( id >= 0 )
		m_uniformValue[id].f32 = value;
}

void
vsMaterial::SetUniformB( int32_t id, bool value )
{
	if ( id >= 0 )
		m_uniformValue[id].b = value;
}

