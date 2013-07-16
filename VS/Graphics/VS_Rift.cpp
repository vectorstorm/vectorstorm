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
Ptr<SensorDevice>  pSensor;
HMDInfo hmd;
SensorFusion FusionResult;

vsRift::vsRift():
	m_hasHmd(false)
{
	System::Init(Log::ConfigureDefaultLog(LogMask_All));
	Ptr<DeviceManager> pManager = *DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();

	if (pHMD)
	{
		m_hasHmd = pHMD->GetDeviceInfo(&hmd);

		m_monitorName    = hmd.DisplayDeviceName;
		m_eyeDistance    = hmd.InterpupillaryDistance;
		for ( int i = 0; i < 4; i++ )
		{
			m_distortionK[i] = hmd.DistortionK[i];
		}

		pSensor = *pHMD->GetSensor();
	}
	else
	{
		pSensor = *pManager->EnumerateDevices<SensorDevice>().CreateDevice();
	}

	if (pSensor)
	{
		FusionResult.AttachToSensor(pSensor);
	}

	//DeviceEnumerator<HMDDevice> de = pManager->EnumerateDevices<HMDDevice>();
	//pHMD     = de.CreateDevice();
	//if (pHMD && pHMD->GetDeviceInfo(&hmd))
	//{
	//m_hasHmd = true;
	//m_monitorName    = hmd.DisplayDeviceName;
	//m_eyeDistance    = hmd.InterpupillaryDistance;
	//for ( int i = 0; i < 4; i++ )
	//{
	//m_distortionK[i] = hmd.DistortionK[i];
	//}
	//}
}

vsRift::~vsRift()
{
	pSensor.Clear();
	pHMD.Clear();
	pManager.Clear();
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

vsVector4D
vsRift::GetDistortionK() const
{
	return vsVector4D(
			m_distortionK[0],
			m_distortionK[1],
			m_distortionK[2],
			m_distortionK[3]
			);

}

