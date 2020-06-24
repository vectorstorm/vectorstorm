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
#include "VS_RenderTarget.h"
#include "VS_Renderer_OpenGL3.h"
#include "VS_Screen.h"
#include "VS_ShaderValues.h"
#include "VS_ShaderVariant.h"
#include "VS_Store.h"
#include "VS_System.h"
#include "VS_TimerSystem.h"
#include "VS_DisableDebugNew.h"
#include <algorithm>
#include "VS_EnableDebugNew.h"

// static bool m_localToWorldAttribIsActive = false;
// static bool m_colorAttribIsActive = false;
// namespace
// {
	vsArray<vsShaderVariantDefinition> g_shaderVariantDefinitions;
// }

void
vsShader::SetShaderVariantDefinitions( const vsArray<vsShaderVariantDefinition>& definitions )
{
	g_shaderVariantDefinitions = definitions;
}

vsShader::vsShader( const vsString &vertexShader,
		const vsString &fragmentShader,
		bool lit,
		bool texture,
		uint32_t variantBits,
		const vsString& vFilename,
		const vsString& fFilename ):
	m_vertexShaderFile(vFilename),
	m_fragmentShaderFile(fFilename),
	m_variantBitsSupported(0L),
	m_system(false),
	m_litBool(lit),
	m_textureBool(texture)
{
	FigureOutAvailableVariants( vertexShader );
	FigureOutAvailableVariants( fragmentShader );

	GL_CHECK_SCOPED("Shader");
	m_current = new vsShaderVariant( vertexShader, fragmentShader, lit, texture, variantBits, vFilename, fFilename );
	// Compile( vertexShader, fragmentShader, lit, texture, m_variantBits );
	// m_current->Compile( vertexShader, fragmentShader, lit, texture, variantBits );

	m_variant.AddItem(m_current);
}

vsShader::~vsShader()
{
	for ( int i = 0; i < m_variant.ItemCount(); i++ )
		vsDelete( m_variant[i] );
	m_variant.Clear();
	m_current = NULL;
	// vsDelete( m_current );
	// vsLog("Destroyed shader %d", m_shader);
	// vsRenderer_OpenGL3::DestroyShader(m_shader);
	// vsDeleteArray( m_uniform );
	// vsDeleteArray( m_attribute );
}

void
vsShader::FigureOutAvailableVariants( const vsString& s_in )
{
	vsString s(s_in);
	size_t variantPos;
	bool done = false;
	vsString pattern("\n// variant ");
	while (!done)
	{
		done = true;
		if ( (variantPos = s.find(pattern)) != vsString::npos )
		{
			done = false;
			size_t nextNewLine = s.find('\n', variantPos+1);
			vsString variant = s.substr(variantPos+pattern.size(), nextNewLine-(variantPos+pattern.size()));
			s.erase(variantPos, nextNewLine-variantPos);
			// vsLog("Variant: %s", variant);

			// [TODO] Figure out the id number of this variant.

			for ( int i = 0; i < g_shaderVariantDefinitions.ItemCount(); i++ )
			{
				if ( g_shaderVariantDefinitions[i].name == variant )
				{
					vsLog("Has Variant: %s", variant);
					m_variantBitsSupported |= BIT(g_shaderVariantDefinitions[i].bitId);
				}
			}
		}
	}

}

vsShader *
vsShader::Load( const vsString &vFile, const vsString &fFile, bool lit, bool texture )
{
	vsFile vShader( vsString("shaders/") + vFile, vsFile::MODE_Read );
	vsFile fShader( vsString("shaders/") + fFile, vsFile::MODE_Read );

	uint32_t vSize = vShader.GetLength();
	uint32_t fSize = fShader.GetLength();

	vsStore *vStore = new vsStore(vSize);
	vsStore *fStore = new vsStore(fSize);

	vShader.Store( vStore );
	fShader.Store( fStore );
	vsString vString( vStore->GetReadHead(), vSize );
	vsString fString( fStore->GetReadHead(), fSize );

	uint32_t variantBits = 0L;
	vsShader *result = new vsShader(vString, fString, lit, texture, variantBits, vFile, fFile);
	// result->m_litBool = lit;
	// result->m_textureBool = texture;


	delete vStore;
	delete fStore;

	return result;
}

