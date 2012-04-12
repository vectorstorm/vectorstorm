/*
 *  VS_EulerAngles.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 14/12/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_EULERANGLES_H
#define VS_EULERANGLES_H

#include "VS_Angle.h"

class vsEulerAngles
{
public:
	
	vsAngle pitch;		// rotation around x
	vsAngle yaw;		// rotation around y
	vsAngle bank;		// rotation around z.  Also called 'roll', sometimes.
	
	vsEulerAngles();
	vsEulerAngles( const vsAngle &pitch_in, const vsAngle &yaw_in, const vsAngle &bank_in ) { Set(pitch_in, yaw_in, bank_in); }
	
	void Set(const vsAngle &pitch, const vsAngle &yaw, const vsAngle &bank);
};

#endif // VS_EULERANGLES_H

