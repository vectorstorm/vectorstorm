/*
 *  Wedge.cpp
 *  VectorStormIPhone
 *
 *  Created by Trevor Powell on 19/09/08.
 *  Copyright 2008 PanicKitten Softworks. All rights reserved.
 *
 */

#include "Wedge.h"

#include "VS_System.h"
#include "VS_Vector.h"
#include "Core.h"
#include "CORE_GameRegistry.h"

vsSystem *gSystem = NULL;

#define MAX_TOUCHES (12)

static vsVector2D acceleration = vsVector2D::Zero;
int	resX, resY;

static void* touchID[MAX_TOUCHES];
static vsVector2D touch[MAX_TOUCHES];
static bool touching[MAX_TOUCHES];
static int orientation = Orientation_Unknown;

void Init()
{
	for ( int i = 0; i < MAX_TOUCHES; i++ )
	{
		touchID[i] = NULL;
		touching[i] = false;
		touch[i] = vsVector2D::Zero;
	}
	
	gSystem = new vsSystem();
	
	//  Create our game
	
	core::Init();
	core::SetGame( coreGameRegistry::GetMainMenu() );
	
	core::PreGoOneFrame();
}

void Draw()
{
	core::GoOneFrame( (1.0f / 60.f) );
}

void Deinit()
{
	core::Deinit();
	
	delete gSystem;
}

int FindTouch(void* identifier)
{
	for ( int i = 0; i < MAX_TOUCHES; i++ )
	{
		if ( touching[i] && touchID[i] == identifier )
			return i;
	}
	
	// we don't have this one yet;  pick an unused slot.
	for ( int i = 0; i < MAX_TOUCHES; i++ )
	{
		if ( touching[i] == false )
		{
			touchID[i] = identifier;
			return i;
		}
	}
	
	assert(0);
	return 0;
}

bool GetTouch(int n)
{
	return touching[n];
}

float GetTouchX(int n)
{
	return touch[n].x;
}

float GetTouchY(int n)
{
	return touch[n].y;
}

void SetTouch(void* identifier, float x, float y)
{
	int n = FindTouch(identifier);
	
	touch[n].Set(x,y);
	touching[n] = true;
}

void SetNoTouch(void* identifier)
{
	int n = FindTouch(identifier);
	
	touching[n] = false;
}

void SetRes( int x, int y )
{
	resX = x;
	resY = y;
}

void SetAcceleration(float x, float y)
{
	acceleration.Set(x,-y);	// accelerometer y is inverse from VectorStorm axes.
}

void	SetDeviceOrientation( int orient )
{
	orientation = orient;
}

int GetDeviceOrientation()
{
	return orientation;
}