vsShader *
vsShader::Load_System( const vsString &vFile, const vsString &fFile, bool lit, bool texture )
{
	vsShader *result = Load(vFile, fFile, lit, texture);
	result->m_system = true;
	return result;
}

void
vsShader::Reload()
{
	// system-owned shader;  don't reload it!
	if ( m_system )
		return;

	// Note:  If I ever really need to be able to reload system-owned shaders
	// at runtime (right now, this is only default_v/default_f), this can probably
	// be made to work by calling	`vsHeap::Push(g_globalHeap);` here at the start
	// of the function, and popping it back off at the bottom.

	if ( !m_vertexShaderFile.empty() && !m_fragmentShaderFile.empty() )
	{
		for ( int i = 0; i < m_variant.ItemCount(); i++ )
			m_variant[i]->Reload();
		// vsFile vShader( vsString("shaders/") + m_vertexShaderFile, vsFile::MODE_Read );
		// vsFile fShader( vsString("shaders/") + m_fragmentShaderFile, vsFile::MODE_Read );
        //
		// uint32_t vSize = vShader.GetLength();
		// uint32_t fSize = fShader.GetLength();
        //
		// vsStore *vStore = new vsStore(vSize);
		// vsStore *fStore = new vsStore(fSize);
        //
		// vShader.Store( vStore );
		// fShader.Store( fStore );
		// vsString vString( vStore->GetReadHead(), vSize );
		// vsString fString( fStore->GetReadHead(), fSize );
        //
		// Compile( vString, fString, m_litBool, m_textureBool, m_variantBits );
        //
		// delete vStore;
		// delete fStore;
	}
}

void
vsShader::ReloadAll()
{
	vsShader *s = vsShader::GetFirstInstance();
	while ( s )
	{
		s->Reload();
		s = s->GetNextInstance();
	}
}

uint32_t
vsShader::GetShaderId() const
{
	return m_current->GetShaderId();
}

uint32_t
vsShader::GetCurrentVariantBits()
{
	return m_current->GetVariantBits();
}

void
vsShader::SetForVariantBits( uint32_t bits )
{
	vsAssert( (bits & m_variantBitsSupported) == bits, "Client asked for bits we don't support??" );

	if ( m_current->GetVariantBits() == bits )
		return; // nothing to do!
	else
	{
		// we need to iterate over our variants looking for one which will do
		// what we want.  If we can't find one, we'll need to create a new one.

		for ( int i = 0; i < m_variant.ItemCount(); i++ )
		{
			if ( m_variant[i]->GetVariantBits() == bits )
			{
				m_current = m_variant[i];
				return;
			}
		}

		// Okay, couldn't find one;  we need to make one!
		{
			vsFile vShader( vsString("shaders/") + m_vertexShaderFile, vsFile::MODE_Read );
			vsFile fShader( vsString("shaders/") + m_fragmentShaderFile, vsFile::MODE_Read );

			uint32_t vSize = vShader.GetLength();
			uint32_t fSize = fShader.GetLength();

			vsStore *vStore = new vsStore(vSize);
			vsStore *fStore = new vsStore(fSize);

			vShader.Store( vStore );
			fShader.Store( fStore );
			vsString vString( vStore->GetReadHead(), vSize );
			vsString fString( fStore->GetReadHead(), fSize );

			uint32_t variantBits = bits;
			vsShaderVariant *result =
				new vsShaderVariant(vString, fString, m_litBool, m_textureBool,
						variantBits, m_vertexShaderFile, m_fragmentShaderFile);

			m_current = result;
			m_variant.AddItem(m_current);

			delete vStore;
			delete fStore;
		}
	}
}

