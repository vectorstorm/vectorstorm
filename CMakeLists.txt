cmake_minimum_required( VERSION 2.6 )
if (DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel")
else()
	message(STATUS "No build type selected, default to Debug")
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel")
endif()
project( vectorstorm )
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
	
find_package(OpenGL)
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(PNG REQUIRED)
if(UNIX)
	find_library(GLEW_LIBRARY GLEW REQUIRED)
else()
	find_library(GLEW_LIBRARY GLEW32 REQUIRED)
endif()
find_path(GLEW_INCLUDE_DIR glew.h
	PATH /usr/include/GL /usr/local/include/GL )
find_library(BOX2D_LIBRARY Box2D REQUIRED)
find_path(BOX2D_INCLUDE_DIR Box2D/Box2D.h
	PATH /usr/local/include )

find_path(OVR_INCLUDE_DIR OVR.h REQUIRED HINTS ../LibOVR/Include/)


set(VS_LIBRARIES
	${OPENGL_LIBRARIES}
	${SDL_LIBRARY}
	${SDLIMAGE_LIBRARY}
	${SDLMIXER_LIBRARY}
	${PNG_LIBRARY}
	${GLEW_LIBRARY}
	${BOX2D_LIBRARY}
	${OVR_LIBRARY}
	)

include_directories(
	${PNG_PNG_INCLUDE_DIR} 
	${SDL_INCLUDE_DIR} 
	${SDLMIXER_INCLUDE_DIR} 
	${SDLIMAGE_INCLUDE_DIR} 
	${ZLIB_INCLUDE_DIRS} 
	${GLEW_INCLUDE_DIR} 
	${BOX2D_INCLUDE_DIR}
	${OVR_INCLUDE_DIR}
	#/usr/local/include
	)

if(APPLE)
	# Set this value (perhaps as env variable?) to do a 32-64 build
	#SET(CMAKE_OSX_ARCHITECTURES i386 x86_64 )
	set(GUI_TYPE MACOSX_BUNDLE)
	add_definitions("-D__apple_cc__")
	mark_as_advanced(BOX2D_LIBRARY SDL_LIBRARY SDL_MIXER_LIBRARY SDL_IMAGE_LIBRARY PNG_LIBRARY GLEW_LIBRARY)
	include_directories(${GLEW_INCLUDE_DIR} ${BOX2D_INCLUDE_DIR})
	#set(XCODE_ATTRIBUTE_SDKROOT macosx10.7)
	#execute_process(COMMAND xcodebuild -version -sdk "${XCODE_ATTRIBUTE_SDKROOT}" Path | head -n 1 OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
	#string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")
endif ()

# find . -type d

set(CORE_SOURCES
	VS/Core/Core.cpp
	VS/Core/CORE_Game.cpp
	VS/Core/CORE_GameMode.cpp
	VS/Core/CORE_GameRegistry.cpp
	VS/Core/CORE_GameSystem.cpp
	)
set(FILES_SOURCES
	VS/Files/VS_File.cpp
	VS/Files/VS_Record.cpp
	VS/Files/VS_Token.cpp
	)
set(GRAPHICS_SOURCES
	VS/Graphics/VS_Camera.cpp
	VS/Graphics/VS_Color.cpp
	VS/Graphics/VS_DisplayList.cpp
	VS/Graphics/VS_Entity.cpp
	VS/Graphics/VS_Fog.cpp
	VS/Graphics/VS_Font.cpp
	VS/Graphics/VS_Fragment.cpp
	VS/Graphics/VS_Light.cpp
	VS/Graphics/VS_Material.cpp
	VS/Graphics/VS_MaterialInternal.cpp
	VS/Graphics/VS_MaterialManager.cpp
	VS/Graphics/VS_Mesh.cpp
	VS/Graphics/VS_Model.cpp
	VS/Graphics/VS_Overlay.cpp
	VS/Graphics/VS_RenderBuffer.cpp
	VS/Graphics/VS_RenderQueue.cpp
	VS/Graphics/VS_RenderTarget.cpp
	VS/Graphics/VS_Renderer.cpp
	VS/Graphics/VS_RendererBloom.cpp
	VS/Graphics/VS_RendererPretty.cpp
	VS/Graphics/VS_RendererShader.cpp
	VS/Graphics/VS_RendererSimple.cpp
	VS/Graphics/VS_RendererState.cpp
	VS/Graphics/VS_Rift.cpp
	VS/Graphics/VS_Scene.cpp
	VS/Graphics/VS_Screen.cpp
	VS/Graphics/VS_Shader.cpp
	VS/Graphics/VS_ShaderSuite.cpp
	VS/Graphics/VS_Sprite.cpp
	VS/Graphics/VS_Texture.cpp
	VS/Graphics/VS_TextureInternal.cpp
	VS/Graphics/VS_TextureManager.cpp
	VS/Input/VS_Input.cpp
	VS/Math/MT/SFMT.cpp
	VS/Math/VS_Angle.cpp
	VS/Math/VS_Box.cpp
	VS/Math/VS_EulerAngles.cpp
	VS/Math/VS_Frustum.cpp
	VS/Math/VS_Math.cpp
	VS/Math/VS_Matrix.cpp
	VS/Math/VS_Perlin.cpp
	VS/Math/VS_Quaternion.cpp
	VS/Math/VS_Random.cpp
	VS/Math/VS_Spline.cpp
	VS/Math/VS_Transform.cpp
	VS/Math/VS_Vector.cpp
	VS/Memory/VS_Heap.cpp
	VS/Memory/VS_Serialiser.cpp
	VS/Memory/VS_Store.cpp
	VS/Network/VS_NetClient.cpp
	VS/Network/VS_Socket.cpp
	VS/Network/VS_SocketTCP.cpp
	VS/Physics/VS_CollisionObject.cpp
	VS/Physics/VS_CollisionSystem.cpp
	VS/Physics/VS_PhysicsEmitter.cpp
	VS/Physics/VS_PhysicsSprite.cpp
	VS/Sound/VS_Music.cpp
	VS/Sound/VS_SoundSample.cpp
	VS/Sound/VS_SoundSystem.cpp
	VS/Threads/VS_Mutex.cpp
	VS/Threads/VS_Semaphore.cpp
	VS/Threads/VS_Task.cpp
	#VS/Utils/VS_Cache.cpp
	VS/Utils/VS_Debug.cpp
	VS/Utils/VS_Factory.cpp
	#VS/Utils/VS_FontMaker.cpp
	VS/Utils/VS_HashTable.cpp
	#VS/Utils/VS_HashTableStore.cpp
	VS/Utils/VS_Image.cpp
	VS/Utils/VS_LocalisationTable.cpp
	VS/Utils/VS_Log.cpp
	VS/Utils/VS_Menu.cpp
	VS/Utils/VS_MeshMaker.cpp
	VS/Utils/VS_Octree.cpp
	#VS/Utils/VS_Pool.cpp
	VS/Utils/VS_Preferences.cpp
	VS/Utils/VS_Primitive.cpp
	VS/Utils/VS_SingletonManager.cpp
	VS/Utils/VS_Spring.cpp
	VS/Utils/VS_String.cpp
	VS/Utils/VS_System.cpp
	VS/Utils/VS_Timer.cpp
	VS/Utils/VS_TimerSystem.cpp
	VS/Utils/img_savepng.cpp
	)
set(SOURCES
	VS/VS_Headers.h
	VS/VS_OpenGL.h
	VS/VS_Prefix.h
	VS/VS_Prefix_Debug.h
	)

	if ( UNIX )
		add_definitions("-D UNIX")
	endif ( UNIX )

include_directories(
	.
	VS
	VS/Core
	VS/Files
	VS/Graphics
	VS/Input
	VS/Math
	VS/Math/MT
	VS/Memory
	VS/Network
	VS/Physics
	VS/Sound
	VS/Threads
	VS/Utils
	SYSTEM
	)

	if (UNIX)
		set(CMAKE_CXX_FLAGS_DEBUG "-include VS_Prefix_Debug.h ${CMAKE_CXX_FLAGS_DEBUG}" )
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-include VS_Prefix_Debug.h ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )
		set(CMAKE_CXX_FLAGS_RELEASE "-include VS_Prefix.h ${CMAKE_CXX_FLAGS_RELEASE}" )
	else ()
		add_definitions("/FI\"VS_Prefix.h\"")
	endif()

	add_library(
		vectorstorm
		${SOURCES}
		${CORE_SOURCES}
		${FILES_SOURCES}
		${GRAPHICS_SOURCES}
		${INPUT_SOURCES}
		${MATH_SOURCES}
		${MEMORY_SOURCES}
		${NETWORK_SOURCES}
		${PHYSICS_SOURCES}
		${SOUND_SOURCES}
		${THREADS_SOURCES}
		${UTILS_SOURCES}
		${PLATFORM_SOURCES}
		)

	set_target_properties(vectorstorm PROPERTIES XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "/Library/Frameworks/")	# Working around bug in XCode 4.3.3

	source_group("VectorStorm" FILES ${SOURCES} )
	source_group("Core" FILES ${CORE_SOURCES} )
	source_group("Files" FILES ${FILES_SOURCES} )
	source_group("Graphics" FILES ${GRAPHICS_SOURCES} )
	source_group("Input" FILES ${INPUT_SOURCES} )
	source_group("Math" FILES ${MATH_SOURCES} )
	source_group("Memory" FILES ${MEMORY_SOURCES} )
	source_group("Network" FILES ${NETWORK_SOURCES} )
	source_group("Physics" FILES ${PHYSICS_SOURCES} )
	source_group("Sound" FILES ${SOUND_SOURCES} )
	source_group("Threads" FILES ${THREADS_SOURCES} )
	source_group("Utils" FILES ${UTILS_SOURCES} )
	source_group("Platform" FILES ${PLATFORM_SOURCES} )

