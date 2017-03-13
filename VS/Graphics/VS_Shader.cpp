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
#include "VS_RenderBuffer.h"
#include "VS_Renderer_OpenGL3.h"
#include "VS_Screen.h"
#include "VS_ShaderValues.h"
#include "VS_Store.h"
#include "VS_System.h"
#include "VS_TimerSystem.h"
#include "VS_DisableDebugNew.h"
#include <algorithm>
#include "VS_EnableDebugNew.h"

static bool m_localToWorldAttribIsActive = false;
static bool m_colorAttribIsActive = false;


vsShader::vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture ):
	m_uniform(NULL),
	m_attribute(NULL),
	m_uniformCount(0),
	m_attributeCount(0),
	m_shader(-1)
{
	GL_CHECK_SCOPED("Shader");
	Compile( vertexShader, fragmentShader, lit, texture );
}

void DoPreprocessor( vsString &s )
{
	bool done = false;
	size_t includePos;
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
vsShader::Compile( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture )
{
	GL_CHECK_SCOPED("Shader::Compile");
	Uniform *oldUniform = m_uniform;
	Attribute *oldAttribute = m_attribute;
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

	vString = version + vString;
	fString = version + fString;

#if !TARGET_OS_IPHONE
	if ( m_shader == 0xffffffff )
		m_shader = vsRenderer_OpenGL3::Compile( vString.c_str(), fString.c_str(), vString.size(), fString.size() );
	else
		vsRenderer_OpenGL3::Compile( m_shader, vString.c_str(), fString.c_str(), vString.size(), fString.size(), false );
	// vsLog("Created shader %d", m_shader);
#endif // TARGET_OS_IPHONE

	m_colorLoc = glGetUniformLocation(m_shader, "universal_color");
	m_instanceColorAttributeLoc = glGetAttribLocation(m_shader, "instanceColorAttrib");
	m_resolutionLoc = glGetUniformLocation(m_shader, "resolution");
	m_mouseLoc = glGetUniformLocation(m_shader, "mouse");
	// m_fogLoc = glGetUniformLocation(m_shader, "fog");
	m_textureLoc = glGetUniformLocation(m_shader, "textures");
	m_shadowTextureLoc = glGetUniformLocation(m_shader, "shadowTexture");
	m_bufferTextureLoc = glGetUniformLocation(m_shader, "bufferTexture");
	m_localToWorldLoc = glGetUniformLocation(m_shader, "localToWorld");
	m_worldToViewLoc = glGetUniformLocation(m_shader, "worldToView");
	m_viewToProjectionLoc = glGetUniformLocation(m_shader, "viewToProjection");
	m_cameraPositionLoc = glGetUniformLocation(m_shader, "cameraPosition");

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
	glGetProgramiv( m_shader, GL_ACTIVE_UNIFORMS, &m_uniformCount );
	glGetProgramiv( m_shader, GL_ACTIVE_ATTRIBUTES, &m_attributeCount );

	m_uniform = new Uniform[m_uniformCount];
	m_attribute = new Attribute[m_attributeCount];

	const int c_maxNameLength = 256;
	char nameBuffer[c_maxNameLength];
	int defaultSamplerBinding = 0;
	for ( GLint i = 0; i < m_uniformCount; i++ )
	{
		GL_CHECK("Shader::Uniform");
		GLint arraySize = 0;
		GLenum type = 0;
		GLsizei actualLength = 0;
		glGetActiveUniform(m_shader, i, c_maxNameLength, &actualLength, &arraySize, &type, nameBuffer);
		m_uniform[i].name = vsString(nameBuffer);
		m_uniform[i].loc = glGetUniformLocation(m_shader, nameBuffer);
		m_uniform[i].type = type;
		m_uniform[i].arraySize = arraySize;
		m_uniform[i].b = false;
		m_uniform[i].f32 = 0.f;

		// initialise to random values, so we definitely set them at least once.
		switch ( m_uniform[i].type )
		{
			case GL_BOOL:
			case GL_INT:
				glGetUniformiv( m_shader, m_uniform[i].loc, &m_uniform[i].b );
				break;
			case GL_FLOAT:
				glGetUniformfv( m_shader, m_uniform[i].loc, &m_uniform[i].f32 );
				break;
			case GL_FLOAT_VEC3:
				glGetUniformfv( m_shader, m_uniform[i].loc, &m_uniform[i].vec4.x );
				break;
			case GL_FLOAT_VEC4:
				glGetUniformfv( m_shader, m_uniform[i].loc, &m_uniform[i].vec4.x );
				break;
			case GL_SAMPLER_2D:
				m_uniform[i].b = defaultSamplerBinding;
				defaultSamplerBinding += m_uniform[i].arraySize;
				break;
			case GL_INT_SAMPLER_BUFFER:
			case GL_SAMPLER_BUFFER:
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
				m_uniform[i].b = 9;
				// m_uniform[i].b = defaultSamplerBinding;
				// defaultSamplerBinding += m_uniform[i].arraySize;
				break;
			default:
				break;
		}
		GL_CHECK("Shader::Uniform");
	}
	GL_CHECK("Shader::Between");
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
	m_fogDensityId = GetUniformId("fogDensity");
	m_fogColorId = GetUniformId("fogColor");

	vsDeleteArray( oldUniform );
	vsDeleteArray( oldAttribute );
}

vsShader::~vsShader()
{
	// vsLog("Destroyed shader %d", m_shader);
	vsRenderer_OpenGL3::DestroyShader(m_shader);
	vsDeleteArray( m_uniform );
	vsDeleteArray( m_attribute );
}

vsShader *
vsShader::Load( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture )
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

	vsShader *result =  new vsShader(vString, fString, lit, texture);
	result->m_vertexShaderFile = vertexShader;
	result->m_fragmentShaderFile = fragmentShader;
	result->m_litBool = lit;
	result->m_textureBool = texture;


	delete vStore;
	delete fStore;

	return result;
}