void
vsShader::SetFog( bool fog, const vsColor& color, float density )
{
	m_current->SetFog(fog, color, density);
	// if ( m_fogColorId >= 0 )
	// {
	// 	// glUniform3f( m_fogColorLoc, color.r, color.g, color.b );
	// 	SetUniformValueVec3(m_fogColorId, color);
	// }
	// if ( m_fogDensityId >= 0 )
	// {
	// 	if ( density >= 1.f )
	// 		vsLogOnce( "Setting surprisingly high fog density: %f", density );
	// 	SetUniformValueF(m_fogDensityId, density);
	// }
}

void
vsShader::SetColor( const vsColor& color )
{
	m_current->SetColor(color);
	// if ( m_colorLoc >= 0 )
	// {
	// 	glUniform4f( m_colorLoc, color.r, color.g, color.b, color.a );
	// }
	// // this is vertex color;  don't set that!
	// glVertexAttrib4f( 3, 1.f, 1.f, 1.f, 1.f );
}

void
vsShader::SetInstanceColors( vsRenderBuffer *colors )
{
	m_current->SetInstanceColors(colors);
	// if ( m_hasInstanceColorsLoc >= 0 )
	// 	glUniform1i( m_hasInstanceColorsLoc, true );
	// if ( m_instanceColorAttributeLoc >= 0 )
	// {
	// 	if ( !m_colorAttribIsActive )
	// 	{
	// 		glEnableVertexAttribArray(m_instanceColorAttributeLoc);
	// 		glVertexAttribDivisor(m_instanceColorAttributeLoc, 1);
	// 		m_colorAttribIsActive = true;
	// 	}
    //
	// 	colors->BindAsAttribute( m_instanceColorAttributeLoc );
	// }
	// GL_CHECK("SetColors");
}

void
vsShader::SetInstanceColors( const vsColor* color, int matCount )
{
	m_current->SetInstanceColors(color, matCount);
// 	if ( m_hasInstanceColorsLoc >= 0 )
// 		glUniform1i( m_hasInstanceColorsLoc, ( matCount >= 2 ) );
// 	if ( matCount <= 0 )
// 		return;
// 	// GL_CHECK("SetInstanceColors");
// 	// if ( m_colorLoc >= 0 )
// 	// {
// 	// 	glUniform4f( m_colorLoc, color[0].r, color[0].g, color[0].b, color[0].a );
// 	// }
// 	// glVertexAttrib4f( 3, color[0].r, color[0].g, color[0].b, color[0].a );
//
// 	if ( m_instanceColorAttributeLoc >= 0 )
// 	{
// 		if ( matCount == 1 )
// 		{
// 			if ( m_colorAttribIsActive )
// 			{
// 				glDisableVertexAttribArray(m_instanceColorAttributeLoc);
// 				m_colorAttribIsActive = false;
// 			}
//
// 			glVertexAttrib4f(m_instanceColorAttributeLoc, color[0].r, color[0].g, color[0].b, color[0].a);
// 		}
// 		else
// 		{
// 			if ( !m_colorAttribIsActive )
// 			{
// 				glEnableVertexAttribArray(m_instanceColorAttributeLoc);
// 				glVertexAttribDivisor(m_instanceColorAttributeLoc, 1);
// 				m_colorAttribIsActive = true;
// 			}
//
// 			GLuint size = sizeof(vsColor)*matCount;
// 			static GLuint g_vbo = 0xffffffff;
// 			static GLuint g_vboSize = 0;
// 			// this could be a lot smarter.
// 			if ( g_vbo == 0xffffffff )
// 			{
// 				glGenBuffers(1, &g_vbo);
// 			}
//
// 			glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
// 			if ( size > g_vboSize )
// 			{
// 				glBufferData(GL_ARRAY_BUFFER, size, color, GL_STREAM_DRAW);
// 				g_vboSize = size;
// 			}
// 			else
// 			{
// 				void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
// 				if ( ptr )
// 				{
// 					memcpy(ptr, color, size);
// 					glUnmapBuffer(GL_ARRAY_BUFFER);
// 				}
// 			}
// 			glVertexAttribPointer(m_instanceColorAttributeLoc, 4, GL_FLOAT, 0, 0, 0);
// #ifdef VS_PRISTINE_BINDINGS
// 			glBindBuffer(GL_ARRAY_BUFFER, 0);
// #endif // VS_PRISTINE_BINDINGS
// 		}
// 	}
	// GL_CHECK("SetColors");
}


