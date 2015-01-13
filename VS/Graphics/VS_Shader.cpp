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
#include "VS_Store.h"
#include "VS_System.h"
#include "VS_TimerSystem.h"

static bool m_localToWorldAttribIsActive = false;
static bool m_colorAttribIsActive = false;


vsShader::vsShader( const vsString &vertexShader, const vsString &fragmentShader, bool lit, bool texture ):
	m_shader(-1)
{
	vsString version;

	vsString vString = vertexShader;
	vsString fString = fragmentShader;

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
	m_shader = vsRenderer_OpenGL3::Compile( vString.c_str(), fString.c_str(), vString.size(), fString.size() );
	//m_shader = vsRenderSchemeShader::Instance()->Compile( vStore->GetReadHead(), fStore->GetReadHead(), vSize, fSize);
#endif // TARGET_OS_IPHONE

	m_alphaRefLoc = glGetUniformLocation(m_shader, "alphaRef");
	m_colorLoc = glGetUniformLocation(m_shader, "universal_color");
	m_instanceColorAttributeLoc = glGetAttribLocation(m_shader, "instanceColorAttrib");
	m_resolutionLoc = glGetUniformLocation(m_shader, "resolution");
	m_globalTimeLoc = glGetUniformLocation(m_shader, "globalTime");
	m_mouseLoc = glGetUniformLocation(m_shader, "mouse");
	m_fogLoc = glGetUniformLocation(m_shader, "fog");
	m_fogDensityLoc = glGetUniformLocation(m_shader, "fogDensity");
	m_fogColorLoc = glGetUniformLocation(m_shader, "fogColor");
	m_textureLoc = glGetUniformLocation(m_shader, "textures");
	m_localToWorldLoc = glGetUniformLocation(m_shader, "localToWorld");
	m_worldToViewLoc = glGetUniformLocation(m_shader, "worldToView");
	m_viewToProjectionLoc = glGetUniformLocation(m_shader, "viewToProjection");
	m_glowLoc = glGetUniformLocation(m_shader, "glow");

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
	for ( GLint i = 0; i < m_uniformCount; i++ )
	{
		GLint arraySize = 0;
		GLenum type = 0;
		GLsizei actualLength = 0;
		glGetActiveUniform(m_shader, i, c_maxNameLength, &actualLength, &arraySize, &type, nameBuffer);
		m_uniform[i].name = vsString(nameBuffer);
		m_uniform[i].loc = glGetUniformLocation(m_shader, nameBuffer);
		m_uniform[i].type = type;
		m_uniform[i].arraySize = arraySize;
		switch ( m_uniform[i].type )
		{
			case GL_BOOL:
				m_uniform[i].b = 0;
				break;
			case GL_FLOAT:
				m_uniform[i].f32 = 0.f;
				break;
			default:
				break;
		}
	}
	for ( GLint i = 0; i < m_attributeCount; i++ )
	{
		GLint arraySize = 0;
		GLenum type = 0;
		GLsizei actualLength = 0;
		glGetActiveAttrib(m_shader, i, c_maxNameLength, &actualLength, &arraySize, &type, nameBuffer);
		m_attribute[i].name = vsString(nameBuffer);
		m_attribute[i].loc = glGetAttribLocation(m_shader, m_attribute[i].name.c_str());
		m_attribute[i].type = type;
		m_attribute[i].arraySize = arraySize;
	}
}

vsShader::~vsShader()
{
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

	delete vStore;
	delete fStore;

	return result;
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
vsShader::SetFog( bool fog, const vsColor& color, float density )
{
	if ( m_fogLoc >= 0 )
	{
		glUniform1i( m_fogLoc, fog );
	}
	if ( m_fogColorLoc >= 0 )
	{
		glUniform3f( m_fogColorLoc, color.r, color.g, color.b );
	}
	if ( m_fogDensityLoc >= 0 )
	{
		glUniform1f( m_fogDensityLoc, density );
	}
}

void
vsShader::SetColor( const vsColor& color )
{
	if ( m_colorLoc >= 0 )
	{
		glUniform4f( m_colorLoc, color.r, color.g, color.b, color.a );
	}

	glVertexAttrib4f( 3, color.r, color.g, color.b, color.a );
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
	CheckGLError("SetColors");
}

void
vsShader::SetInstanceColors( const vsColor* color, int matCount )
{
	if ( matCount <= 0 )
		return;
	CheckGLError("SetInstanceColors");
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

			static GLuint g_vbo = 0xffffffff;
			// this could be a lot smarter.
			if ( g_vbo == 0xffffffff )
			{
				glGenBuffers(1, &g_vbo);
			}
			glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vsColor) * matCount, color, GL_STREAM_DRAW);
			glVertexAttribPointer(m_instanceColorAttributeLoc, 4, GL_FLOAT, 0, 0, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
	CheckGLError("SetColors");
}


void
vsShader::SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] )
{
	if ( m_textureLoc >= 0 )
	{
		const GLint value[MAX_TEXTURE_SLOTS] = { 0, 1, 2, 3, 4, 5, 6, 7 };
		glUniform1iv( m_textureLoc, MAX_TEXTURE_SLOTS, value );
	}
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
			// this could be a lot smarter.
			if ( g_vbo == 0xffffffff )
			{
				glGenBuffers(1, &g_vbo);
			}
			vsAssert( sizeof(vsMatrix4x4) == 64, "Whaa?" );
			glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vsMatrix4x4) * matCount, localToWorld, GL_STREAM_DRAW);
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
vsShader::SetGlow( float glowAlpha )
{
	if ( m_glowLoc >= 0 )
	{
		glUniform1f( m_glowLoc, glowAlpha );
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
vsShader::Prepare( vsMaterial *material )
{
	for ( int i = 0; i < m_uniformCount; i++ )
	{
		switch( m_uniform[i].type )
		{
			case GL_BOOL:
				{
					bool b = material->UniformB(i);
					if ( b != m_uniform[i].b )
						SetUniformValueB( i, b );
					break;
				}
			case GL_FLOAT:
				{
					float f = material->UniformF(i);
					if ( f != m_uniform[i].f32 )
						SetUniformValueF( i, f );
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
		int yRes = vsScreen::Instance()->GetHeight();
		// the coordinate system in the GLSL shader is inverted from the
		// coordinate system we like to use.  So let's invert it!
		glUniform2f( m_mouseLoc, mousePos.x, yRes - mousePos.y );
	}
}

void
vsShader::SetUniformValueF( int i, float value )
{
	glUniform1f( m_uniform[i].loc, value );
	m_uniform[i].f32 = value;
}

void
vsShader::SetUniformValueB( int i, bool value )
{
	glUniform1i( m_uniform[i].loc, value );
	m_uniform[i].b = value;
}

