/*
 *  VS_ShaderOptions.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/06/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_SHADEROPTIONS_H
#define VS_SHADEROPTIONS_H

struct vsShaderOptions
{
	uint32_t mask;
	uint32_t value;

	vsShaderOptions(): mask(0), value(0) {}

	void SetBit(uint8_t bitId, bool bitValue);
	void ClearBit(uint8_t bitId);
};

#endif // VS_SHADEROPTIONS_H

