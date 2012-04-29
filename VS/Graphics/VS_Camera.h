/*
 *  VS_Camera.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 20/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_CAMERA_H
#define VS_CAMERA_H

#include "VS/Math/VS_Angle.h"
#include "VS/Math/VS_Frustum.h"
#include "VS/Math/VS_Transform.h"
#include "VS/Math/VS_Vector.h"

class vsBox3D;

class vsCamera2D
{
protected:
	vsTransform2D	m_transform;
	
public:
	
				vsCamera2D();
	virtual		~vsCamera2D();
	
	virtual void		Update( float /*timeStep*/ ) {}
	
	const vsVector2D &	GetPosition() { return m_transform.GetTranslation(); }
	const vsAngle &		GetAngle() { return m_transform.GetAngle(); }
	float				GetFOV() { return m_transform.GetScale().y; }
	
	void				SetPosition( const vsVector2D &pos ) { m_transform.SetTranslation( pos ); }
	void				SetAngle( const vsAngle &ang ) { m_transform.SetAngle( ang ); }
	void				SetFOV( float fov ) { m_transform.SetScale( vsVector2D( fov, fov ) ); }
	
		// this "GetCameraTransform()" function is giving an incorrect 'scale' value.  Need to fix it!
	const vsTransform2D &		GetCameraTransform() { return m_transform; }
	
	bool				IsPositionVisible( const vsVector2D &pos, float r=0.f );
	bool				WrapAround( vsVector2D &pos, float r=0.f );
};


class vsCamera3D
{
protected:
	vsTransform3D	m_transform;
	vsFrustum		m_frustum;
	float			m_fov;		// vertical FOV, in radians
	float			m_nearPlane;
	float			m_farPlane;
	
	void			UpdateFrustum();

public:
	
	vsCamera3D();
	virtual		~vsCamera3D();
	
	virtual void		Update( float /*timeStep*/ ) { UpdateFrustum(); }
	
	const vsVector3D &		GetPosition() const { return m_transform.GetTranslation(); }
	const vsQuaternion &	GetOrientation() const { return m_transform.GetRotation(); }
	const vsTransform3D &	GetTransform() const { return m_transform; }
	float					GetFOV() const { return m_fov; }
	float					GetNearPlane() const { return m_nearPlane; }
	float					GetFarPlane() const { return m_farPlane; }
	
	void				SetTransform( const vsTransform3D &t ) { m_transform = t; UpdateFrustum();}
	void				SetPosition( const vsVector4D &pos ) { m_transform.SetTranslation( pos ); UpdateFrustum();}
	void				SetOrientation( const vsQuaternion &quat ) { m_transform.SetRotation( quat ); UpdateFrustum();}
	void				SetFOV( float fov ) { m_fov = fov; UpdateFrustum();}
	void				SetNearPlane( float np ) { m_nearPlane = np; UpdateFrustum();}
	void				SetFarPlane( float fp ) { m_farPlane = fp; UpdateFrustum();}

	bool				IsPositionVisible( const vsVector3D &pos, float r=0.f ) const;
	bool				IsBox3DVisible( const vsBox3D &box ) const;
};


#endif // VS_CAMERA_H
