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

vsShader::vsShader( const vsString &vertexShader, const vsString &fragmentShader ):
	m_shader(-1)
{
	if ( vsRendererShader::Exists() )
	{
		vsFile vShader( vsString("shaders/") + vertexShader, vsFile::MODE_Read );
		vsFile fShader( vsString("shaders/") + fragmentShader, vsFile::MODE_Read );

		uint32 vSize = vShader.GetLength();
		uint32 fSize = fShader.GetLength();

		vsStore *vStore = new vsStore(vSize);
		vsStore *fStore = new vsStore(fSize);

		vShader.Store( vStore );
		fShader.Store( fStore );

#if !TARGET_OS_IPHONE
		m_shader = vsRendererShader::Instance()->Compile( vStore->GetReadHead(), fStore->GetReadHead(), vSize, fSize );
#endif // TARGET_OS_IPHONE

		delete vStore;
		delete fStore;
	}
}

vsShader::~vsShader()
{
}

