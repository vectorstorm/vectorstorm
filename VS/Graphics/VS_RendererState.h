//
//  VS_RendererState.h
//  VectorStorm
//
//  Created by Trevor Powell on 23/04/11.
//  Copyright 2011 VectorStorm. All rights reserved.
//

#ifndef VS_RENDERER_STATE_H
#define VS_RENDERER_STATE_H

#include "VS/Math/VS_Vector.h"

struct vsRendererStateBlock;

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
		// Bool_PrimitiveRestartFixedIndex,
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

	class SimpleBoolStateSetter
	{
	private:
		Bool	m_type;
		bool	m_nextValue;
		bool	m_value;

		void DoFlush();
	public:

		SimpleBoolStateSetter();
		void SetType( Bool type );
		void Set( bool newValue );
		bool Get() const;
		void Flush();
		void Force();
	};
	class SimpleIntStateSetter
	{
	private:
		Int m_type;
		int m_nextValue;
		int m_value;

		void DoFlush();
	public:

		SimpleIntStateSetter();
		void SetType( Int type );
		void Set( int newValue );
		int Get() const;
		void Flush();
		void Force();
	};
	class SimpleFloat2StateSetter
	{
	private:
		Float2 m_type;
		vsVector2D m_nextValue;
		vsVector2D m_value;

		void DoFlush();
	public:

		SimpleFloat2StateSetter();
		void SetType( Float2 type );
		void Set( const vsVector2D& newValue );
		const vsVector2D& Get() const;
		void Flush();
		void Force();
	};


    SimpleBoolStateSetter	m_boolState[BOOL_COUNT];
    //StateSetter<float>	*m_floatState[FLOAT_COUNT];
    SimpleFloat2StateSetter	m_float2State[FLOAT2_COUNT];
	SimpleIntStateSetter	m_intState[INT_COUNT];

public:

    vsRendererState();
    ~vsRendererState();

	static vsRendererState *Instance();

	void	SetBool( Bool key, bool value );
	bool	GetBool( Bool key ) const;
	//void	SetFloat( Float key, float value );
	void	SetFloat2( Float2 key, float valueA, float valueB);
	void	SetInt( Int key, int value );

	void	Flush();
	void	Force();

	vsRendererStateBlock StateBlock() const;
	void Apply( const vsRendererStateBlock& block );
};

// grabs a copy of a vsRendererState.
struct vsRendererStateBlock
{
    bool m_boolState[vsRendererState::BOOL_COUNT];
    vsVector2D m_float2State[vsRendererState::FLOAT2_COUNT];
	int m_intState[vsRendererState::INT_COUNT];
};

#endif // VS_RENDERER_STATE_H

