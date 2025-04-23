//
//  VS_RendererState.cpp
//  MMORPG2
//
//  Created by Trevor Powell on 23/04/11.
//  Copyright 2011 VectorStorm. All rights reserved.
//

#include "VS_RendererState.h"
#include "VS_OpenGL.h"

namespace
{
	vsRendererState *s_instance = nullptr;
}
vsRendererState *
vsRendererState::Instance()
{
	return s_instance;
}


vsRendererState::vsRendererState()
{
	vsAssert( s_instance == nullptr, "More than one renderer state created??" );
	s_instance = this;

	for ( int i = 0; i < BOOL_COUNT; i++ )
		m_boolState[i].SetType( (Bool)i );
	for ( int i = 0; i < INT_COUNT; i++ )
		m_intState[i].SetType( (Int)i );
	for ( int i = 0; i < FLOAT2_COUNT; i++ )
		m_float2State[i].SetType( (Float2)i );

	m_intState[Int_CullFace].Set( GL_FRONT );
}

vsRendererState::~vsRendererState()
{
}

void
vsRendererState::SetBool( vsRendererState::Bool key, bool value )
{
	m_boolState[key].Set(value);

	//Flush();
}

bool
vsRendererState::GetBool( vsRendererState::Bool key ) const
{
	return m_boolState[key].Get();
}

/*void
vsRendererState::SetFloat( vsRendererState::Float key, float value )
{
	m_floatState[key]->Set(value);

	//Flush();
}*/

void
vsRendererState::SetFloat2( vsRendererState::Float2 key, float valueA, float valueB )
{
	m_float2State[key].Set( vsVector2D(valueA, valueB) );
}

void
vsRendererState::SetInt( vsRendererState::Int key, int value )
{
	m_intState[key].Set(value);

	//Flush();
}

void
vsRendererState::Flush()
{
	for ( int i = 0; i < BOOL_COUNT; i++ )
	{
		m_boolState[i].Flush();
	}
    for ( int i = 0; i < INT_COUNT; i++ )
    {
        m_intState[i].Flush();
    }
	/*for ( int i = 0; i < FLOAT_COUNT; i++ )
	{
		m_floatState[i]->Flush();
	}*/
	for ( int i = 0; i < FLOAT2_COUNT; i++ )
	{
		m_float2State[i].Flush();
	}
}


void
vsRendererState::Force()
{
	for ( int i = 0; i < BOOL_COUNT; i++ )
	{
		m_boolState[i].Force();
	}
    for ( int i = 0; i < INT_COUNT; i++ )
    {
        m_intState[i].Force();
    }
	for ( int i = 0; i < FLOAT2_COUNT; i++ )
	{
		m_float2State[i].Force();
	}
	/*for ( int i = 0; i < FLOAT_COUNT; i++ )
	{
		m_floatState[i]->Force();
	}*/
}

vsRendererStateBlock
vsRendererState::StateBlock() const
{
	vsRendererStateBlock result;
	for ( int i = 0; i < BOOL_COUNT; i++ )
	{
		result.m_boolState[i] = m_boolState[i].Get();
	}
    for ( int i = 0; i < INT_COUNT; i++ )
    {
		result.m_intState[i] = m_intState[i].Get();
    }
    for ( int i = 0; i < FLOAT2_COUNT; i++ )
    {
		result.m_float2State[i] = m_float2State[i].Get();
    }
	return result;
}

void
vsRendererState::Apply( const vsRendererStateBlock& block )
{
	for ( int i = 0; i < BOOL_COUNT; i++ )
	{
		m_boolState[i].Set( block.m_boolState[i] );
	}
    for ( int i = 0; i < INT_COUNT; i++ )
    {
		m_intState[i].Set( block.m_intState[i] );
    }
    for ( int i = 0; i < FLOAT2_COUNT; i++ )
    {
		m_float2State[i].Set( block.m_float2State[i] );
    }
}

void glEnableDisable( int tag, bool enable )
{
	if ( enable )
		glEnable(tag);
	else
		glDisable(tag);
}
// void glClientStateSetter( int attribute, bool enable )
// {
// 	if ( enable )
// 		glEnableVertexAttribArray(attribute);
// 	else
// 		glDisableVertexAttribArray(attribute);
// }

vsRendererState::SimpleBoolStateSetter::SimpleBoolStateSetter()
{
	m_type = BOOL_COUNT;
	m_value = m_nextValue = false;
}

