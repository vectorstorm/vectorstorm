/*
 *  VS_Scene.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 12/03/07.
 *  Copyright 2007 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Scene.h"

#include "VS_Camera.h"
#include "VS_DisplayList.h"
#include "VS_Entity.h"
#include "VS_RenderQueue.h"
#include "VS_Screen.h"
#include "VS_System.h"
//#include "VS_Transform.h"

#include "VS_OpenGL.h"

vsTransform2D	g_drawingCameraTransform = vsTransform2D::Zero;

#if defined(DEBUG_SCENE)

class vsDebugCamera : public vsCamera2D
{
public:
	vsDebugCamera() : vsCamera2D()
	{
	}

	void				Init()
	{
		SetFOV( (float)vsScreen::Instance()->GetHeight() );
		SetPosition( vsVector2D( vsScreen::Instance()->GetWidth() * 0.5f,
					vsScreen::Instance()->GetHeight() * 0.5f ) );
	}
};
static vsDebugCamera s_debugCamera;

void
vsScene::SetDebugCamera()
{
	s_debugCamera.Init();
	s_debugCamera.SetAspectRatio( vsScreen::Instance()->GetAspectRatio() );
	SetCamera2D( &s_debugCamera );
}

#endif // DEBUG_SCENE


vsScene::vsScene():
	m_queue( new vsRenderQueue( 3, 1024*200 ) ),
	m_entityList( new vsEntity ),
	m_defaultCamera( new vsCamera2D ),
	m_camera( NULL ),
	m_camera3D( NULL ),
	m_defaultCamera3D( new vsCamera3D ),
	m_fog( NULL ),
	m_viewport(),
	m_is3d( false ),
	m_cameraIsReference( false ),
	m_flatShading( false ),
	m_stencilTest( false ),
	m_hasViewport( false ),
	m_enabled( true )
{
	m_camera = m_defaultCamera;
	m_camera3D = m_defaultCamera3D;

	for ( int i = 0; i < MAX_SCENE_LIGHTS; i++ )
	{
		m_light[i] = NULL;
	}
}

vsScene::~vsScene()
{
	vsDelete( m_queue );
	vsDelete( m_defaultCamera3D );
	vsDelete( m_defaultCamera );
	vsDelete( m_entityList );
}

void
vsScene::SetCamera2D( vsCamera2D *camera, bool reference )
{
	if ( camera )
		m_camera = camera;
	else
		m_camera = m_defaultCamera;

	m_cameraIsReference = reference;
}

void
vsScene::SetCamera3D( vsCamera3D *camera, bool reference )
{
	Set3D(true);
	if ( camera )
		m_camera3D = camera;
	else
		m_camera3D = m_defaultCamera3D;

	m_cameraIsReference = reference;
}

float
vsScene::GetFOV()
{
	return m_camera->GetFOV();
}

void
vsScene::UpdateVideoMode()
{
	// TODO:  This is all really awkward;  it shouldn't be here.
	//
	// Cameras should *either* have an aspect ratio set on them explicitly by
	// clients (if the clients know what they're doing), *or* automatically take
	// the aspect ratio of the viewport they're rendering into.  We shouldn't
	// change them automatically based upon a resize event, in the former case,
	// and particularly shouldn't be changing them HERE in the latter case!
	// (That should happen at the time of render)

#if defined(DEBUG_SCENE)
	if ( m_camera == &s_debugCamera )
	{
		// reset debug camera parameters, to match new resolution.
		SetDebugCamera();
	}
#endif // DEBUG_SCENE

//	if ( m_camera3D )
//	{
//		m_camera3D->SetAspectRatio( vsScreen::Instance()->GetAspectRatio() );
//	}
//	if ( m_camera )
//	{
//		m_camera->SetAspectRatio( vsScreen::Instance()->GetAspectRatio() );
//	}
}

void
vsScene::SetViewport( const vsBox2D& viewport )
{
	m_viewport = viewport;
	m_hasViewport = true;
}

void
vsScene::ClearViewport()
{
	m_hasViewport = false;
}

void
vsScene::Update( float timeStep )
{
	if ( !IsEnabled() )
		return;

	vsEntity *entity = m_entityList->GetNext();
	vsEntity *next;
	while ( entity != m_entityList )
	{
		next = entity->GetNext();		// entities might remove themselves during their Update, so pre-cache the next entity

		entity->Update( timeStep );

		if ( entity->GetNext() == entity )
			entity = next;
		else
			entity = entity->GetNext();
	}

	if ( m_camera && !m_cameraIsReference )
	{
		m_camera->Update( timeStep );
	}
	if ( m_camera3D && !m_cameraIsReference )
	{
		m_camera3D->Update( timeStep );
	}
}

void
vsScene::Draw( vsDisplayList *list )
{
	if ( !IsEnabled() )
		return;
	// if ( m_flatShading )
	// {
	// 	list->FlatShading();
	// }
	// else
	// {
	// 	list->SmoothShading();
	// }

	if ( m_hasViewport )
	{
		list->SetViewport( m_viewport );
	}

	m_queue->StartRender(this);

	if ( m_is3d )
	{
		list->SetProjectionMatrix4x4( m_camera3D->GetProjectionMatrix() );
		m_queue->SetProjectionMatrix(  m_camera3D->GetProjectionMatrix() );
		m_queue->SetFOV( m_camera3D->GetFieldOfView() );
		//list->Set3DProjection( m_camera3D->GetFOV(), m_camera3D->GetNearPlane(), m_camera3D->GetFarPlane() );
		// list->SetCameraTransform( m_camera3D->GetTransform() );

		for ( int i = 0; i < MAX_SCENE_LIGHTS; i++ )
		{
			if ( m_light[i] != NULL )
			{
				list->Light( *m_light[i] );
			}
		}

		if ( m_fog )
		{
			list->Fog( *m_fog );
		}

	}
	else
	{
		g_drawingCameraTransform = m_camera->GetCameraTransform();
		list->SetProjectionMatrix4x4( m_camera->GetProjectionMatrix() );
		m_queue->SetProjectionMatrix(  m_camera->GetProjectionMatrix() );
	}
	list->SetWorldToViewMatrix4x4( m_queue->GetWorldToViewMatrix() );

	if ( m_stencilTest )
	{
		//list->ClearStencil();
		list->EnableStencil();
	}

	vsEntity *entity = m_entityList->GetNext();
	while ( entity != m_entityList )
	{
		if ( m_is3d || (!m_camera || entity->OnScreen( m_camera->GetCameraTransform() )) )
		{
			entity->Draw( m_queue );
		}
		entity = entity->GetNext();
	}

	m_queue->Draw(list);

	m_queue->EndRender();

	if ( m_stencilTest )
	{
		list->DisableStencil();
	}
	if ( m_hasViewport )
	{
		list->ClearViewport();
	}

	list->ClearLights();
	list->ClearFog();
	// list->SetMaterial(vsMaterial::White);
}

void
vsScene::RegisterEntityOnBottom( vsEntity *sprite )
{
	sprite->Extract();
	m_entityList->Append(sprite);
}

void
vsScene::RegisterEntityOnTop( vsEntity *sprite )
{
	sprite->Extract();
	m_entityList->Prepend(sprite);
}

void
vsScene::AddLight( vsLight *light )
{
	for ( int i = 0; i < MAX_SCENE_LIGHTS; i++ )
	{
		if ( m_light[i] == NULL )
		{
			m_light[i] = light;
			return;
		}
	}
}

void
vsScene::RemoveLight( vsLight *light )
{
	for ( int i = 0; i < MAX_SCENE_LIGHTS; i++ )
	{
		if ( m_light[i] == light )
		{
			m_light[i] = NULL;
		}
	}
}

/*
 vsColor
 vsScene::CalculateLightOnNormal( const vsVector3D &normal )
 {
 vsColor result(0.0f,0.0f,0.0f,1.f);

 for ( int i = 0; i < MAX_SCENE_LIGHTS; i++ )
 {
 if ( m_light[i] )
 {
 float luminance = normal.Dot(m_light[i]->m_position);

 result += m_light[i]
 }
 }
 }*/

