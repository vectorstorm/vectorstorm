//
//  VS_RendererState.h
//  MMORPG2
//
//  Created by Trevor Powell on 23/04/11.
//  Copyright 2011 VectorStorm. All rights reserved.
//

#ifndef VS_RENDERER_STATE_H
#define VS_RENDERER_STATE_H

#include "VS_Array.h"

template<typename T>
class StateSetter
{
	T	m_nextValue;
protected:
	T	m_value;

public:

	StateSetter( const T& initialValue )
	{
		m_value = m_nextValue = initialValue;
	}

	virtual ~StateSetter(){}

	void Set( const T &newValue )
	{
		m_nextValue = newValue;
	}

	virtual void DoFlush() = 0;

	void Flush()
	{
		if ( m_nextValue != m_value )
		{
			m_value = m_nextValue;
			DoFlush();
		}
	}
	void Force()
	{
		m_value = m_nextValue;
		DoFlush();
	}
};

class vsRendererState
{
public:
	enum Bool
	{
		Bool_AlphaTest,
		Bool_Blend,
		Bool_ColorMaterial,
		Bool_CullFace,
		Bool_DepthTest,
		Bool_Fog,
		Bool_Lighting,
		Bool_LineSmooth,
		Bool_Multisample,
		Bool_PolygonOffsetFill,
#if !TARGET_OS_IPHONE
		Bool_PolygonSmooth,
#endif
		Bool_StencilTest,
		//Bool_Smooth,
		//Bool_Texture2D,
		Bool_DepthMask,
		ClientBool_VertexArray,
		ClientBool_NormalArray,
		ClientBool_ColorArray,
		ClientBool_TextureCoordinateArray,
		BOOL_COUNT
	};
	enum Float
	{
		Float_AlphaThreshhold,
		Float_PolygonOffsetUnits,
		FLOAT_COUNT
	};
	enum Int
	{
		Int_CullFace,
		INT_COUNT
	};

private:
    StateSetter<bool>	*m_boolState[BOOL_COUNT];
    StateSetter<float>	*m_floatState[FLOAT_COUNT];
	StateSetter<int>	*m_intState[INT_COUNT];


public:

    vsRendererState();
    ~vsRendererState();

	void	SetBool( Bool key, bool value );
	void	SetFloat( Float key, float value );
	void	SetInt( Int key, int value );

	void	Flush();
	void	Force();

};

#endif // VS_RENDERER_STATE_H

