/*
 *  VS_Shader.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 1/03/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Shader.h"

#include "VS_File.h"
#include "VS_Input.h"
#include "VS_Store.h"
#include "VS_Screen.h"
#include "VS_System.h"
#include "VS_TimerSystem.h"
#include "VS_Renderer_OpenGL2.h"

vsShader::vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture ):
	m_shader(-1)
{
	if ( vsRenderer_OpenGL2::Exists() )
	{
		vsFile vShader( vsString("shaders/") + vertexShader, vsFile::MODE_Read );
		vsFile fShader( vsString("shaders/") + fragmentShader, vsFile::MODE_Read );

		uint32_t vSize = vShader.GetLength();
		uint32_t fSize = fShader.GetLength();

		vsStore *vStore = new vsStore(vSize);
		vsStore *fStore = new vsStore(fSize);

		vShader.Store( vStore );
		fShader.Store( fStore );
		vsString vString( vStore->GetReadHead(), vSize );
		vsString fString( fStore->GetReadHead(), fSize );

		vsString version;

		// check whether each shader begins with a #version statement.
		// If so, let's remove and remember it, then re-insert it into
		// the final version of the shader.  (We check on both vertex
		// and fragment shaders, and insert the same value into both)
		if ( vString.find("#version") == 0 )
		{
			int64_t pos = vString.find('\n');
			if ( pos != vsString::npos )
			{
				version = std::string("#version ") + vString.substr(9, pos-9) + "\n";
				vString.erase(0,pos);
			}
		}
		if ( fString.find("#version") == 0 )
		{
			int64_t pos = fString.find('\n');
			if ( pos != vsString::npos )
			{
				std::string fVersion = std::string("#version ") + fString.substr(9, pos-9) + "\n";
				if ( version == vsEmptyString )
					version = fVersion;
				else
					vsAssert( version == fVersion, "Non-matching #version statements in vertex and fragment shaders" );

				fString.erase(0,pos);
			}
		}

		if ( lit )
		{
			vString = "#define LIT 1\n" + vString;
			fString = "#define LIT 1\n" + fString;
		}
		if ( texture )
		{
			vString = "#define TEXTURE 1\n" + vString;
			fString = "#define TEXTURE 1\n" + fString;
		}

		vString = version + vString;
		fString = version + fString;

#if !TARGET_OS_IPHONE
		m_shader = vsRenderer_OpenGL2::Compile( vString.c_str(), fString.c_str(), vString.size(), fString.size() );
		//m_shader = vsRenderSchemeShader::Instance()->Compile( vStore->GetReadHead(), fStore->GetReadHead(), vSize, fSize);
#endif // TARGET_OS_IPHONE

        m_alphaRefLoc = glGetUniformLocation(m_shader, "alphaRef");
		m_resolutionLoc = glGetUniformLocation(m_shader, "resolution");
		m_globalTimeLoc = glGetUniformLocation(m_shader, "globalTime");
		m_mouseLoc = glGetUniformLocation(m_shader, "mouse");
		m_fogLoc = glGetUniformLocation(m_shader, "fog");
		m_textureLoc = glGetUniformLocation(m_shader, "texture");
		m_localToWorldLoc = glGetUniformLocation(m_shader, "localToWorld");
		m_worldToViewLoc = glGetUniformLocation(m_shader, "worldToView");
		m_viewToProjectionLoc = glGetUniformLocation(m_shader, "viewToProjection");

		delete vStore;
		delete fStore;
	}
}

vsShader::~vsShader()
{
	vsRenderer_OpenGL2::DestroyShader(m_shader);
}

void
vsShader::SetAlphaRef( float aref )
{
	if ( m_alphaRefLoc >= 0 )
	{
		glUniform1f( m_alphaRefLoc, aref );
	}
}

void
vsShader::SetFog( bool fog )
{
	if ( m_fogLoc >= 0 )
	{
		glUniform1i( m_fogLoc, fog );
	}
}

void
vsShader::SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] )
{
	if ( m_textureLoc >= 0 )
	{
		const GLint value[MAX_TEXTURE_SLOTS] = { 0, 1, 2, 3 };
		glUniform1iv( m_textureLoc, 4, value );
	}
}

void
vsShader::SetLocalToWorld( const vsMatrix4x4& localToWorld )
{
	if ( m_localToWorldLoc >= 0 )
	{
		glUniformMatrix4fv( m_localToWorldLoc, 1, false, (GLfloat*)&localToWorld );
	}
}

void
vsShader::SetWorldToView( const vsMatrix4x4& worldToView )
{
	if ( m_worldToViewLoc >= 0 )
	{
		glUniformMatrix4fv( m_worldToViewLoc, 1, false, (GLfloat*)&worldToView );
	}
}

void
vsShader::SetViewToProjection( const vsMatrix4x4& projection )
{
	if ( m_viewToProjectionLoc >= 0 )
	{
		glUniformMatrix4fv( m_viewToProjectionLoc, 1, false, (GLfloat*)&projection );
	}
}

void
vsShader::Prepare()
{
	if ( m_resolutionLoc >= 0 )
	{
		int xRes = vsSystem::GetScreen()->GetWidth();
		int yRes = vsSystem::GetScreen()->GetHeight();
		glUniform2f( m_resolutionLoc, xRes, yRes );
	}
	if ( m_globalTimeLoc >= 0 )
	{
		int milliseconds = vsTimerSystem::Instance()->GetMicrosecondsSinceInit() / 1000;
		float seconds = milliseconds / 1000.f;
		glUniform1f( m_globalTimeLoc, seconds );
	}
	if ( m_mouseLoc >= 0 )
	{
		vsVector2D mousePos = vsInput::Instance()->GetWindowMousePosition();
		int yRes = vsSystem::GetScreen()->GetHeight();
		// the coordinate system in the GLSL shader is inverted from the
		// coordinate system we like to use.  So let's invert it!
		glUniform2f( m_mouseLoc, mousePos.x, yRes - mousePos.y );
	}
}

