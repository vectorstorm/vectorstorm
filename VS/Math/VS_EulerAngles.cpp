/*
 *  VS_EulerAngles.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 14/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_EulerAngles.h"

vsEulerAngles::vsEulerAngles():
	pitch(0.f),
	yaw(0.f),
	bank(0.f)
{
}

void
vsEulerAngles::Set( const vsAngle &pitch_in, const vsAngle &yaw_in, const vsAngle &bank_in )
{
	pitch = pitch_in;
	yaw = yaw_in;
	bank = bank_in;
}