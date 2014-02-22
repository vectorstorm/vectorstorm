/*
 *  VS_Headers.h
 *  VectorStormDLL
 *
 *  Created by Trevor Powell on 3/05/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#ifndef VS_HEADERS_H
#define VS_HEADERS_H

#include <Core/Core.h>
#include <Core/CORE_Game.h>
#include <Core/CORE_GameMode.h>
#include <Core/CORE_GameRegistry.h>

#include <Files/VS_File.h>
#include <Files/VS_Record.h>
#include <Files/VS_Token.h>

#include <Memory/VS_Heap.h>
#include <Memory/VS_Serialiser.h>
#include <Memory/VS_Store.h>

#include <VS/Utils/VS_AutomaticInstanceList.h>
#include <VS/Utils/VS_LinkedList.h>
#include <VS/Utils/VS_Singleton.h>
#include <VS/Utils/VS_StrongPointer.h>
#include <VS/Utils/VS_StrongPointerTarget.h>
#include <VS/Utils/VS_WeakPointer.h>
#include <VS/Utils/VS_WeakPointerTarget.h>

#include <Sound/VS_Music.h>
#include <Sound/VS_SoundSample.h>
#include <Sound/VS_SoundSystem.h>

#include <Physics/VS_CollisionObject.h>
#include <Physics/VS_PhysicsEmitter.h>
#include <Physics/VS_PhysicsSprite.h>
#include <Physics/VS_CollisionSystem.h>
#include <Input/VS_Input.h>
#include <Utils/VS_TimerSystem.h>

#include <Utils/VS_Menu.h>
#include <Utils/VS_Spring.h>
#include <Utils/VS_Timer.h>
#include <Utils/VS_Tween.h>

#include <VS/Graphics/VS_Camera.h>
#include <VS/Graphics/VS_Color.h>
#include <VS/Graphics/VS_DisplayList.h>
#include <VS/Graphics/VS_DynamicMaterial.h>
#include <VS/Graphics/VS_Entity.h>
#include <VS/Graphics/VS_Fog.h>
#include <VS/Graphics/VS_Fragment.h>
#include <VS/Graphics/VS_Font.h>
#include <VS/Graphics/VS_Light.h>
#include <VS/Graphics/VS_Material.h>
#include <VS/Graphics/VS_Mesh.h>
#include <VS/Graphics/VS_Model.h>
#include <VS/Graphics/VS_RenderBuffer.h>
#include <VS/Graphics/VS_RenderPipeline.h>
#include <VS/Graphics/VS_RenderPipelineStage.h>
#include <VS/Graphics/VS_RenderPipelineStageBlit.h>
#include <VS/Graphics/VS_RenderPipelineStageScenes.h>
#include <VS/Graphics/VS_RenderQueue.h>
#include <VS/Graphics/VS_RenderTarget.h>
#include <VS/Graphics/VS_Renderer.h>
#include <VS/Graphics/VS_Scene.h>
#include <VS/Graphics/VS_Screen.h>
#include <VS/Graphics/VS_Shader.h>
#include <VS/Graphics/VS_ShaderSuite.h>
#include <VS/Graphics/VS_Sprite.h>
#include <VS/Graphics/VS_Texture.h>
#include <VS/Graphics/VS_TextureManager.h>

#include <VS/Network/VS_NetClient.h>
#include <VS/Network/VS_Socket.h>
#include <VS/Network/VS_SocketTCP.h>

#include <VS/Math/VS_Angle.h>
#include <VS/Math/VS_Box.h>
#include <VS/Math/VS_EulerAngles.h>
#include <VS/Math/VS_Math.h>
#include <VS/Math/VS_Matrix.h>
#include <VS/Math/VS_Perlin.h>
#include <VS/Math/VS_Quaternion.h>
#include <VS/Math/VS_Random.h>
#include <VS/Math/VS_Spline.h>
#include <VS/Math/VS_Transform.h>
#include <VS/Math/VS_Vector.h>

#include <VS/Threads/VS_Mutex.h>
#include <VS/Threads/VS_Semaphore.h>
#include <VS/Threads/VS_Task.h>

#include <VS/Utils/VS_Array.h>
#include <VS/Utils/VS_ArrayStore.h>
#include <VS/Utils/VS_Debug.h>
#include <VS/Utils/VS_Factory.h>
#include <VS/Utils/VS_HashTable.h>
#include <VS/Utils/VS_HashTableStore.h>
#include <VS/Utils/VS_Image.h>
#include <VS/Utils/VS_LocalisationTable.h>
#include <VS/Utils/VS_MeshMaker.h>
#include <VS/Utils/VS_LinkedList.h>
#include <VS/Utils/VS_LinkedListStore.h>
#include <VS/Utils/VS_Log.h>
#include <VS/Utils/VS_Octree.h>
#include <VS/Utils/VS_PointOctree.h>
#include <VS/Utils/VS_Pool.h>
#include <VS/Utils/VS_Preferences.h>
#include <VS/Utils/VS_Primitive.h>
#include <VS/Utils/VS_SingletonManager.h>
#include <VS/Utils/VS_String.h>
#include <VS/Utils/VS_System.h>

#endif // VS_HEADERS