void
vsShader::ReloadAll()
{
	vsShader *s = vsShader::GetFirstInstance();
	while ( s )
	{
		if ( s->m_vertexShaderFile != vsEmptyString && s->m_fragmentShaderFile != vsEmptyString )
		{
			vsFile vShader( vsString("shaders/") + s->m_vertexShaderFile, vsFile::MODE_Read );
			vsFile fShader( vsString("shaders/") + s->m_fragmentShaderFile, vsFile::MODE_Read );

			uint32_t vSize = vShader.GetLength();
			uint32_t fSize = fShader.GetLength();

			vsStore *vStore = new vsStore(vSize);
			vsStore *fStore = new vsStore(fSize);

			vShader.Store( vStore );
			fShader.Store( fStore );
			vsString vString( vStore->GetReadHead(), vSize );
			vsString fString( fStore->GetReadHead(), fSize );

			s->Compile( vString, fString, s->m_litBool, s->m_textureBool );

			delete vStore;
			delete fStore;
		}

		s = s->GetNextInstance();
	}
}

void
vsShader::SetFog( bool fog, const vsColor& color, float density )
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
vsShader::SetColor( const vsColor& color )
{
	if ( m_colorLoc >= 0 )
	{
		glUniform4f( m_colorLoc, color.r, color.g, color.b, color.a );
	}
	// this is vertex color;  don't set that!
	glVertexAttrib4f( 3, 1.f, 1.f, 1.f, 1.f );
}

void
vsShader::SetInstanceColors( vsRenderBuffer *colors )
{
	if ( m_instanceColorAttributeLoc >= 0 )
	{
		if ( !m_colorAttribIsActive )
		{
			glEnableVertexAttribArray(m_instanceColorAttributeLoc);
			glVertexAttribDivisor(m_instanceColorAttributeLoc, 1);
			m_colorAttribIsActive = true;
		}

		colors->BindAsAttribute( m_instanceColorAttributeLoc );
	}
	// CheckGLError("SetColors");
}