vsEntity *
vsScene::FindEntityAtPosition( const vsVector2D &pos )
{
	if ( !IsEnabled() )
		return NULL;
	vsEntity *result = NULL;

	vsEntity *entity = m_entityList->GetPrev();
	while ( !result && entity != m_entityList )
	{
		result = entity->FindEntityAtPosition(pos);

		entity = entity->GetPrev();
	}

	return result;
}

vsVector2D
vsScene::GetCenter()
{
	vsVector2D pos = vsVector2D::Zero;

	vsTransform2D worldToCamera;
	worldToCamera.SetTranslation( m_camera->GetPosition() );
	worldToCamera.SetAngle( m_camera->GetAngle() );

	pos = worldToCamera.ApplyTo(pos);

	return pos;
}

vsVector2D
vsScene::GetCorner(bool bottom, bool right)
{
	vsVector2D pos = vsVector2D::Zero;
	// okay.  First, let's start by grabbing the coordinate of our requested corner,
	// as though we had no camera.  Later on, we'll correct for the camera.

	float fov = GetFOV();			// fov is measured VERTICALLY.  So our height is FOV.
	float halfFov = 0.5f * fov;		// since we assume that '0,0' is in the middle, our coords vertically are [-halfFov .. halfFov]

	if ( bottom )
		pos.y = halfFov;
	else	// top
		pos.y = -halfFov;

	// now, to figure out where the edge is, we need to know our screen aspect
	// ratio, which is the ratio of horizontal pixels to vertical pixels.  Note
	// that we're going to use the CAMERA's recorded aspect ratio, not the ACTUAL
	// aspect ratio, since the two may differ.  When we tell people corner coordinates,
	// though, they're likely to want to use them for rendering from our current
	// camera, so let's return values in that coordinate space.
	float aspectRatio = m_camera->GetAspectRatio();

	if ( right )
		pos.x = halfFov * aspectRatio;
	else	// left
		pos.x = -halfFov * aspectRatio;


	// Okay!  Now we have the corner of our screen.  Now we just need to figure out where this camera-relative coordinate sits in world-space
	// to do this, we take the world-to-camera transform off the camera, and then apply its inverse to our position.

	vsTransform2D worldToCamera;
	worldToCamera.SetTranslation( m_camera->GetPosition() );
	worldToCamera.SetAngle( m_camera->GetAngle() );

	pos = worldToCamera.ApplyTo(pos);

	return pos;
}


