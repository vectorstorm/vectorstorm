/*
 *  Wedge.h
 *  VectorStormIPhone
 *
 *  Created by Trevor Powell on 19/09/08.
 *  Copyright 2008 PanicKitten Softworks. All rights reserved.
 *
 */

#ifndef WEDGE_H
#define WEDGE_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
	
	enum 
	{
		Orientation_Unknown,
		Orientation_Portrait,            // Device oriented vertically, home button on the bottom
		Orientation_PortraitUpsideDown,  // Device oriented vertically, home button on the top
		Orientation_LandscapeLeft,       // Device oriented horizontally, home button on the right
		Orientation_LandscapeRight,      // Device oriented horizontally, home button on the left
		Orientation_FaceUp,              // Device oriented flat, face up
		Orientation_FaceDown             // Device oriented flat, face down
	};
	
	void Init();
	void Draw();
	void Deinit();
	
	bool	GetTouch(int n);
	float	GetTouchX(int n);
	float	GetTouchY(int n);
	
	void	SetTouch(void* touch, float x, float y);
	void	SetNoTouch(void* touch);
	
	void	SetAcceleration(float x, float y);
	void	SetRes( int x, int y );
	
	void	SetDeviceOrientation( int orient );
	int GetDeviceOrientation();
	
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // WEDGE_H