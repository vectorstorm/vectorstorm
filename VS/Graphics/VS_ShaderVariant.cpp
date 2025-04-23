/*
 *  VS_ShaderVariant.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 23/06/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_ShaderVariant.h"
#include "VS_OpenGL.h"
#include "VS_File.h"
#include "VS_Input.h"
#include "VS_RenderTarget.h"
#include "VS_Screen.h"
#include "VS_Store.h"
#include "VS_ShaderValues.h"
#include "VS_ShaderUniformRegistry.h"
#include "VS_TimerSystem.h"
#include "VS_Renderer_OpenGL3.h"
#include "VS_RenderBuffer.h"
#include "VS_Profile.h"
#include "VS_VertexArrayObject.h"

// static bool m_localToWorldAttribIsActive = false;
// static bool m_colorAttribIsActive = false;

	extern vsArray<vsShaderVariantDefinition> g_shaderVariantDefinitions;

vsShaderVariant::vsShaderVariant( const vsString &vertexShader,
		const vsString &fragmentShader,
		bool lit,
		bool texture,
		uint32_t variantBits,
		const vsString& vFilename,
		const vsString& fFilename ):
	m_uniform(nullptr),
	m_attribute(nullptr),
	m_uniformCount(0),
	m_attributeCount(0),
	m_vertexShaderFile(vFilename),
	m_fragmentShaderFile(fFilename),
	m_system(false),
	m_shader(-1),
	m_variantBits(variantBits),
	m_litBool(lit),
	m_textureBool(texture)
{
	GL_CHECK_SCOPED("Shader");
	Compile( vertexShader, fragmentShader, lit, texture, m_variantBits );
}

void
vsShaderVariant::DoPreprocessor( vsString &s )
{
	bool done = false;
	size_t includePos;
	// size_t variantPos;
	// size_t commentPos;
	while (!done)
	{
		done = true;

		// if ( (commentPos = s.find("#<{(|")) != vsString::npos )
		// {
		// 	done = false;
		// 	// We have a C-style comment block.  Kill everything
		// 	// from here to the first comment close.
		// 	size_t commentEndPos = s.find("|)}>#");
		// 	vsAssert( commentEndPos != vsString::npos, "Unterminated comment block??" );
		// 	commentEndPos += 2;
		// 	s.erase( commentPos, commentEndPos-commentPos );
		// 	continue;
		// }
		if ( (includePos = s.find("\n#include \"")) != vsString::npos )
		{
			includePos++; // skip the newline
			done = false;
			// We have a #include directive.  Let's find the filename
			size_t cursor = includePos;
			bool inFilename = false;
			vsString filename;
			while (1)
			{
				if ( s[cursor] != '\"' )
				{
					if ( inFilename )
						filename += s[cursor];
				}
				else
				{
					if ( !inFilename )
						inFilename = true;
					else
					{
						// Okay, we have our filename and our include statement!
						cursor++;
						vsFile file( vsString("shaders/") + filename, vsFile::MODE_Read );
						uint32_t size = file.GetLength();
						vsStore *vStore = new vsStore(size);
						file.Store( vStore );
						vsString fileContents( vStore->GetReadHead(), size );
						s.replace(includePos, cursor-includePos, fileContents);
						delete vStore;
						break;
					}
				}
				cursor++;
			}
			continue;
		}
	}
}

void
vsShaderVariant::Compile( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture, uint32_t variantBits )
{
	GL_CHECK_SCOPED("Shader::Compile");
	vsShader::Uniform *oldUniform = m_uniform;
	vsShader::Attribute *oldAttribute = m_attribute;
	// int oldUniformCount = m_uniformCount;
	// int oldAttributeCount = m_attributeCount;

	vsString version;

	vsString vString = vertexShader;
	vsString fString = fragmentShader;

	vString.erase( std::remove(vString.begin(), vString.end(), '\r'), vString.end() );
	fString.erase( std::remove(fString.begin(), fString.end(), '\r'), fString.end() );

	// check whether each shader begins with a #version statement.
	// If so, let's remove and remember it, then re-insert it into
	// the final version of the shader.  (We check on both vertex
	// and fragment shaders, and insert the same value into both)
	if ( vString.find("#version") == 0 )
	{
		size_t pos = vString.find('\n');
		if ( pos != vsString::npos )
		{
			version = std::string("#version ") + vString.substr(9, pos-9) + "\n";
			vString.erase(0,pos);
		}
	}
	if ( fString.find("#version") == 0 )
	{
		size_t pos = fString.find('\n');
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

	DoPreprocessor(vString);
	DoPreprocessor(fString);

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


	vsArray<vsString> defines;
	// [TODO] Fill out defines based upon variant bits somehow.
	for ( int i = 0; i < g_shaderVariantDefinitions.ItemCount(); i++ )
	{
		if ( m_variantBits & BIT(g_shaderVariantDefinitions[i].bitId) )
			defines.AddItem( g_shaderVariantDefinitions[i].name );
	}

	for ( int i = 0; i < defines.ItemCount(); i++ )
	{
		vString = vsFormatString("#define %s 1\n%s", defines[i], vString);
		fString = vsFormatString("#define %s 1\n%s", defines[i], fString);
	}

	vsString vFilename = vsFormatString("// filename: %s\n", m_vertexShaderFile);
	vsString fFilename = vsFormatString("// filename: %s\n", m_fragmentShaderFile);

	vString = version + vFilename + vString;
	fString = version + fFilename + fString;

#if !TARGET_OS_IPHONE
	if ( m_shader == 0xffffffff )
		m_shader = vsRenderer_OpenGL3::Compile( vString, fString );
	else
		vsRenderer_OpenGL3::Compile( m_shader, vString, fString, false );
	// vsLog("Created shader %d", m_shader);
#endif // TARGET_OS_IPHONE

	m_colorLoc = glGetUniformLocation(m_shader, "universal_color");
	m_instanceColorAttributeLoc = glGetAttribLocation(m_shader, "instanceColorAttrib");
	m_hasInstanceColorsLoc = glGetUniformLocation(m_shader, "hasInstanceColors");
	m_resolutionLoc = glGetUniformLocation(m_shader, "resolution");
	m_mouseLoc = glGetUniformLocation(m_shader, "mouse");
	// m_fogLoc = glGetUniformLocation(m_shader, "fog");
	m_textureLoc = glGetUniformLocation(m_shader, "textures");
	m_shadowTextureLoc = glGetUniformLocation(m_shader, "shadowTexture");
	m_bufferTextureLoc = glGetUniformLocation(m_shader, "bufferTexture");
	m_localToWorldLoc = glGetUniformLocation(m_shader, "localToWorld");
	m_worldToViewLoc = glGetUniformLocation(m_shader, "worldToView");
	m_viewToProjectionLoc = glGetUniformLocation(m_shader, "viewToProjection");
	m_viewportLoc = glGetUniformLocation(m_shader, "viewport");
	m_cameraPositionLoc = glGetUniformLocation(m_shader, "cameraPosition");
	m_cameraDirectionLoc = glGetUniformLocation(m_shader, "cameraDirection");


	m_localToWorldAttributeLoc = glGetAttribLocation(m_shader, "localToWorldAttrib");

	// for ( int i = 0; i < 4; i++ )
	// {
	// m_lightSourceLoc[i] = glGetUniformLocation(m_shader, vsFormatString("lightSource[%d]", i).c_str());
	// m_lightSourceLoc[i] = glGetUniformLocation(m_shader, vsFormatString("lightSource[%d]", i).c_str());
	// }
	m_lightAmbientLoc = glGetUniformLocation(m_shader, "lightSource[0].ambient");
	m_lightDiffuseLoc = glGetUniformLocation(m_shader, "lightSource[0].diffuse");;
	m_lightSpecularLoc = glGetUniformLocation(m_shader, "lightSource[0].specular");;
	m_lightPositionLoc = glGetUniformLocation(m_shader, "lightSource[0].position");;
	m_lightHalfVectorLoc = glGetUniformLocation(m_shader, "lightSource[0].halfVector");;

	m_depthOnlyLoc = glGetUniformLocation(m_shader, "depthOnly");

	int activeUniformCount = 0;
	glGetProgramiv( m_shader, GL_ACTIVE_UNIFORMS, &activeUniformCount );
	glGetProgramiv( m_shader, GL_ACTIVE_ATTRIBUTES, &m_attributeCount );

	// 'activeUniformCount' only counts an array of uniforms as a single thing.
	// We're going to expand those out so that we can bind values to individual
	// array elements as if they were separate uniforms.  So the first step is,
	// we need to count how many *actual* uniforms we're going to create, so
	// we can allocate enough space.
	m_uniformCount = 0;
	const int c_maxNameLength = 256;
	char nameBuffer[c_maxNameLength];
	for ( GLint i = 0; i < activeUniformCount; i++ )
	{
		GLint arraySize = 0;
		GLenum type = 0;
		GLsizei actualLength = 0;
		glGetActiveUniform(m_shader, i, c_maxNameLength, &actualLength, &arraySize, &type, nameBuffer);
		m_uniformCount += arraySize;
	}

	m_uniform = new vsShader::Uniform[m_uniformCount];

	int nextUniform = 0;
	// int defaultSamplerBinding = 0;
	for ( GLint i = 0; i < activeUniformCount; i++ )
	{
		GL_CHECK("Shader::Uniform");
		GLint arraySize = 0;
		GLenum type = 0;
		GLsizei actualLength = 0;
		glGetActiveUniform(m_shader, i, c_maxNameLength, &actualLength, &arraySize, &type, nameBuffer);

		vsString baseName = vsString(nameBuffer);
		vsString::size_type arrayPos = baseName.find("[0]");

		for ( int arrayIndex = 0; arrayIndex < arraySize; arrayIndex++ )
		{
			int ui = nextUniform++;

			vsString name(baseName);
			if ( arraySize > 1 && arrayPos != vsString::npos)
			{
				name.replace( arrayPos, 3, vsFormatString("[%d]", arrayIndex) );
				// name += vsFormatString("[%d]", arrayIndex);
			}

			m_uniform[ui].name = name;
			m_uniform[ui].uid = vsShaderUniformRegistry::UID(name);
			m_uniform[ui].loc = glGetUniformLocation(m_shader, name.c_str());
			m_uniform[ui].type = type;
			m_uniform[ui].arraySize = arraySize;
			m_uniform[ui].i32 = 0;
			m_uniform[ui].u32 = 0;
			m_uniform[ui].f32 = 0.f;

			// initialise to random values, so we definitely set them at least once.
			switch ( m_uniform[ui].type )
			{
				case GL_BOOL:
				case GL_INT:
					glGetUniformiv( m_shader, m_uniform[ui].loc, &m_uniform[ui].i32 );
					break;
				case GL_UNSIGNED_INT:
					glGetUniformuiv( m_shader, m_uniform[ui].loc, &m_uniform[ui].u32 );
					break;
				case GL_FLOAT:
					glGetUniformfv( m_shader, m_uniform[ui].loc, &m_uniform[ui].f32 );
					break;
				case GL_FLOAT_VEC3:
					glGetUniformfv( m_shader, m_uniform[ui].loc, m_uniform[ui].vec4 );
					break;
				case GL_FLOAT_VEC4:
					glGetUniformfv( m_shader, m_uniform[ui].loc, m_uniform[ui].vec4 );
					break;
				case GL_FLOAT_MAT4:
					glGetUniformfv( m_shader, m_uniform[ui].loc, (GLfloat*)&(m_uniform[ui].mat.x.x) );
					break;
				case GL_SAMPLER_2D:
				case GL_UNSIGNED_INT_SAMPLER_2D:
				case GL_UNSIGNED_INT_SAMPLER_BUFFER:
				case GL_INT_SAMPLER_BUFFER:
				case GL_SAMPLER_BUFFER:
					glGetUniformiv( m_shader, m_uniform[ui].loc, &m_uniform[ui].i32 );
					break;
					// we're still by default binding buffer textures to slot 9.
					//
					// Really what we want is to look at the material's texture slots
					// to find the first buffer texture in there, and set that slot
					// index here.  And then if there's another sampler buffer in
					// the material's slots, and another uniform asking for one,
					// use that next.  And so on.
					//
					// Or alternately, maybe I want to ignore that whole complexity,
					// and just have samplers get numbered directly regardless of
					// type, and let the shader simply throw errors if the coder
					// hasn't lined up the shader's samplers to match the material.
					// Maybe that'd be most sensible.
					//
					// But for right now, stick with hardcoded 9, since that's still
					// hardcoded on the vsDynamicMaterial.
					//
					// m_uniform[ui].i32 = -1;
					// m_uniform[ui].i32 = defaultSamplerBinding;
					// defaultSamplerBinding += m_uniform[ui].arraySize;
					break;
				default:
					break;
			}
			GL_CHECK("Shader::Uniform");
		}
	}
	GL_CHECK("Shader::Between");
	m_attribute = new vsShader::Attribute[m_attributeCount];
	for ( GLint i = 0; i < m_attributeCount; i++ )
	{
		GL_CHECK("Shader::Attribute");
		GLint arraySize = 0;
		GLenum type = 0;
		GLsizei actualLength = 0;
		glGetActiveAttrib(m_shader, i, c_maxNameLength, &actualLength, &arraySize, &type, nameBuffer);
		m_attribute[i].name = vsString(nameBuffer);
		m_attribute[i].loc = glGetAttribLocation(m_shader, m_attribute[i].name.c_str());
		m_attribute[i].type = type;
		m_attribute[i].arraySize = arraySize;
		GL_CHECK("Shader::Attribute");
	}

	GL_CHECK("Shader::Other");
	m_globalTimeUniformId = GetUniformId("globalTime");
	m_globalSecondsUniformId = GetUniformId("globalSeconds");
	m_globalMicrosecondsUniformId = GetUniformId("globalMicroseconds");
	m_fogDensityId = GetUniformId("fogDensity");
	m_fogColorId = GetUniformId("fogColor");

	vsDeleteArray( oldUniform );
	vsDeleteArray( oldAttribute );
}

vsShaderVariant::~vsShaderVariant()
{
	// vsLog("Destroyed shader %d", m_shader);
	vsRenderer_OpenGL3::DestroyShader(m_shader);
	vsDeleteArray( m_uniform );
	vsDeleteArray( m_attribute );
}

void
vsShaderVariant::Reload( const vsString& vertexShader, const vsString &fragmentShader )
{
	PROFILE("vsShaderVariant::Reload");
	// system-owned shader;  don't reload it!
	if ( m_system )
		return;

	// Note:  If I ever really need to be able to reload system-owned shaders
	// at runtime (right now, this is only default_v/default_f), this can probably
	// be made to work by calling	`vsHeap::Push(g_globalHeap);` here at the start
	// of the function, and popping it back off at the bottom.

	if ( !m_vertexShaderFile.empty() && !m_fragmentShaderFile.empty() )
	{
		Compile( vertexShader, fragmentShader, m_litBool, m_textureBool, m_variantBits );
	}
}

void
vsShaderVariant::SetFog( bool fog, const vsColor& color, float density )
{
	if ( m_fogColorId >= 0 )
	{
		// glUniform3f( m_fogColorLoc, color.r, color.g, color.b );
		SetUniformValueVec3(m_fogColorId, color);
	}
	if ( m_fogDensityId >= 0 )
	{
		if ( density >= 1.f )
			vsLogOnce( "Setting surprisingly high fog density: %f", density );
		SetUniformValueF(m_fogDensityId, density);
	}
}

void
vsShaderVariant::SetColor( vsVertexArrayObject *vao, const vsColor& color )
{
	if ( m_colorLoc >= 0 )
	{
		SetUniformValueVec4( m_colorLoc, color );
	}
	// this is vertex color;  don't set that!
	if ( !vao->IsSet(3) )
		vao->SetStaticAttribute4F( 3, vsVector4D(1,1,1,1) );
	// glVertexAttrib4f( 3, 1.f, 1.f, 1.f, 1.f );

}

void
vsShaderVariant::SetInstanceColors( vsVertexArrayObject* vao, vsRenderBuffer *colors )
{
	PROFILE("vsShaderVariant::SetInstanceColors (buffer)");
	if ( m_hasInstanceColorsLoc >= 0 )
		SetUniformValueB( m_hasInstanceColorsLoc, true );
	if ( m_instanceColorAttributeLoc >= 0 )
	{
		vao->SetAttributeDivisor(m_instanceColorAttributeLoc,1);

		colors->BindAsAttribute( vao, m_instanceColorAttributeLoc );
	}
	// GL_CHECK("SetColors");
}

void
vsShaderVariant::SetInstanceColors( vsVertexArrayObject* vao, const vsColor* color, int matCount )
{
	if ( m_hasInstanceColorsLoc >= 0 )
		SetUniformValueB( m_hasInstanceColorsLoc, (matCount >= 2) );
	if ( matCount <= 0 )
		return;
	// GL_CHECK("SetInstanceColors");
	// if ( m_colorLoc >= 0 )
	// {
	// 	glUniform4f( m_colorLoc, color[0].r, color[0].g, color[0].b, color[0].a );
	// }
	// glVertexAttrib4f( 3, color[0].r, color[0].g, color[0].b, color[0].a );

	if ( m_instanceColorAttributeLoc >= 0 )
	{
		if ( matCount == 1 )
		{
			// if ( m_colorAttribIsActive )
			// {
			// 	// vao->UnbindAttribute(m_instanceColorAttributeLoc);
			// 	m_colorAttribIsActive = false;
			// }
			vao->SetStaticAttribute4F(m_instanceColorAttributeLoc, vsVector4D(
						color[0].r, color[0].g, color[0].b, color[0].a
						) );
			// glVertexAttrib4f(m_instanceColorAttributeLoc, color[0].r, color[0].g, color[0].b, color[0].a);
		}
		else
		{
			PROFILE("vsShaderVariant::SetInstanceColors (immediate)");
			// if ( !m_colorAttribIsActive )
			{
				vao->SetAttributeDivisor(m_instanceColorAttributeLoc, 1);
				// glEnableVertexAttribArray(m_instanceColorAttributeLoc);
				// glVertexAttribDivisor(m_instanceColorAttributeLoc, 1);
				// m_colorAttribIsActive = true;
			}

			GLuint size = sizeof(vsColor)*matCount;
			static GLuint g_vbo = 0xffffffff;
			static GLuint g_vboSize = 0;
			// this could be a lot smarter.
			if ( g_vbo == 0xffffffff )
			{
				glGenBuffers(1, &g_vbo);
			}

			glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
			if ( size > g_vboSize )
			{
				glBufferData(GL_ARRAY_BUFFER, size, color, GL_STREAM_DRAW);
				g_vboSize = size;
			}
			else
			{
				void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
				if ( ptr )
				{
					memcpy(ptr, color, size);
					glUnmapBuffer(GL_ARRAY_BUFFER);
				}
			}
			vao->BindAttribute(m_instanceColorAttributeLoc, g_vbo, 4, vsVertexArrayObject::ComponentType_Float, false, 0, 0);
			// glVertexAttribPointer(m_instanceColorAttributeLoc, 4, GL_FLOAT, 0, 0, 0);
#ifdef VS_PRISTINE_BINDINGS
			glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif // VS_PRISTINE_BINDINGS
		}
	}
	// GL_CHECK("SetColors");
}


void
vsShaderVariant::SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] )
{
	if ( m_textureLoc >= 0 )
	{
		const GLint value[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
		glUniform1iv( m_textureLoc, 8, value );
	}

	// NO NEED TO DO THIS ANY MORE:  TEXTURES ARE NOW BEING BOUND GENERICALLY.
	//
	// if ( m_textureLoc >= 0 )
	// {
	// 	const GLint value[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	// 	glUniform1iv( m_textureLoc, 8, value );
	// }
	// if ( m_shadowTextureLoc >= 0 )
	// {
	// 	glUniform1i( m_shadowTextureLoc, 8 );
	// }
	// if ( m_bufferTextureLoc >= 0 )
	// {
	// 	glUniform1i( m_bufferTextureLoc, 9 );
	// }
}

void
vsShaderVariant::SetLocalToWorld( vsVertexArrayObject *vao, vsRenderBuffer* buffer )
{
	if ( m_localToWorldAttributeLoc >= 0 )
	{
		// if ( !m_localToWorldAttribIsActive )
		// {
			vao->SetAttributeDivisor(m_localToWorldAttributeLoc, 1);
			vao->SetAttributeDivisor(m_localToWorldAttributeLoc+1, 1);
			vao->SetAttributeDivisor(m_localToWorldAttributeLoc+2, 1);
			vao->SetAttributeDivisor(m_localToWorldAttributeLoc+3, 1);
		// 	m_localToWorldAttribIsActive = true;
		// }

		buffer->BindAsAttribute( vao, m_localToWorldAttributeLoc );
	}
}

void
vsShaderVariant::SetLocalToWorld( vsVertexArrayObject *vao, const vsMatrix4x4* localToWorld, int matCount )
{
	if ( m_localToWorldLoc >= 0 )
	{
 		if ( matCount == 1 )
			SetUniformValueMat4( m_localToWorldLoc, *localToWorld );
		else
		{
			vsMatrix4x4 inv;
			inv.x.x = -2.f;
			SetUniformValueMat4( m_localToWorldLoc, inv );
		}
	}
	if ( m_localToWorldAttributeLoc >= 0 )
	{
		if ( matCount == 1 )
		{
			// if ( m_localToWorldAttribIsActive )
			// {
			// 	m_localToWorldAttribIsActive = false;
			// }
            //
			vao->SetStaticAttribute4F(m_localToWorldAttributeLoc, localToWorld->x);
			vao->SetStaticAttribute4F(m_localToWorldAttributeLoc+1, localToWorld->y);
			vao->SetStaticAttribute4F(m_localToWorldAttributeLoc+2, localToWorld->z);
			vao->SetStaticAttribute4F(m_localToWorldAttributeLoc+3, localToWorld->w);
		}
		else
		{
			// if ( !m_localToWorldAttribIsActive )
			{
				vao->SetAttributeDivisor(m_localToWorldAttributeLoc, 1);
				vao->SetAttributeDivisor(m_localToWorldAttributeLoc+1, 1);
				vao->SetAttributeDivisor(m_localToWorldAttributeLoc+2, 1);
				vao->SetAttributeDivisor(m_localToWorldAttributeLoc+3, 1);
				// m_localToWorldAttribIsActive = true;
			}

			static GLuint g_vbo = 0xffffffff;
			static GLuint g_vboSize = 0;
			GLuint size = sizeof(vsMatrix4x4) * matCount;
			// this could be a lot smarter.
			if ( g_vbo == 0xffffffff )
			{
				glGenBuffers(1, &g_vbo);
			}
			glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
			if ( size > g_vboSize )
			{
				glBufferData(GL_ARRAY_BUFFER, size, localToWorld, GL_STREAM_DRAW);
				g_vboSize = size;
			}
			else
			{
				void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
				if ( ptr )
				{
					memcpy(ptr, localToWorld, size);
					glUnmapBuffer(GL_ARRAY_BUFFER);
				}
			}
			vsAssert( sizeof(vsMatrix4x4) == 64, "Whaa?" );
			// glVertexAttribPointer(m_localToWorldAttributeLoc, 4, GL_FLOAT, 0, 64, 0);
			// glVertexAttribPointer(m_localToWorldAttributeLoc+1, 4, GL_FLOAT, 0, 64, (void*)16);
			// glVertexAttribPointer(m_localToWorldAttributeLoc+2, 4, GL_FLOAT, 0, 64, (void*)32);
			// glVertexAttribPointer(m_localToWorldAttributeLoc+3, 4, GL_FLOAT, 0, 64, (void*)48);

			vao->BindAttribute(m_localToWorldAttributeLoc, g_vbo, 4, vsVertexArrayObject::ComponentType_Float, false, 64, (void*)0);
			vao->BindAttribute(m_localToWorldAttributeLoc+1, g_vbo, 4, vsVertexArrayObject::ComponentType_Float, false, 64, (void*)16);
			vao->BindAttribute(m_localToWorldAttributeLoc+2, g_vbo, 4, vsVertexArrayObject::ComponentType_Float, false, 64, (void*)32);
			vao->BindAttribute(m_localToWorldAttributeLoc+3, g_vbo, 4, vsVertexArrayObject::ComponentType_Float, false, 64, (void*)48);
#ifdef VS_PRISTINE_BINDINGS
			glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif // VS_PRISTINE_BINDINGS
		}
	}
}

void
vsShaderVariant::SetWorldToView( const vsMatrix4x4& worldToView )
{
	if ( m_worldToViewLoc >= 0 )
	{
		SetUniformValueMat4( m_worldToViewLoc, worldToView );
	}
	// assume no scaling.
	if ( m_cameraPositionLoc >= 0 )
	{
		vsVector3D t = worldToView.Inverse().w;
		SetUniformValueVec3( m_cameraPositionLoc, t );
	}
	if ( m_cameraDirectionLoc >= 0 )
	{
		vsVector3D dir = worldToView.Inverse().z;
		SetUniformValueVec3( m_cameraDirectionLoc, dir );
	}
}

void
vsShaderVariant::SetViewToProjection( const vsMatrix4x4& projection )
{
	if ( m_viewToProjectionLoc >= 0 )
	{
		SetUniformValueMat4( m_viewToProjectionLoc, projection );
	}
}

void
vsShaderVariant::SetViewport( const vsVector2D& viewportDims )
{
	if ( m_viewportLoc >= 0 )
	{
		SetUniformValueVec2( m_viewportLoc, viewportDims );
	}
}

int32_t
vsShaderVariant::GetUniformId(const vsString& name) const
{
	for ( int i = 0; i < m_uniformCount; i++ )
	{
		if ( m_uniform[i].name == name )
			return i;
	}
	return -1;
}

void
vsShaderVariant::SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector )
{
	if ( id != 0 )
		return;
	if ( m_lightAmbientLoc >= 0 )
	{
		SetUniformValueVec4( m_lightAmbientLoc, ambient );
	}
	if ( m_lightDiffuseLoc >= 0 )
	{
		SetUniformValueVec4( m_lightDiffuseLoc, diffuse );
	}
	if ( m_lightSpecularLoc >= 0 )
	{
		SetUniformValueVec4( m_lightSpecularLoc, specular );
	}
	if ( m_lightPositionLoc >= 0 )
	{
		SetUniformValueVec3( m_lightPositionLoc, position );
	}
	if ( m_lightHalfVectorLoc >= 0 )
	{
		SetUniformValueVec3( m_lightHalfVectorLoc, halfVector );
	}
}

void
vsShaderVariant::Prepare( vsMaterial *material, vsShaderValues *values, vsRenderTarget *target )
{
	PROFILE("vsShaderVariant::Prepare");
	// GLint current;
	// glGetIntegerv(GL_CURRENT_PROGRAM, &current);
	// vsAssert( current == (GLint)m_shader, "This shader isn't currently active??" );

	vsShaderValues *matValues = material->GetShaderValues();
	{
		PROFILE("Setting shader values");

		bool bb;
		int b;
		float f;
		vsVector4D v;
		vsMatrix4x4 m;

	for ( int i = 0; i < m_uniformCount; i++ )
	{
		const vsShader::Uniform& u = m_uniform[i];

		switch( u.type )
		{
			case GL_BOOL:
				{
					bb = 0;
					if ( !values || !values->UniformB( u.uid, bb ) )
						 matValues->UniformB( u.uid, bb );
					SetUniformValueB( i, bb );
					break;
				}
			case GL_FLOAT:
				{
					f = 0.f;
					if ( !values || !values->UniformF( u.uid, f ) )
						matValues->UniformF( u.uid, f );
					SetUniformValueF( i, f );
					break;
				}
			case GL_FLOAT_VEC2:
				{
					v.Set(0,0,0,0);
					if ( !values || !values->UniformVec4( u.uid, v ) )
						matValues->UniformVec4( u.uid, v );
					SetUniformValueVec2( i, vsVector2D(v.x,v.y) );
					break;
				}
			case GL_FLOAT_VEC3:
				{
					v.Set(0,0,0,0);
					if ( !values || !values->UniformVec4( u.uid, v ) )
						matValues->UniformVec4( u.uid, v );
					SetUniformValueVec3( i, v );
					break;
				}
			case GL_FLOAT_VEC4:
				{
					v.Set(0,0,0,0);
					if ( !values || !values->UniformVec4( u.uid, v ) )
						matValues->UniformVec4( u.uid, v );
					SetUniformValueVec4( i, v );
					break;
				}
			case GL_FLOAT_MAT4:
				{
					m = vsMatrix4x4::Identity;
					if ( !values || !values->UniformMat4( u.uid, m ) )
						matValues->UniformMat4( u.uid, m );
					SetUniformValueMat4( i, m );
					break;
				}
			case GL_INT:
			case GL_SAMPLER_2D:
			case GL_UNSIGNED_INT_SAMPLER_2D:
			case GL_SAMPLER_2D_SHADOW:
			case GL_UNSIGNED_INT_SAMPLER_BUFFER:
			case GL_INT_SAMPLER_BUFFER:
			case GL_SAMPLER_BUFFER:
				{
					b = 0;
					if ( !values || !values->UniformI( u.uid, b ) )
						matValues->UniformI( u.uid, b);
					SetUniformValueI( i, b );
					break;
				}
			case GL_UNSIGNED_INT:
				{
					b = 0;
					if ( !values || !values->UniformI( u.uid, b ) )
						matValues->UniformI( u.uid, b);
					uint32_t ui = (uint32_t)b; // [TODO] make less horrible
					SetUniformValueUI( i, ui );
					break;
				}

			default:
				// [TODO]  Handle more uniform types
				break;
		}
	}

	}
	{
		PROFILE("Setting explicit variables");

	if ( m_resolutionLoc >= 0 )
	{
		int xRes = vsScreen::Instance()->GetWidth();
		int yRes = vsScreen::Instance()->GetHeight();
		SetUniformValueVec2( m_resolutionLoc, vsVector2D(xRes, yRes) );
	}
	if ( m_globalTimeUniformId >= 0 ||
			m_globalSecondsUniformId >= 0 ||
			m_globalMicrosecondsUniformId >= 0 )
	{
		uint64_t microseconds = vsTimerSystem::Instance()->GetMicrosecondsSinceInit();

		if ( m_globalTimeUniformId >= 0 )
		{
			uint64_t milliseconds = microseconds / 1000;
			float seconds = milliseconds / 1000.f;
			SetUniformValueF( m_globalTimeUniformId, seconds );
		}
		if ( m_globalSecondsUniformId >= 0 )
		{
			uint64_t seconds = microseconds / 1000000;
			SetUniformValueUI( m_globalSecondsUniformId, (GLuint)seconds );
		}
		if ( m_globalMicrosecondsUniformId >= 0 )
		{
			uint64_t seconds = microseconds / 1000000;
			uint64_t fractional = microseconds - (seconds * 1000000);
			SetUniformValueUI( m_globalMicrosecondsUniformId, (GLuint)fractional );
		}
	}
	if ( m_mouseLoc >= 0 )
	{
		vsVector2D mousePos = vsInput::Instance()->GetWindowMousePosition();
		SetUniformValueVec2( m_mouseLoc, vsVector2D(mousePos.x, mousePos.y) );
	}
	if ( m_depthOnlyLoc >= 0 )
	{
		SetUniformValueB( m_depthOnlyLoc, target->IsDepthOnly() );
	}
	}

}

void
vsShaderVariant::SetUniformValueF( int i, float value )
{
	if ( value != m_uniform[i].f32 )
	{
		glUniform1f( m_uniform[i].loc, value );
		m_uniform[i].f32 = value;
	}
}

void
vsShaderVariant::SetUniformValueB( int i, bool value )
{
	if ( value != m_uniform[i].i32 )
	{
		glUniform1i( m_uniform[i].loc, value );
		m_uniform[i].i32 = value;
	}
}

void
vsShaderVariant::SetUniformValueUI( int i, unsigned int value )
{
	if ( value != m_uniform[i].u32 )
	{
		glUniform1ui( m_uniform[i].loc, value );
		m_uniform[i].u32 = value;
	}
}

void
vsShaderVariant::SetUniformValueI( int i, int value )
{
	if ( value != m_uniform[i].i32 )
	{
		glUniform1i( m_uniform[i].loc, value );
		m_uniform[i].i32 = value;
	}
}

void
vsShaderVariant::SetUniformValueVec2( int i, const vsVector2D& value )
{
	if ( value.x != m_uniform[i].vec4[0] ||
			value.y != m_uniform[i].vec4[1] )
	{
		glUniform2f( m_uniform[i].loc, value.x, value.y );
		m_uniform[i].vec4[0] = value.x;
		m_uniform[i].vec4[1] = value.y;
	}
}

void
vsShaderVariant::SetUniformValueVec3( int i, const vsVector3D& value )
{
	if ( value.x != m_uniform[i].vec4[0] ||
			value.y != m_uniform[i].vec4[1] ||
			value.z != m_uniform[i].vec4[2] )
	{
		glUniform3f( m_uniform[i].loc, value.x, value.y, value.z );
		m_uniform[i].vec4[0] = value.x;
		m_uniform[i].vec4[1] = value.y;
		m_uniform[i].vec4[2] = value.z;
	}
}


void
vsShaderVariant::SetUniformValueVec3( int i, const vsColor& value )
{
	if ( value.r != m_uniform[i].vec4[0] ||
			value.b != m_uniform[i].vec4[1] ||
			value.g != m_uniform[i].vec4[2] )
	{
		glUniform3f( m_uniform[i].loc, value.r, value.g, value.b );
		m_uniform[i].vec4[0] = value.r;
		m_uniform[i].vec4[1] = value.g;
		m_uniform[i].vec4[2] = value.b;
	}
}

void
vsShaderVariant::SetUniformValueVec4( int i, const vsVector4D& value )
{
	if ( value.x != m_uniform[i].vec4[0] ||
			value.y != m_uniform[i].vec4[1] ||
			value.z != m_uniform[i].vec4[2] ||
			value.w != m_uniform[i].vec4[3] )
	{
		glUniform4f( m_uniform[i].loc, value.x, value.y, value.z, value.w );
		m_uniform[i].vec4[0] = value.x;
		m_uniform[i].vec4[1] = value.y;
		m_uniform[i].vec4[2] = value.z;
		m_uniform[i].vec4[3] = value.w;
	}
}

void
vsShaderVariant::SetUniformValueVec4( int i, const vsColor& value )
{
	if ( value.r != m_uniform[i].vec4[0] ||
			value.g != m_uniform[i].vec4[1] ||
			value.b != m_uniform[i].vec4[2] ||
			value.a != m_uniform[i].vec4[3] )
	{
		glUniform4f( m_uniform[i].loc, value.r, value.g, value.b, value.a );
		m_uniform[i].vec4[0] = value.r;
		m_uniform[i].vec4[1] = value.g;
		m_uniform[i].vec4[2] = value.b;
		m_uniform[i].vec4[3] = value.a;
	}
}

void
vsShaderVariant::SetUniformValueMat4( int i, const vsMatrix4x4& value )
{
	if ( value != m_uniform[i].mat )
	{
		glUniformMatrix4fv( m_uniform[i].loc, 1, GL_FALSE, (const GLfloat*)&value );
		m_uniform[i].mat = value;
	}
	// else
	// {
	// 	vsLog("Woo, avoided re-setting a mat4 uniform value!");
	// }
}