void
vsShader::SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] )
{
	m_current->SetTextures(texture);
	// // NO NEED TO DO THIS ANY MORE:  TEXTURES ARE NOW BEING BOUND GENERICALLY.
	// //
	// if ( m_textureLoc >= 0 )
	// {
	// 	const GLint value[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	// 	glUniform1iv( m_textureLoc, 8, value );
	// }
	// // if ( m_shadowTextureLoc >= 0 )
	// // {
	// // 	glUniform1i( m_shadowTextureLoc, 8 );
	// // }
	// // if ( m_bufferTextureLoc >= 0 )
	// // {
	// // 	glUniform1i( m_bufferTextureLoc, 9 );
	// // }
}

void
vsShader::SetLocalToWorld( vsRenderBuffer* buffer )
{
	m_current->SetLocalToWorld(buffer);
	// if ( m_localToWorldAttributeLoc >= 0 )
	// {
	// 	if ( !m_localToWorldAttribIsActive )
	// 	{
	// 		glEnableVertexAttribArray(m_localToWorldAttributeLoc);
	// 		glEnableVertexAttribArray(m_localToWorldAttributeLoc+1);
	// 		glEnableVertexAttribArray(m_localToWorldAttributeLoc+2);
	// 		glEnableVertexAttribArray(m_localToWorldAttributeLoc+3);
	// 		glVertexAttribDivisor(m_localToWorldAttributeLoc, 1);
	// 		glVertexAttribDivisor(m_localToWorldAttributeLoc+1, 1);
	// 		glVertexAttribDivisor(m_localToWorldAttributeLoc+2, 1);
	// 		glVertexAttribDivisor(m_localToWorldAttributeLoc+3, 1);
	// 		m_localToWorldAttribIsActive = true;
	// 	}
    //
	// 	buffer->BindAsAttribute( m_localToWorldAttributeLoc );
	// }
}