void
vsShader::SetInstanceColors( const vsColor* color, int matCount )
{
	if ( matCount <= 0 )
		return;
	// CheckGLError("SetInstanceColors");
	// if ( m_colorLoc >= 0 )
	// {
	// 	glUniform4f( m_colorLoc, color[0].r, color[0].g, color[0].b, color[0].a );
	// }
	// glVertexAttrib4f( 3, color[0].r, color[0].g, color[0].b, color[0].a );
	if ( m_instanceColorAttributeLoc >= 0 )
	{
		if ( matCount == 1 )
		{
			if ( m_colorAttribIsActive )
			{
				glDisableVertexAttribArray(m_instanceColorAttributeLoc);
				m_colorAttribIsActive = false;
			}

			glVertexAttrib4f(m_instanceColorAttributeLoc, color[0].r, color[0].g, color[0].b, color[0].a);
		}
		else
		{
			if ( !m_colorAttribIsActive )
			{
				glEnableVertexAttribArray(m_instanceColorAttributeLoc);
				glVertexAttribDivisor(m_instanceColorAttributeLoc, 1);
				m_colorAttribIsActive = true;
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
			glVertexAttribPointer(m_instanceColorAttributeLoc, 4, GL_FLOAT, 0, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
	// CheckGLError("SetColors");
}


void
vsShader::SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] )
{
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
vsShader::SetLocalToWorld( vsRenderBuffer* buffer )
{
	if ( m_localToWorldAttributeLoc >= 0 )
	{
		if ( !m_localToWorldAttribIsActive )
		{
			glEnableVertexAttribArray(m_localToWorldAttributeLoc);
			glEnableVertexAttribArray(m_localToWorldAttributeLoc+1);
			glEnableVertexAttribArray(m_localToWorldAttributeLoc+2);
			glEnableVertexAttribArray(m_localToWorldAttributeLoc+3);
			glVertexAttribDivisor(m_localToWorldAttributeLoc, 1);
			glVertexAttribDivisor(m_localToWorldAttributeLoc+1, 1);
			glVertexAttribDivisor(m_localToWorldAttributeLoc+2, 1);
			glVertexAttribDivisor(m_localToWorldAttributeLoc+3, 1);
			m_localToWorldAttribIsActive = true;
		}

		buffer->BindAsAttribute( m_localToWorldAttributeLoc );
	}
}

void
vsShader::SetLocalToWorld( const vsMatrix4x4* localToWorld, int matCount )
{
	if ( m_localToWorldLoc >= 0 )
	{
		glUniformMatrix4fv( m_localToWorldLoc, 1, false, (GLfloat*)localToWorld );
	}
	if ( m_localToWorldAttributeLoc >= 0 )
	{
		if ( matCount == 1 )
		{
			if ( m_localToWorldAttribIsActive )
			{
				glDisableVertexAttribArray(m_localToWorldAttributeLoc);
				glDisableVertexAttribArray(m_localToWorldAttributeLoc+1);
				glDisableVertexAttribArray(m_localToWorldAttributeLoc+2);
				glDisableVertexAttribArray(m_localToWorldAttributeLoc+3);
				m_localToWorldAttribIsActive = false;
			}

			glVertexAttrib4f(m_localToWorldAttributeLoc, localToWorld->x.x, localToWorld->x.y, localToWorld->x.z, localToWorld->x.w );
			glVertexAttrib4f(m_localToWorldAttributeLoc+1, localToWorld->y.x, localToWorld->y.y, localToWorld->y.z, localToWorld->y.w );
			glVertexAttrib4f(m_localToWorldAttributeLoc+2, localToWorld->z.x, localToWorld->z.y, localToWorld->z.z, localToWorld->z.w );
			glVertexAttrib4f(m_localToWorldAttributeLoc+3, localToWorld->w.x, localToWorld->w.y, localToWorld->w.z, localToWorld->w.w );
		}
		else
		{
			if ( !m_localToWorldAttribIsActive )
			{
				glEnableVertexAttribArray(m_localToWorldAttributeLoc);
				glEnableVertexAttribArray(m_localToWorldAttributeLoc+1);
				glEnableVertexAttribArray(m_localToWorldAttributeLoc+2);
				glEnableVertexAttribArray(m_localToWorldAttributeLoc+3);
				glVertexAttribDivisor(m_localToWorldAttributeLoc, 1);
				glVertexAttribDivisor(m_localToWorldAttributeLoc+1, 1);
				glVertexAttribDivisor(m_localToWorldAttributeLoc+2, 1);
				glVertexAttribDivisor(m_localToWorldAttributeLoc+3, 1);
				m_localToWorldAttribIsActive = true;
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
			glVertexAttribPointer(m_localToWorldAttributeLoc, 4, GL_FLOAT, 0, 64, 0);
			glVertexAttribPointer(m_localToWorldAttributeLoc+1, 4, GL_FLOAT, 0, 64, (void*)16);
			glVertexAttribPointer(m_localToWorldAttributeLoc+2, 4, GL_FLOAT, 0, 64, (void*)32);
			glVertexAttribPointer(m_localToWorldAttributeLoc+3, 4, GL_FLOAT, 0, 64, (void*)48);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
}

void
vsShader::SetWorldToView( const vsMatrix4x4& worldToView )
{
	if ( m_worldToViewLoc >= 0 )
	{
		glUniformMatrix4fv( m_worldToViewLoc, 1, false, (GLfloat*)&worldToView );
	}
	// assume no scaling.
	if ( m_cameraPositionLoc >= 0 )
	{
		vsVector3D t = worldToView.Inverse().w;
		glUniform3fv(m_cameraPositionLoc, 1, (GLfloat*)&t);
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

int32_t
vsShader::GetUniformId(const vsString& name) const
{
	for ( int i = 0; i < m_uniformCount; i++ )
	{
		if ( m_uniform[i].name == name )
			return i;
	}
	return -1;
}

void
vsShader::SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector )
{
	if ( id != 0 )
		return;
	if ( m_lightAmbientLoc >= 0 )
	{
		glUniform4fv( m_lightAmbientLoc, 1, (GLfloat*)&ambient );
	}
	if ( m_lightDiffuseLoc >= 0 )
	{
		glUniform4fv( m_lightDiffuseLoc, 1, (GLfloat*)&diffuse );
	}
	if ( m_lightSpecularLoc >= 0 )
	{
		glUniform4fv( m_lightSpecularLoc, 1, (GLfloat*)&specular );
	}
	if ( m_lightPositionLoc >= 0 )
	{
		glUniform3fv( m_lightPositionLoc, 1, (GLfloat*)&position );
	}
	if ( m_lightHalfVectorLoc >= 0 )
	{
		glUniform3fv( m_lightHalfVectorLoc, 1, (GLfloat*)&halfVector );
	}
}

void
vsShader::Prepare( vsMaterial *material, vsShaderValues *values )
{
	// GLint current;
	// glGetIntegerv(GL_CURRENT_PROGRAM, &current);
	// vsAssert( current == (GLint)m_shader, "This shader isn't currently active??" );
	for ( int i = 0; i < m_uniformCount; i++ )
	{
		switch( m_uniform[i].type )
		{
			case GL_BOOL:
				{
					bool b;
					if ( !values || !values->UniformB( m_uniform[i].name, b ) )
						 b = material->UniformB(i);
					SetUniformValueB( i, b );
					break;
				}
			case GL_FLOAT:
				{
					if ( m_uniform[i].arraySize == 1 )
					{
						float f;
						if ( !values || !values->UniformF( m_uniform[i].name, f ) )
							f = material->UniformF(i);
						SetUniformValueF( i, f );
					}
					break;
				}
			case GL_FLOAT_VEC3:
				{
					vsVector4D v;
					if ( !values || !values->UniformVec4( m_uniform[i].name, v ) )
						v = material->UniformVec4(i);
					SetUniformValueVec3( i, v );
					break;
				}
			case GL_FLOAT_VEC4:
				{
					vsVector4D v;
					if ( !values || !values->UniformVec4( m_uniform[i].name, v ) )
						v = material->UniformVec4(i);
					SetUniformValueVec4( i, v );
					break;
				}
			case GL_FLOAT_MAT4:
				{
					vsMatrix4x4 v;
					if ( !values || !values->UniformMat4( m_uniform[i].name, v ) )
						v = material->UniformMat4(i);
					SetUniformValueMat4( i, v );
					break;
				}
			case GL_INT:
			case GL_SAMPLER_2D:
			case GL_INT_SAMPLER_BUFFER:
			case GL_SAMPLER_BUFFER:
				{
					int b;
					// TODO:  Expose samplers through shadervalue objects, maybe?
					b = material->UniformI(i);
					SetUniformValueI( i, b );
					break;
				}

			default:
				// TODO:  Handle more uniform types
				break;
		}
	}

	if ( m_resolutionLoc >= 0 )
	{
		int xRes = vsScreen::Instance()->GetWidth();
		int yRes = vsScreen::Instance()->GetHeight();
		glUniform2f( m_resolutionLoc, (float)xRes, (float)yRes );
	}
	if ( m_globalTimeUniformId >= 0 )
	{
		int milliseconds = vsTimerSystem::Instance()->GetMicrosecondsSinceInit() / 1000;
		float seconds = milliseconds / 1000.f;
		SetUniformValueF( m_globalTimeUniformId, seconds );
	}
	if ( m_mouseLoc >= 0 )
	{
		vsVector2D mousePos = vsInput::Instance()->GetWindowMousePosition();
		int yRes = vsScreen::Instance()->GetHeight();
		// the coordinate system in the GLSL shader is inverted from the
		// coordinate system we like to use.  So let's invert it!
		glUniform2f( m_mouseLoc, mousePos.x, yRes - mousePos.y );
	}
}

void
vsShader::ValidateCache( vsMaterial *activeMaterial )
{
	return;
/*
	for ( int i = 0; i < m_uniformCount; i++ )
	{
		switch( m_uniform[i].type )
		{
			case GL_BOOL:
				{
					GLint valueNow = -1;
					glGetUniformiv( m_shader, m_uniform[i].loc, &valueNow );
					CheckGLError("Test");
					vsAssert( valueNow != -1, "-1??" );
					vsAssert( valueNow == m_uniform[i].b, "Caching system is broken?" );
					break;
				}
			case GL_FLOAT:
				{
					if ( m_uniform[i].arraySize == 1 )
					{
						float valueNow = -1.f;
						glGetUniformfv( m_shader, m_uniform[i].loc, &valueNow );
						CheckGLError("Test");
						vsAssert( valueNow != -1.f, "-1??" );
						vsAssert( valueNow == m_uniform[i].f32, "Caching system is broken?" );
					}
					break;
				}
		}
	}
	*/
}

void
vsShader::SetUniformValueF( int i, float value )
{
	if ( value != m_uniform[i].f32 )
	{
		glUniform1f( m_uniform[i].loc, value );
		m_uniform[i].f32 = value;
	}
}

void
vsShader::SetUniformValueB( int i, bool value )
{
	if ( value != m_uniform[i].b )
	{
		glUniform1i( m_uniform[i].loc, value );
		m_uniform[i].b = value;
	}
}

void
vsShader::SetUniformValueI( int i, int value )
{
	for ( int j = 0; j < m_uniform[i].arraySize; j++ )
	{
		glUniform1i( m_uniform[i].loc+j, value+j );
	}
}

void
vsShader::SetUniformValueVec3( int i, const vsVector3D& value )
{
	glUniform3f( m_uniform[i].loc, value.x, value.y, value.z );
	m_uniform[i].vec4 = value;
}

void
vsShader::SetUniformValueVec3( int i, const vsColor& value )
{
	glUniform3f( m_uniform[i].loc, value.r, value.g, value.b );
	m_uniform[i].vec4.Set(value.r, value.g, value.b, 0.f);
}

void
vsShader::SetUniformValueVec4( int i, const vsVector4D& value )
{
	glUniform4f( m_uniform[i].loc, value.x, value.y, value.z, value.w );
	m_uniform[i].vec4 = value;
}

void
vsShader::SetUniformValueVec4( int i, const vsColor& value )
{
	glUniform4f( m_uniform[i].loc, value.r, value.g, value.b, value.a );
	m_uniform[i].vec4.Set(value.r, value.g, value.b, value.a);
}

void
vsShader::SetUniformValueMat4( int i, const vsMatrix4x4& value )
{
	glUniformMatrix4fv( m_uniform[i].loc, 1, GL_FALSE, (const GLfloat*)&value );
}

