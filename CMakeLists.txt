cmake_minimum_required( VERSION 3.1 FATAL_ERROR )
if (DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel")
else()
	message(STATUS "No build type selected, default to Debug")
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel")
endif()
project( vectorstorm )
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

option( USE_SDL_SOUND "If disabled, don't use the SDL-based sound system." YES )
option( USE_BOX2D_PHYSICS "If disabled, don't use the Box2D collision system." YES )
option( VS_OVERLOAD_ALLOCATORS "If enabled, use custom internal allocators which can track memory overruns and leaks." YES )
option( BACKTRACE_SUPPORTED "If enabled, generate backtraces in the event of a crash. (currently only supported for Linux/OSX/MinGW)" YES )
option( VS_GL_DEBUG "If enabled, try to create an OpenGL Debug context and spit out any errors to the log.  Additionally, perform extra OpenGL error testing." NO )
option( VS_TIMING_BARS "If enabled, we'll draw timing bars at the bottom left of the screen" YES )
option( VS_DEFAULT_VIRTUAL_CONTROLLER "If enabled, we use the default controller settings.  If disabled, games must provide their own." YES )
option( VS_GAMEPADS "If enabled, vsInput will include gamepad support.  Disable if you want to use SteamInput or another input manager to handle gamepads" YES )
option( VS_TOOL "Various adjustments for tool (non-game) support" NO )
option( VS_TOOL "Various adjustments for tool (non-game) support" NO )
option( VS_PRISTINE_BINDINGS "If enabled, we clear bindings after using them" NO )
option( VS_TRACY "If enabled, support remote profiling via tracy" NO )
if ( APPLE )
	option(VS_APPBUNDLE "Build in app bundle" YES)
endif()

# If we have a choice between legacy libgl.so and more modern
# libOpenGL.so (the "GL Vendor-Neutral Dispatch" library), let's
# use the modern vender-neutral library.
#
set( OpenGL_GL_PREFERENCE "GLVND" )
find_package(OpenGL REQUIRED)
find_library(SDL_LIBRARY SDL2 REQUIRED)
find_package(ZLIB)
# find_library(ZLIB_LIBRARY zlib REQUIRED)

if ( USE_SDL_SOUND )
	find_library(SDL_MIXER_LIBRARY SDL2_mixer REQUIRED )
endif()
find_path(SDL_INCLUDE_DIR SDL2/SDL.h REQUIRED HINTS ${SDL_LIBRARY}/Headers)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	add_definitions("-D IS_64BIT")
else()
	add_definitions("-D IS_32BIT")
endif()

if(UNIX)
	find_package(PNG REQUIRED)
	find_package(PhysFS REQUIRED)
	# find_library(GLEW_LIBRARY GLEW REQUIRED)
	if(APPLE)
		# find_library(GLEW_LIBRARIES glew REQUIRED)
	else(APPLE)
		find_package(GLEW REQUIRED)
	endif(APPLE)

	find_library(PTHREAD_LIBRARY pthread)

else()
	find_library(PHYSFS_LIBRARY physfs REQUIRED)
	find_path(PHYSFS_INCLUDE_DIR physfs.h REQUIRED)
	if ( MSVC )
		add_definitions("-D _CRT_SECURE_NO_WARNINGS")
		add_definitions("-D MSVC")
		# MSVC links glew32 in.  MinGW compiles it in.
		find_library(GLEW_LIBRARIES glew32 REQUIRED
			PATH ${ANNEX}/lib )
	endif()
endif()
find_path(GLEW_INCLUDE_DIR GL/glew.h)

if (USE_BOX2D_PHYSICS)
	find_library(BOX2D_LIBRARY_RELEASE Box2D REQUIRED)
	find_library(BOX2D_LIBRARY_DEBUG Box2D REQUIRED)
	find_path(BOX2D_INCLUDE_DIR Box2D/Box2D.h)
endif()

find_path(ZLIB_INCLUDE_DIRS zlib.h REQUIRED)
if(WIN32)
	find_file(ZLIB_DLL zlib1.dll REQUIRED)
	find_file(PNG_DLL libpng16-16.dll REQUIRED)
	find_file(JPEG_DLL libjpeg-9.dll REQUIRED)
	find_file(SDL2_DLL SDL2.dll REQUIRED)
	if ( USE_SDL_SOUND )
		find_file(SDL2_MIXER_DLL SDL2_mixer.dll REQUIRED)
	endif()

	if ( MINGW )
		if ( BACKTRACE_SUPPORTED )
			find_library(DBGHELP_LIBRARY dbghelp REQUIRED )
			find_file(DBGHELP_DLL dbghelp.dll REQUIRED )
			find_file(EXCHNDL_DLL exchndl.dll REQUIRED )
			find_file(MGWHELP_DLL mgwhelp.dll REQUIRED )
			find_file(SYMSRV_DLL symsrv.dll REQUIRED )
			find_file(SYMSRV_YES symsrv.yes REQUIRED )
			find_library(EXCHNDL_LIBRARY exchndl REQUIRED )

			# For unknown reasons, order of library link is important
			# for making MinGW's "static linkage" functionality work
			# correctly.  dbghelp was pulling in libstdc++ as a dependency,
			# even thuogh we'd said to link that library statically.  By
			# explicitly adding stdc++ as a link dependency FIRST, we
			# ensure that it gets pulled in statically.  (Whereas if
			# we tried to link dbghelp first, it would be pulled in
			# dynamically, and then we we later tried to link it statically, it
			# would just leave it dynamic
			SET( PRE_LIBRARIES "stdc++" )
		endif()

		find_file(PHYSFS_DLL libphysfs.dll REQUIRED )
		if ( USE_SDL_SOUND )
			find_file(VORBIS_DLL libvorbis-0.dll REQUIRED )
			find_file(VORBISFILE_DLL libvorbisfile-3.dll REQUIRED )
			find_file(OGG_DLL libogg-0.dll REQUIRED )
		endif()
	else()
		find_file(GLEW_DLL glew32.dll REQUIRED
			PATH c:/src/buildzone/bin/Release/Win32 )
	endif()

	set(VS_PLATFORM_DLLS ${PNG_DLL} ${JPEG_DLL} ${GLEW_DLL}
		${ZLIB_DLL} ${SDL2_DLL}
		${SDL2_MIXER_DLL} ${VORBIS_DLL} ${VORBISFILE_DLL}
		${PHYSFS_DLL} ${OGG_DLL}
		${DBGHELP_DLL}
		${EXCHNDL_DLL}
		${MGWHELP_DLL}
		${SYMSRV_DLL}
		${SYMSRV_YES}
		${STDCPP_DLL}
		PARENT_SCOPE)
endif()

# Check our build environment and set any variables
# which need setting
set(CMAKE_REQUIRED_LIBRARIES ${SDL_LIBRARY})
include(CheckFunctionExists)
check_function_exists( SDL_GL_GetDrawableSize HIGHDPI_SUPPORTED )
if(WIN32)
	set(HIGHDPI_SUPPORTED OFF)
else(WIN32)
endif(WIN32)

include_directories(vectorstorm PUBLIC "${PROJECT_BINARY_DIR}/config")
set( VECTORSTORM_INCLUDE_PATHS "${PROJECT_BINARY_DIR}/config" "${CURRENT_BINARY_DIR}/config" PARENT_SCOPE)
configure_file( "${PROJECT_SOURCE_DIR}/VS_Config.h.in"
	"${PROJECT_BINARY_DIR}/config/VS_Config.h" )

include_directories(
	${SDL_INCLUDE_DIR}
	${SDLMIXER_INCLUDE_DIR}
	${SDLIMAGE_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
	${GLEW_INCLUDE_DIR}
	${BOX2D_INCLUDE_DIR}
	${PHYSFS_INCLUDE_DIR}
	#/usr/local/include
	)

if(APPLE)
	# Set this value (perhaps as env variable?) to do a 32-64 build
	#SET(CMAKE_OSX_ARCHITECTURES i386 x86_64 )
	set(GUI_TYPE MACOSX_BUNDLE)
	add_definitions("-D__apple_cc__")
	mark_as_advanced(BOX2D_RELEASE_LIBRARY SDL_LIBRARY SDL_MIXER_LIBRARY PNG_LIBRARY GLEW_LIBRARIES)
	include_directories(${GLEW_INCLUDE_DIR} ${BOX2D_INCLUDE_DIR})
	#set(XCODE_ATTRIBUTE_SDKROOT macosx10.7)
	#execute_process(COMMAND xcodebuild -version -sdk "${XCODE_ATTRIBUTE_SDKROOT}" Path | head -n 1 OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
	#string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
	# set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")
endif ()

if(MSVC)
	add_definitions("-DMSVC")
endif()


# find . -type d

set(CORE_SOURCES
	VS/Core/Core.cpp
	VS/Core/Core.h
	VS/Core/CORE_Game.cpp
	VS/Core/CORE_Game.h
	VS/Core/CORE_GameMode.cpp
	VS/Core/CORE_GameMode.h
	VS/Core/CORE_GameRegistry.cpp
	VS/Core/CORE_GameRegistry.h
	VS/Core/CORE_GameSystem.cpp
	VS/Core/CORE_GameSystem.h
	)
set(FILES_SOURCES
	VS/Files/VS_File.cpp
	VS/Files/VS_File.h
	VS/Files/VS_FileCache.cpp
	VS/Files/VS_FileCache.h
	VS/Files/VS_Record.cpp
	VS/Files/VS_Record.h
	VS/Files/VS_RecordStream.cpp
	VS/Files/VS_RecordStream.h
	VS/Files/VS_Token.cpp
	VS/Files/VS_Token.h
	VS/Files/VS_PhysFS.cpp
	VS/Files/VS_PhysFS.h
	)
set(GRAPHICS_SOURCES
	VS/Graphics/VS_BuiltInFont.cpp
	VS/Graphics/VS_BuiltInFont.h
	VS/Graphics/VS_Camera.cpp
	VS/Graphics/VS_Camera.h
	VS/Graphics/VS_Color.cpp
	VS/Graphics/VS_Color.h
	VS/Graphics/VS_DisplayList.cpp
	VS/Graphics/VS_DisplayList.h
	VS/Graphics/VS_DynamicBatch.cpp
	VS/Graphics/VS_DynamicBatch.h
	VS/Graphics/VS_DynamicBatchManager.cpp
	VS/Graphics/VS_DynamicBatchManager.h
	VS/Graphics/VS_DynamicMaterial.cpp
	VS/Graphics/VS_DynamicMaterial.h
	VS/Graphics/VS_Entity.cpp
	VS/Graphics/VS_Entity.h
	VS/Graphics/VS_Fog.cpp
	VS/Graphics/VS_Fog.h
	VS/Graphics/VS_Font.cpp
	VS/Graphics/VS_Font.h
	VS/Graphics/VS_FontRenderer.cpp
	VS/Graphics/VS_FontRenderer.h
	VS/Graphics/VS_Fragment.cpp
	VS/Graphics/VS_Fragment.h
	VS/Graphics/VS_Light.cpp
	VS/Graphics/VS_Light.h
	VS/Graphics/VS_Lines.cpp
	VS/Graphics/VS_Lines.h
	VS/Graphics/VS_Material.cpp
	VS/Graphics/VS_Material.h
	VS/Graphics/VS_MaterialInternal.cpp
	VS/Graphics/VS_MaterialInternal.h
	VS/Graphics/VS_MaterialManager.cpp
	VS/Graphics/VS_MaterialManager.h
	VS/Graphics/VS_Mesh.cpp
	VS/Graphics/VS_Mesh.h
	VS/Graphics/VS_Model.cpp
	VS/Graphics/VS_Model.h
	VS/Graphics/VS_ModelInstance.cpp
	VS/Graphics/VS_ModelInstance.h
	VS/Graphics/VS_ModelInstanceGroup.cpp
	VS/Graphics/VS_ModelInstanceGroup.h
	VS/Graphics/VS_RenderBuffer.cpp
	VS/Graphics/VS_RenderBuffer.h
	VS/Graphics/VS_RenderQueue.cpp
	VS/Graphics/VS_RenderQueue.h
	VS/Graphics/VS_RenderTarget.cpp
	VS/Graphics/VS_RenderTarget.h
	VS/Graphics/VS_Renderer.cpp
	VS/Graphics/VS_Renderer.h
	VS/Graphics/VS_Renderer_OpenGL3.cpp
	VS/Graphics/VS_Renderer_OpenGL3.h
	VS/Graphics/VS_RenderPipeline.cpp
	VS/Graphics/VS_RenderPipeline.h
	VS/Graphics/VS_RenderPipelineStage.cpp
	VS/Graphics/VS_RenderPipelineStage.h
	VS/Graphics/VS_RenderPipelineStageBlit.cpp
	VS/Graphics/VS_RenderPipelineStageBlit.h
	VS/Graphics/VS_RenderPipelineStageBloom.cpp
	VS/Graphics/VS_RenderPipelineStageBloom.h
	VS/Graphics/VS_RenderPipelineStageScenes.cpp
	VS/Graphics/VS_RenderPipelineStageScenes.h
	# VS/Graphics/VS_RenderScheme.cpp
	# VS/Graphics/VS_RenderSchemeFixedFunction.cpp
	# VS/Graphics/VS_RenderSchemeBloom.cpp
	# VS/Graphics/VS_RenderSchemeShader.cpp
	VS/Graphics/VS_RendererState.cpp
	VS/Graphics/VS_RendererState.h
	VS/Graphics/VS_Scene.cpp
	VS/Graphics/VS_Scene.h
	VS/Graphics/VS_Screen.cpp
	VS/Graphics/VS_Screen.h
	VS/Graphics/VS_Shader.cpp
	VS/Graphics/VS_Shader.h
	VS/Graphics/VS_ShaderCache.cpp
	VS/Graphics/VS_ShaderCache.h
	VS/Graphics/VS_ShaderOptions.cpp
	VS/Graphics/VS_ShaderOptions.h
	VS/Graphics/VS_ShaderRef.cpp
	VS/Graphics/VS_ShaderRef.h
	VS/Graphics/VS_ShaderSuite.cpp
	VS/Graphics/VS_ShaderSuite.h
	VS/Graphics/VS_ShaderUniformRegistry.cpp
	VS/Graphics/VS_ShaderUniformRegistry.h
	VS/Graphics/VS_ShaderValues.cpp
	VS/Graphics/VS_ShaderValues.h
	VS/Graphics/VS_ShaderVariant.cpp
	VS/Graphics/VS_ShaderVariant.h
	VS/Graphics/VS_Sprite.cpp
	VS/Graphics/VS_Sprite.h
	VS/Graphics/VS_Texture.cpp
	VS/Graphics/VS_Texture.h
	VS/Graphics/VS_TextureInternal.cpp
	VS/Graphics/VS_TextureInternal.h
	VS/Graphics/VS_TextureInternalIPhone.h
	VS/Graphics/VS_TextureManager.cpp
	VS/Graphics/VS_TextureManager.h
	)
set(INPUT_SOURCES
	VS/Input/VS_Input.cpp
	VS/Input/VS_Input.h
	)
set(MATH_SOURCES
	VS/Math/VS_Angle.cpp
	VS/Math/VS_Angle.h
	VS/Math/VS_Box.cpp
	VS/Math/VS_Box.h
	VS/Math/VS_EulerAngles.cpp
	VS/Math/VS_EulerAngles.h
	VS/Math/VS_Frustum.cpp
	VS/Math/VS_Frustum.h
	VS/Math/VS_Math.cpp
	VS/Math/VS_Math.h
	VS/Math/VS_Matrix.cpp
	VS/Math/VS_Matrix.h
	VS/Math/VS_Perlin.cpp
	VS/Math/VS_Perlin.h
	VS/Math/VS_Quaternion.cpp
	VS/Math/VS_Quaternion.h
	VS/Math/VS_Random.cpp
	VS/Math/VS_Random.h
	VS/Math/VS_Span.cpp
	VS/Math/VS_Span.h
	VS/Math/VS_Spline.cpp
	VS/Math/VS_Spline.h
	VS/Math/VS_Transform.cpp
	VS/Math/VS_Transform.h
	VS/Math/VS_Vector.cpp
	VS/Math/VS_Vector.h
	)
set(MEMORY_SOURCES
	VS/Memory/VS_Heap.cpp
	VS/Memory/VS_Heap.h
	VS/Memory/VS_Serialiser.cpp
	VS/Memory/VS_Serialiser.h
	VS/Memory/VS_Store.cpp
	VS/Memory/VS_Store.h
	)
set(NETWORK_SOURCES
	VS/Network/VS_NetClient.cpp
	VS/Network/VS_NetClient.h
	VS/Network/VS_Socket.cpp
	VS/Network/VS_Socket.h
	VS/Network/VS_SocketTCP.cpp
	VS/Network/VS_SocketTCP.h
	)
if ( USE_BOX2D_PHYSICS )
	set(PHYSICS_SOURCES
		VS/Physics/VS_CollisionObject.cpp
		VS/Physics/VS_CollisionObject.h
		VS/Physics/VS_CollisionSystem.cpp
		VS/Physics/VS_CollisionSystem.h
		VS/Physics/VS_PhysicsEmitter.cpp
		VS/Physics/VS_PhysicsEmitter.h
		VS/Physics/VS_PhysicsSprite.cpp
		VS/Physics/VS_PhysicsSprite.h
		)
endif()
if ( USE_SDL_SOUND )
	set(SOUND_SOURCES
		VS/Sound/VS_Music.cpp
		VS/Sound/VS_Music.h
		VS/Sound/VS_SoundSample.cpp
		VS/Sound/VS_SoundSample.h
		VS/Sound/VS_SoundSystem.cpp
		VS/Sound/VS_SoundSystem.h
		)
endif()
set(THREADS_SOURCES
	VS/Threads/VS_Mutex.cpp
	VS/Threads/VS_Mutex.h
	VS/Threads/VS_Semaphore.cpp
	VS/Threads/VS_Semaphore.h
	VS/Threads/VS_Spinlock.cpp
	VS/Threads/VS_Spinlock.h
	VS/Threads/VS_Task.cpp
	VS/Threads/VS_Task.h
	)
set(UTILS_SOURCES
	VS/Utils/VS_Array.h
	VS/Utils/VS_ArrayStore.h
	VS/Utils/VS_AutomaticInstanceList.h
	VS/Utils/VS_Backtrace.cpp
	VS/Utils/VS_Backtrace.h
	#VS/Utils/VS_Cache.cpp
	VS/Utils/VS_Cache.h
	VS/Utils/VS_Debug.cpp
	VS/Utils/VS_Debug.h
	VS/Utils/VS_Demangle.cpp
	VS/Utils/VS_Demangle.h
	VS/Utils/VS_Factory.cpp
	VS/Utils/VS_Factory.h
	#VS/Utils/VS_FontMaker.cpp
	VS/Utils/VS_FontMaker.h
	VS/Utils/VS_HashTable.cpp
	VS/Utils/VS_HashTable.h
	#VS/Utils/VS_HashTableStore.cpp
	VS/Utils/VS_HashTableStore.h
	VS/Utils/VS_IntHashTable.cpp
	VS/Utils/VS_IntHashTable.h
	VS/Utils/VS_FloatImage.cpp
	VS/Utils/VS_FloatImage.h
	VS/Utils/VS_HalfFloatImage.cpp
	VS/Utils/VS_HalfFloatImage.h
	VS/Utils/VS_HalfIntImage.cpp
	VS/Utils/VS_HalfIntImage.h
	VS/Utils/VS_Image.cpp
	VS/Utils/VS_Image.h
	VS/Utils/VS_LinkedList.h
	VS/Utils/VS_LinkedListStore.h
	VS/Utils/VS_LocalisationTable.cpp
	VS/Utils/VS_LocalisationTable.h
	VS/Utils/VS_LocString.cpp
	VS/Utils/VS_LocString.h
	VS/Utils/VS_Log.cpp
	VS/Utils/VS_Log.h
	#VS/Utils/VS_Menu.cpp
	VS/Utils/VS_Menu.h
	VS/Utils/VS_MeshMaker.cpp
	VS/Utils/VS_MeshMaker.h
	VS/Utils/VS_Octree.cpp
	VS/Utils/VS_Octree.h
	VS/Utils/VS_PointOctree.h
	#VS/Utils/VS_Pool.cpp
	VS/Utils/VS_Pool.h
	VS/Utils/VS_Preferences.cpp
	VS/Utils/VS_Preferences.h
	VS/Utils/VS_Profile.cpp
	VS/Utils/VS_Profile.h
	VS/Utils/VS_Primitive.cpp
	VS/Utils/VS_Primitive.h
	VS/Utils/VS_Singleton.h
	VS/Utils/VS_SingletonManager.cpp
	VS/Utils/VS_SingletonManager.h
	VS/Utils/VS_SingleFloatImage.cpp
	VS/Utils/VS_SingleFloatImage.h
	VS/Utils/VS_Sleep.cpp
	VS/Utils/VS_Sleep.h
	VS/Utils/VS_Spring.cpp
	VS/Utils/VS_Spring.h
	VS/Utils/VS_String.cpp
	VS/Utils/VS_String.h
	VS/Utils/VS_StringTable.cpp
	VS/Utils/VS_StringTable.h
	VS/Utils/VS_StrongPointer.h
	VS/Utils/VS_StrongPointerTarget.h
	VS/Utils/VS_System.cpp
	VS/Utils/VS_System.h
	VS/Utils/VS_Timer.cpp
	VS/Utils/VS_Timer.h
	VS/Utils/VS_TimerSystem.cpp
	VS/Utils/VS_TimerSystem.h
	VS/Utils/VS_Tween.h
	VS/Utils/VS_VolatileArray.h
	VS/Utils/VS_VolatileArrayStore.h
	VS/Utils/VS_WeakPointer.h
	VS/Utils/VS_WeakPointerTarget.h
	VS/Utils/fmt/format.cc
	VS/Utils/fmt/format.h
	VS/Utils/stb/stb.cpp

	VS/Utils/uni-algo/src/cpp_uni_data.cpp
	)
set(SOURCES
	VS/VS_Headers.h
	VS/VS_OpenGL.h
	VS/VS_OpenGL.cpp
	VS/VS_Prefix.h
	VS/VS_Prefix_Debug.h
	)
if ( MINGW )
	add_definitions("-D MINGW")
	set( SOURCES
		glew.c
		${SOURCES}
		)
endif( MINGW )
if ( APPLE )
	set( SOURCES
		glew.c
		${SOURCES}
		)
endif (APPLE )

if ( VS_TRACY )
	message("TRACY ENABLED")
	add_definitions("-DTRACY_ENABLE")
	include_directories( tracy )
	set( SOURCES
		tracy/TracyClient.cpp
		${SOURCES}
		)
endif ( VS_TRACY )


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
	VS/Utils/stb
	SYSTEM
	)

if ((CMAKE_COMPILER_IS_GNUCXX) OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
	set(CMAKE_CXX_FLAGS_DEBUG "-include VS_Prefix_Debug.h -Wall -Werror ${CMAKE_CXX_FLAGS_DEBUG}" )
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-include VS_Prefix_Debug.h -Wall -Werror ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )
	set(CMAKE_CXX_FLAGS_RELEASE "-include VS_Prefix.h -Wall -Werror ${CMAKE_CXX_FLAGS_RELEASE}" )

	if ( MINGW AND BACKTRACE_SUPPORTED )
		set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS_DEBUG}" )
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" )
	endif()
