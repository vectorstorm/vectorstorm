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
#include "VS_Store.h"

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

        m_alphaRef = glGetUniformLocation(m_shader, "alphaRef");

		delete vStore;
		delete fStore;
	}
}

vsShader::~vsShader()
{
	vsRendererShader::Instance()->DestroyShader(m_shader);
}

void
vsShader::SetAlphaRef( float aref )
{
    if ( m_alphaRef >= 0 )
    {
        glUniform1f( m_alphaRef, aref );
    }
}

