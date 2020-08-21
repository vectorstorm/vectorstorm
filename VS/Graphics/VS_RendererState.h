//
//  VS_RendererState.h
//  VectorStorm
//
//  Created by Trevor Powell on 23/04/11.
//  Copyright 2011 VectorStorm. All rights reserved.
//

#ifndef VS_RENDERER_STATE_H
#define VS_RENDERER_STATE_H

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

template<typename T, typename U>
class StateSetter2
{
	T	m_nextValueA;
	U	m_nextValueB;
protected:
	T	m_valueA;
	U	m_valueB;

public:

	StateSetter2( const T& initialValueA, const U& initialValueB )
	{
		m_valueA = m_nextValueA = initialValueA;
		m_valueB = m_nextValueB = initialValueB;
	}

	virtual ~StateSetter2(){}

	void Set( const T &newValueA, const U &newValueB )
	{
		m_nextValueA = newValueA;
		m_nextValueB = newValueB;
	}

	virtual void DoFlush() = 0;

	void Flush()
	{
		if ( m_nextValueA != m_valueA || m_nextValueB != m_valueB )
		{
			m_valueA = m_nextValueA;
			m_valueB = m_nextValueB;
			DoFlush();
		}
	}
	void Force()
	{
		m_valueA = m_nextValueA;
		m_valueB = m_nextValueB;
		DoFlush();
	}
};

class vsRendererState
{
public:
	enum Bool
	{
		// Bool_AlphaTest,
		Bool_Blend,
		// Bool_ColorMaterial,
		Bool_CullFace,
		Bool_DepthTest,
		// Bool_Fog,
		// Bool_Lighting,
		Bool_Multisample,
		Bool_PolygonOffsetFill,
		Bool_StencilTest,
		Bool_ScissorTest,
		//Bool_Smooth,
		//Bool_Texture2D,
		Bool_DepthMask,
		ClientBool_VertexArray,
		ClientBool_NormalArray,
		ClientBool_ColorArray,
		ClientBool_TextureCoordinateArray,
		ClientBool_OtherArray,
		BOOL_COUNT
	};
	enum Float
	{
		FLOAT_COUNT
	};
	enum Float2
	{
		Float2_PolygonOffsetConstantAndFactor,
		FLOAT2_COUNT
	};
	enum Int
	{
		Int_CullFace,
		INT_COUNT
	};

private:
    StateSetter<bool>	*m_boolState[BOOL_COUNT];
    //StateSetter<float>	*m_floatState[FLOAT_COUNT];
    StateSetter2<float,float>	*m_float2State[FLOAT2_COUNT];
	StateSetter<int>	*m_intState[INT_COUNT];


public:

    vsRendererState();
    ~vsRendererState();

	void	SetBool( Bool key, bool value );
	//void	SetFloat( Float key, float value );
	void	SetFloat2( Float2 key, float valueA, float valueB);
	void	SetInt( Int key, int value );

	void	Flush();
	void	Force();

};

#endif // VS_RENDERER_STATE_H