void
vsRendererState::SimpleBoolStateSetter::SetType( Bool type ) { m_type = type; }

void
vsRendererState::SimpleBoolStateSetter::Set( bool newValue )
{
	m_nextValue = newValue;
}

bool
vsRendererState::SimpleBoolStateSetter::Get() const
{
	return m_nextValue;
}

void
vsRendererState::SimpleBoolStateSetter::Flush()
{
	if ( m_nextValue != m_value )
	{
		m_value = m_nextValue;
		DoFlush();
	}
}

void
vsRendererState::SimpleBoolStateSetter::Force()
{
	m_value = m_nextValue;
	DoFlush();
}

void
vsRendererState::SimpleBoolStateSetter::DoFlush()
{
	switch( m_type )
	{
		case Bool_Blend:
			glEnableDisable(GL_BLEND, m_value);
			break;
		case Bool_CullFace:
			glEnableDisable(GL_CULL_FACE, m_value);
			break;
		case Bool_DepthTest:
			glEnableDisable(GL_DEPTH_TEST, m_value);
			break;
		case Bool_Multisample:
			glEnableDisable(GL_MULTISAMPLE, m_value);
			break;
		case Bool_PolygonOffsetFill:
			glEnableDisable(GL_POLYGON_OFFSET_FILL, m_value);
			break;
		case Bool_StencilTest:
			glEnableDisable(GL_STENCIL_TEST, m_value);
			break;
		case Bool_ScissorTest:
			glEnableDisable(GL_SCISSOR_TEST, m_value);
			break;
		case Bool_DepthMask:
			glDepthMask( m_value );
			break;
		// case ClientBool_VertexArray:
		// 	glClientStateSetter( 0, m_value );
		// 	break;
		// case ClientBool_TextureCoordinateArray:
		// 	glClientStateSetter( 1, m_value );
		// 	break;
		// case ClientBool_NormalArray:
		// 	glClientStateSetter( 2, m_value );
		// 	break;
		// case ClientBool_ColorArray:
		// 	glClientStateSetter( 3, m_value );
		// 	break;
		// case ClientBool_OtherArray:
		// 	glClientStateSetter( 4, m_value );
		// 	break;
		case BOOL_COUNT:
			break;
	}
}

vsRendererState::SimpleIntStateSetter::SimpleIntStateSetter()
{
	m_type = INT_COUNT;
	m_value = m_nextValue = false;
}

void
vsRendererState::SimpleIntStateSetter::SetType( Int type )
{
	m_type = type;
}

void
vsRendererState::SimpleIntStateSetter::Set( int newValue )
{
	m_nextValue = newValue;
}

int
vsRendererState::SimpleIntStateSetter::Get() const
{
	return m_nextValue;
}

void
vsRendererState::SimpleIntStateSetter::Flush()
{
	if ( m_nextValue != m_value )
	{
		m_value = m_nextValue;
		DoFlush();
	}
}

void
vsRendererState::SimpleIntStateSetter::Force()
{
	m_value = m_nextValue;
	DoFlush();
}

void
vsRendererState::SimpleIntStateSetter::DoFlush()
{
	switch( m_type )
	{
		case Int_CullFace:
			glCullFace( m_value ) ;
			break;
		case INT_COUNT:
			break;
	}
}

vsRendererState::SimpleFloat2StateSetter::SimpleFloat2StateSetter()
{
	m_type = FLOAT2_COUNT;
	m_value = m_nextValue = vsVector2D::Zero;
}

void
vsRendererState::SimpleFloat2StateSetter::SetType( Float2 type )
{
	m_type = type;
}

void
vsRendererState::SimpleFloat2StateSetter::Set( const vsVector2D& newValue )
{
	m_nextValue = newValue;
}

const vsVector2D&
vsRendererState::SimpleFloat2StateSetter::Get() const
{
	return m_nextValue;
}

void
vsRendererState::SimpleFloat2StateSetter::Flush()
{
	if ( m_nextValue != m_value )
	{
		m_value = m_nextValue;
		DoFlush();
	}
}

void
vsRendererState::SimpleFloat2StateSetter::Force()
{
	m_value = m_nextValue;
	DoFlush();
}

void
vsRendererState::SimpleFloat2StateSetter::DoFlush()
{
	switch( m_type )
	{
		case Float2_PolygonOffsetConstantAndFactor:
			glPolygonOffset( m_value.y, m_value.x ) ;
			break;
		case FLOAT2_COUNT:
			break;
	}
}

