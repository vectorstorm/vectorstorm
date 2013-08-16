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
	float				GetFieldOfView() { return m_transform.GetScale().y; }
	float				GetFOV() { return GetFieldOfView(); }

	void				SetPosition( const vsVector2D &pos ) { m_transform.SetTranslation( pos ); }
	void				SetAngle( const vsAngle &ang ) { m_transform.SetAngle( ang ); }
	void				SetFieldOfView( float fov ) { m_transform.SetScale( vsVector2D( fov, fov ) ); }
	void				SetFOV( float fov ) { SetFieldOfView(fov); }

		// this "GetCameraTransform()" function is giving an incorrect 'scale' value.  Need to fix it!
	const vsTransform2D &		GetCameraTransform() { return m_transform; }

	bool				IsPositionVisible( const vsVector2D &pos, float r=0.f );
	bool				WrapAround( vsVector2D &pos, float r=0.f );
};


class vsCamera3D
{
public:
	enum ProjectionType
	{
		PT_Perspective,
		PT_Orthographic
	};
protected:

	vsTransform3D	m_transform;
	vsFrustum		m_frustum;
	float			m_fov;		// vertical FOV, in radians
	float			m_nearPlane;
	float			m_farPlane;
	float			m_aspectRatio;
	ProjectionType	m_type;

	void			UpdateFrustum();

public:

	vsCamera3D( ProjectionType t = PT_Perspective );
	virtual		~vsCamera3D();

	virtual void		Update( float /*timeStep*/ ) { UpdateFrustum(); }

	const vsVector3D &		GetPosition() const { return m_transform.GetTranslation(); }
	const vsQuaternion &	GetOrientation() const { return m_transform.GetRotation(); }
	const vsTransform3D &	GetTransform() const { return m_transform; }
	float					GetFieldOfView() const { return m_fov; }
	float					GetFOV() const { return GetFieldOfView(); } // deprecated
	float					GetNearPlane() const { return m_nearPlane; }
	float					GetFarPlane() const { return m_farPlane; }
	float				GetAspectRatio() const { return m_aspectRatio; }

	void				SetProjectionType( ProjectionType t ) { m_type = t; }
	ProjectionType		GetProjectionType() const { return m_type; }

	vsMatrix4x4			GetProjectionMatrix();

	void				SetTransform( const vsTransform3D &t ) { m_transform = t; UpdateFrustum();}
	void				SetPosition( const vsVector4D &pos ) { m_transform.SetTranslation( pos ); UpdateFrustum();}
	void				SetOrientation( const vsQuaternion &quat ) { m_transform.SetRotation( quat ); UpdateFrustum();}
	void				SetFieldOfView( float fov ) { m_fov = fov; UpdateFrustum();}
	void				SetFOV( float fov ) { SetFieldOfView(fov); } // deprecated
	void				SetNearPlane( float np ) { m_nearPlane = np; UpdateFrustum();}
	void				SetFarPlane( float fp ) { m_farPlane = fp; UpdateFrustum();}
	void				SetAspectRatio( float ar ) { m_aspectRatio = ar; UpdateFrustum(); }

	// Convenience function, which can be used instead of SetOrientation, if you
	// just want the camera to look at a particular position.  Optionally, you
	// may supply an 'upDirection' value to specify which direction should appear
	// as 'up' from the camera's point of view.  (If one is not specified, the
	// positive Y axis will be used)
	void				LookAt( const vsVector3D &lookat, const vsVector3D &upDirection = vsVector3D::YAxis );

	bool				IsPositionVisible( const vsVector3D &pos, float r=0.f ) const;
	bool				IsBox3DVisible( const vsBox3D &box ) const;
};


#endif // VS_CAMERA_H