void
vsShader::SetLocalToWorld( const vsMatrix4x4* localToWorld, int matCount )
{
	m_current->SetLocalToWorld(localToWorld,matCount);
// 	if ( m_localToWorldLoc >= 0 )
// 	{
//  		if ( matCount == 1 )
// 			glUniformMatrix4fv( m_localToWorldLoc, 1, false, (GLfloat*)localToWorld );
// 		else
// 		{
// 			vsMatrix4x4 inv;
// 			inv.x.x = -2.f;
// 			glUniformMatrix4fv( m_localToWorldLoc, 1, false, (GLfloat*)&inv );
// 		}
// 	}
// 	if ( m_localToWorldAttributeLoc >= 0 )
// 	{
// 		if ( matCount == 1 )
// 		{
// 			if ( m_localToWorldAttribIsActive )
// 			{
// 				glDisableVertexAttribArray(m_localToWorldAttributeLoc);
// 				glDisableVertexAttribArray(m_localToWorldAttributeLoc+1);
// 				glDisableVertexAttribArray(m_localToWorldAttributeLoc+2);
// 				glDisableVertexAttribArray(m_localToWorldAttributeLoc+3);
// 				m_localToWorldAttribIsActive = false;
// 			}
//
// 			glVertexAttrib4f(m_localToWorldAttributeLoc, localToWorld->x.x, localToWorld->x.y, localToWorld->x.z, localToWorld->x.w );
// 			glVertexAttrib4f(m_localToWorldAttributeLoc+1, localToWorld->y.x, localToWorld->y.y, localToWorld->y.z, localToWorld->y.w );
// 			glVertexAttrib4f(m_localToWorldAttributeLoc+2, localToWorld->z.x, localToWorld->z.y, localToWorld->z.z, localToWorld->z.w );
// 			glVertexAttrib4f(m_localToWorldAttributeLoc+3, localToWorld->w.x, localToWorld->w.y, localToWorld->w.z, localToWorld->w.w );
// 		}
// 		else
// 		{
// 			if ( !m_localToWorldAttribIsActive )
// 			{
// 				glEnableVertexAttribArray(m_localToWorldAttributeLoc);
// 				glEnableVertexAttribArray(m_localToWorldAttributeLoc+1);
// 				glEnableVertexAttribArray(m_localToWorldAttributeLoc+2);
// 				glEnableVertexAttribArray(m_localToWorldAttributeLoc+3);
// 				glVertexAttribDivisor(m_localToWorldAttributeLoc, 1);
// 				glVertexAttribDivisor(m_localToWorldAttributeLoc+1, 1);
// 				glVertexAttribDivisor(m_localToWorldAttributeLoc+2, 1);
// 				glVertexAttribDivisor(m_localToWorldAttributeLoc+3, 1);
// 				m_localToWorldAttribIsActive = true;
// 			}
//
// 			static GLuint g_vbo = 0xffffffff;
// 			static GLuint g_vboSize = 0;
// 			GLuint size = sizeof(vsMatrix4x4) * matCount;
// 			// this could be a lot smarter.
// 			if ( g_vbo == 0xffffffff )
// 			{
// 				glGenBuffers(1, &g_vbo);
// 			}
// 			glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
// 			if ( size > g_vboSize )
// 			{
// 				glBufferData(GL_ARRAY_BUFFER, size, localToWorld, GL_STREAM_DRAW);
// 				g_vboSize = size;
// 			}
// 			else
// 			{
// 				void *ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
// 				if ( ptr )
// 				{
// 					memcpy(ptr, localToWorld, size);
// 					glUnmapBuffer(GL_ARRAY_BUFFER);
// 				}
// 			}
// 			vsAssert( sizeof(vsMatrix4x4) == 64, "Whaa?" );
// 			glVertexAttribPointer(m_localToWorldAttributeLoc, 4, GL_FLOAT, 0, 64, 0);
// 			glVertexAttribPointer(m_localToWorldAttributeLoc+1, 4, GL_FLOAT, 0, 64, (void*)16);
// 			glVertexAttribPointer(m_localToWorldAttributeLoc+2, 4, GL_FLOAT, 0, 64, (void*)32);
// 			glVertexAttribPointer(m_localToWorldAttributeLoc+3, 4, GL_FLOAT, 0, 64, (void*)48);
// #ifdef VS_PRISTINE_BINDINGS
// 			glBindBuffer(GL_ARRAY_BUFFER, 0);
// #endif // VS_PRISTINE_BINDINGS
// 		}
// 	}
}

void
vsShader::SetWorldToView( const vsMatrix4x4& worldToView )
{
	m_current->SetWorldToView(worldToView);
	// if ( m_worldToViewLoc >= 0 )
	// {
	// 	glUniformMatrix4fv( m_worldToViewLoc, 1, false, (GLfloat*)&worldToView );
	// }
	// // assume no scaling.
	// if ( m_cameraPositionLoc >= 0 )
	// {
	// 	vsVector3D t = worldToView.Inverse().w;
	// 	glUniform3fv(m_cameraPositionLoc, 1, (GLfloat*)&t);
	// }
	// if ( m_cameraDirectionLoc >= 0 )
	// {
	// 	vsVector3D dir = worldToView.Inverse().z;
	// 	glUniform3fv(m_cameraDirectionLoc, 1, (GLfloat*)&dir);
	// }
}

void
vsShader::SetViewToProjection( const vsMatrix4x4& projection )
{
	m_current->SetViewToProjection(projection);
	// if ( m_viewToProjectionLoc >= 0 )
	// {
	// 	glUniformMatrix4fv( m_viewToProjectionLoc, 1, false, (GLfloat*)&projection );
	// }
}

int32_t
vsShader::GetUniformId(const vsString& name) const
{
	return m_current->GetUniformId(name);
}

const vsShader::Uniform *
vsShader::GetUniform(int i) const
{
	return m_current->GetUniform(i);
}

int32_t
vsShader::GetUniformCount() const
{
	return m_current->GetUniformCount();
}

int32_t
vsShader::GetAttributeCount() const
{
	return m_current->GetAttributeCount();
}


