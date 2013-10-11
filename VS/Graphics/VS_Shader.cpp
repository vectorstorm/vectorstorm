/*
 *  VS_Shader.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 1/03/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#include "VS_Shader.h"

#include "VS_RendererShader.h"

#include "VS_File.h"
#include "VS_Input.h"
#include "VS_Store.h"
#include "VS_Screen.h"
#include "VS_System.h"
#include "VS_TimerSystem.h"

vsShader::vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture ):
	m_shader(-1)
{
	if ( vsRendererShader::Exists() )
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


#if !TARGET_OS_IPHONE
		m_shader = vsRendererShader::Instance()->Compile( vString.c_str(), fString.c_str(), vString.size(), fString.size() );
		//m_shader = vsRendererShader::Instance()->Compile( vStore->GetReadHead(), fStore->GetReadHead(), vSize, fSize);
#endif // TARGET_OS_IPHONE

        m_alphaRefLoc = glGetUniformLocation(m_shader, "alphaRef");
		m_resolutionLoc = glGetUniformLocation(m_shader, "resolution");
		m_globalTimeLoc = glGetUniformLocation(m_shader, "globalTime");
		m_mouseLoc = glGetUniformLocation(m_shader, "mouse");

		delete vStore;
		delete fStore;
	}
}

vsShader::~vsShader()
{
	if ( vsRendererShader::Exists() )
	{
		vsRendererShader::Instance()->DestroyShader(m_shader);
	}
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

