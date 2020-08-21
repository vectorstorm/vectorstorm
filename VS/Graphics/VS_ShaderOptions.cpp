/*
 *  VS_ShaderOptions.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/06/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_ShaderOptions.h"

void
vsShaderOptions::SetBit(uint8_t bitId, bool bitValue)
{
	uint32_t bitMask = BIT(bitId);

	if ( bitValue )
		value |= bitMask;
	else
		value &= ~bitMask;

	mask |= bitMask;
}

void
vsShaderOptions::ClearBit(uint8_t bitId)
{
	mask &= ~BIT(bitId);
}
