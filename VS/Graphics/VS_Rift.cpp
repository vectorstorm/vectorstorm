/*
 *  VS_Rift.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 15-07-2013.
 *  Copyright 2013 Trevor Powell.  All rights reserved.
 *
 */
#include "VS_Rift.h"
#include "VS_Screen.h"
#include "VS_System.h"
#include <OVR.h>
using namespace OVR;

Ptr<DeviceManager> pManager;
Ptr<HMDDevice>     pHMD;
Ptr<SensorDevice>  pSensor;
HMDInfo hmd;
SensorFusion FusionResult;
Util::Render::StereoConfig* pConfig = NULL;

vsRift::vsRift(int resX, int resY):
	m_hasHmd(false)
{
	System::Init(Log::ConfigureDefaultLog(LogMask_All));
	Ptr<DeviceManager> pManager = *DeviceManager::Create();
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();

	if (pHMD)
	{
		m_hasHmd = pHMD->GetDeviceInfo(&hmd);

		if ( m_hasHmd )
		{
			Util::Render::Viewport vp(0,0,resX,resY);
			pConfig = new Util::Render::StereoConfig(Util::Render::Stereo_LeftRight_Multipass, vp);
			pConfig->SetHMDInfo(hmd);
			//pConfig->GetEyeRenderParams(Util::Render::StereoEye_Left);
			// Configure proper Distortion Fit.
			// For 7" screen, fit to touch left side of the view, leaving a bit of invisible
			// screen on the top (saves on rendering cost).
			// For smaller screens (5.5"), fit to the top.
			if (hmd.HScreenSize > 0.0f)
			{
				if (hmd.HScreenSize > 0.140f) // 7"
					pConfig->SetDistortionFitPointVP(-1.0f, 0.0f);
				else
					pConfig->SetDistortionFitPointVP(0.0f, 1.0f);
			}
			pConfig->Set2DAreaFov(DegreeToRad(85.0f));

			m_monitorName    = hmd.DisplayDeviceName;
			m_eyeDistance    = hmd.InterpupillaryDistance;
			for ( int i = 0; i < 4; i++ )
			{
				m_distortionK[i] = hmd.DistortionK[i];
			}
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
	vsDelete(pConfig);
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

struct EyeDistortionParams
vsRift::GetDistortionParams(RiftEye eye)
{
	Util::Render::StereoEyeParams sep;
	if ( eye == RiftEye_Left )
		sep = pConfig->GetEyeRenderParams(Util::Render::StereoEye_Left);
	else
		sep = pConfig->GetEyeRenderParams(Util::Render::StereoEye_Right);

	struct EyeDistortionParams result;
	for ( int i = 0; i < 4; i++ )
	{
		result.K[i] = sep.pDistortion->K[i];
		result.ChromaticAberration[i] = sep.pDistortion->ChromaticAberration[i];
	}
	result.XCenterOffset = sep.pDistortion->XCenterOffset;
	result.YCenterOffset = sep.pDistortion->YCenterOffset;
	result.Scale = sep.pDistortion->Scale;
	if ( eye == RiftEye_Right )
	{
		result.XCenterOffset *= -1.f;
	}
	return result;
}