else ()
	set(CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}")
	add_definitions("/FI\"VS_Prefix.h\"")
endif()

	add_definitions("-DVECTORSTORM_INTERNAL")
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

	set( LIBRARIES
		${PRE_LIBRARIES}
		${DBGHELP_LIBRARY}
		${OPENGL_LIBRARIES}
		# ${PNG_LIBRARY}
		${SDL_LIBRARY}
		${GLEW_LIBRARIES}
		${ZLIB_LIBRARY}
		${PTHREAD_LIBRARY}
		${PHYSFS_LIBRARY}
		${EXCHNDL_LIBRARY}
		# ${PLATFORM_LIBRARIES}
		)
	if ( USE_SDL_SOUND )
		set( LIBRARIES ${LIBRARIES}
			${SDL_MIXER_LIBRARY})
	endif()
	if ( USE_BOX2D_PHYSICS )
		set( LIBRARIES ${LIBRARIES}
			debug ${BOX2D_LIBRARY_DEBUG}
			optimized ${BOX2D_LIBRARY_RELEASE})
	endif()



	target_link_libraries( vectorstorm
		${LIBRARIES}
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

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND UNIX)
	# Optimise matrix maths, even if we're in a debug build.  I don't actually
	# need to debug that, and fast matrix math is always nice!
	set_source_files_properties(VS/Math/VS_Matrix.cpp PROPERTIES COMPILE_FLAGS -O3)
	set_source_files_properties(VS/Math/VS_Quaternion.cpp PROPERTIES COMPILE_FLAGS -O3)
endif ()
