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

vsScene *		vsScene::s_current = NULL;

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
	m_hasViewport( false )
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
	//	delete m_displayList;
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
	s_current = this;

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

	s_current = NULL;
}

void
vsScene::Draw( vsDisplayList *list, const DrawSettings& s )
{
	s_current = this;

	//	m_displayList->Clear();

	if ( m_flatShading )
	{
		list->FlatShading();
	}
	else
	{
		list->SmoothShading();
	}

	if ( s.useCustomViewport )
	{
		list->SetViewport( s.customViewport );
	}
	else if ( m_hasViewport )
	{
		// TODO:  If the scene has a viewport, we should embed that viewport
		// inside the custom one (if there is a custom one)
		list->SetViewport( m_viewport );
	}

	if ( m_is3d )
	{
		vsMatrix4x4 proj = m_camera3D->GetProjectionMatrix();
		vsTransform3D trans = m_camera3D->GetTransform();
		if ( s.horizontalPixelOffset != 0.f)
		{
			vsMatrix4x4 H = vsMatrix4x4::Identity;
			H.w.x = s.horizontalPixelOffset;
			proj = H * proj;
		}
		if ( s.cameraLocalOffset != vsVector3D::Zero )
		{
			vsVector3D localOffset = trans.GetRotation().ApplyTo(s.cameraLocalOffset);
			trans.SetTranslation( trans.GetTranslation() + localOffset );
		}
		list->SetProjectionMatrix4x4(proj);
		list->SetCameraProjection(trans);

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
		vsMatrix4x4 proj = m_camera->GetOrthoMatrix( vsSystem::GetScreen()->GetAspectRatio() );
		g_drawingCameraTransform = m_camera->GetCameraTransform();
		if ( s.horizontalPixelOffset != 0.f)
		{
			vsMatrix4x4 H = vsMatrix4x4::Identity;
			H.w.x = s.horizontalPixelOffset;
			proj = H * proj;
		}
		list->SetProjectionMatrix4x4( proj );
		//list->SetCameraTransform( m_camera->GetCameraTransform() );
	}

	if ( m_stencilTest )
	{
		//list->ClearStencil();
		list->EnableStencil();
	}

	m_queue->StartRender(this);

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
	list->SetMaterial(vsMaterial::White);

	s_current = NULL;
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

	// now, to figure out where the edge is, we need to know our screen aspect ratio, which is the ratio of horizontal pixels to vertical pixels.
	float aspectRatio = vsSystem::GetScreen()->GetAspectRatio();

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

#if defined(DEBUG_SCENE)

class vsDebugCamera : public vsCamera2D
{
public:
	vsDebugCamera() : vsCamera2D()
	{
	}

	void				Init()
	{
		SetFOV( 1.0f );
		SetPosition( vsVector2D(0.5f * vsSystem::GetScreen()->GetAspectRatio() , 0.5f) );
	}
};

static vsDebugCamera s_debugCamera;

void
vsScene::SetDebugCamera()
{
	s_debugCamera.Init();
	SetCamera2D( &s_debugCamera );
}

#endif // DEBUG_SCENE


