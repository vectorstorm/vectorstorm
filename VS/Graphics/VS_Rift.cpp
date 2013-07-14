/*
 *  VS_Rift.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15-07-2013.
 *  Copyright 2013 Trevor Powell.  All rights reserved.
 *
 */
#include "VS_Rift.h"
#include <OVR.h>
using namespace OVR;

Ptr<DeviceManager> pManager;
Ptr<HMDDevice>     pHMD;
HMDInfo hmd;

vsRift::vsRift()
{
	System::Init(Log::ConfigureDefaultLog(LogMask_All));
	pManager = *DeviceManager::Create();
	pHMD     = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
	if (pHMD->GetDeviceInfo(&hmd))
	{
		m_monitorName    = hmd.DisplayDeviceName;
		m_eyeDistance    = hmd.InterpupillaryDistance;
		for ( int i = 0; i < 4; i++ )
		{
			m_distortionK[i] = hmd.DistortionK[i];
		}
	}
}

vsRift::~vsRift()
{
	System::Destroy();
}

float
vsRift::GetLensSeparationDistance() const
{
	return hmd.LensSeparationDistance;
}

float
vsRift::GetHScreenSize() const
{
	return hmd.HScreenSize;
}

float
vsRift::GetVScreenSize() const
{
	return hmd.VScreenSize;
}

float
vsRift::GetDistanceBetweenPupils() const
{
	return hmd.InterpupillaryDistance;
}