void
vsShader::SetLight( int id, const vsColor& ambient, const vsColor& diffuse,
			const vsColor& specular, const vsVector3D& position,
			const vsVector3D& halfVector )
{
	return m_current->SetLight(id, ambient, diffuse, specular, position, halfVector);
	// if ( id != 0 )
	// 	return;
	// if ( m_lightAmbientLoc >= 0 )
	// {
	// 	glUniform4fv( m_lightAmbientLoc, 1, (GLfloat*)&ambient );
	// }
	// if ( m_lightDiffuseLoc >= 0 )
	// {
	// 	glUniform4fv( m_lightDiffuseLoc, 1, (GLfloat*)&diffuse );
	// }
	// if ( m_lightSpecularLoc >= 0 )
	// {
	// 	glUniform4fv( m_lightSpecularLoc, 1, (GLfloat*)&specular );
	// }
	// if ( m_lightPositionLoc >= 0 )
	// {
	// 	glUniform3fv( m_lightPositionLoc, 1, (GLfloat*)&position );
	// }
	// if ( m_lightHalfVectorLoc >= 0 )
	// {
	// 	glUniform3fv( m_lightHalfVectorLoc, 1, (GLfloat*)&halfVector );
	// }
}

void
vsShader::Prepare( vsMaterial *material, vsShaderValues *values, vsRenderTarget *target )
{
	m_current->Prepare(material, values, target);
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
					GL_CHECK("Test");
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
						GL_CHECK("Test");
						vsAssert( valueNow != -1.f, "-1??" );
						vsAssert( valueNow == m_uniform[i].f32, "Caching system is broken?" );
					}
					break;
				}
		}
	}
	*/
}

// void
// vsShader::SetUniformValueF( int i, float value )
// {
// 	if ( value != m_uniform[i].f32 )
// 	{
// 		glUniform1f( m_uniform[i].loc, value );
// 		m_uniform[i].f32 = value;
// 	}
// }
//
// void
// vsShader::SetUniformValueB( int i, bool value )
// {
// 	if ( value != m_uniform[i].b )
// 	{
// 		glUniform1i( m_uniform[i].loc, value );
// 		m_uniform[i].b = value;
// 	}
// }
//
// void
// vsShader::SetUniformValueI( int i, int value )
// {
// 	// for ( int j = 0; j < m_uniform[i].arraySize; j++ )
// 	// {
// 		glUniform1i( m_uniform[i].loc, value );
// 	// }
// }
//
// void
// vsShader::SetUniformValueVec2( int i, const vsVector2D& value )
// {
// 	glUniform2f( m_uniform[i].loc, value.x, value.y );
// 	m_uniform[i].vec4.x = value.x;
// 	m_uniform[i].vec4.y = value.y;
// }
//
// void
// vsShader::SetUniformValueVec3( int i, const vsVector3D& value )
// {
// 	glUniform3f( m_uniform[i].loc, value.x, value.y, value.z );
// 	m_uniform[i].vec4 = value;
// }
//
//
// void
// vsShader::SetUniformValueVec3( int i, const vsColor& value )
// {
// 	glUniform3f( m_uniform[i].loc, value.r, value.g, value.b );
// 	m_uniform[i].vec4.Set(value.r, value.g, value.b, 0.f);
// }
//
// void
// vsShader::SetUniformValueVec4( int i, const vsVector4D& value )
// {
// 	glUniform4f( m_uniform[i].loc, value.x, value.y, value.z, value.w );
// 	m_uniform[i].vec4 = value;
// }
//
// void
// vsShader::SetUniformValueVec4( int i, const vsColor& value )
// {
// 	glUniform4f( m_uniform[i].loc, value.r, value.g, value.b, value.a );
// 	m_uniform[i].vec4.Set(value.r, value.g, value.b, value.a);
// }
//
// void
// vsShader::SetUniformValueMat4( int i, const vsMatrix4x4& value )
// {
// 	glUniformMatrix4fv( m_uniform[i].loc, 1, GL_FALSE, (const GLfloat*)&value );
// }
//
