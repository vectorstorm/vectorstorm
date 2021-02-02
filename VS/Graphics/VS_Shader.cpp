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
#include "VS_ShaderUniformRegistry.h"
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
	vsArray<vsShaderAutoBitDefinition> g_shaderAutoBitDefinitions;
	vsArray<uint32_t> g_shaderPreCompileBitPatterns;
// }

void
vsShader::SetShaderVariantDefinitions( const vsArray<vsShaderVariantDefinition>& definitions )
{
	g_shaderVariantDefinitions = definitions;
}

void
vsShader::SetAutoBits( const vsArray<vsShaderAutoBitDefinition>& definitions )
{
	g_shaderAutoBitDefinitions = definitions;

	for ( int i = 0; i < g_shaderAutoBitDefinitions.ItemCount(); i++ )
	{
		g_shaderAutoBitDefinitions[i].uniformUID = vsShaderUniformRegistry::UID(
				g_shaderAutoBitDefinitions[i].uniformName );
	}
}

void
vsShader::SetPreCompileBitPatterns( const vsArray<uint32_t>& patterns )
{
	g_shaderPreCompileBitPatterns = patterns;
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
	m_vertexShaderText(vertexShader),
	m_fragmentShaderText(fragmentShader),
	m_variantBitsSupported(0L),
	m_current(NULL),
	m_system(false),
	m_litBool(lit),
	m_textureBool(texture)
{
	FigureOutAvailableVariants( vertexShader );
	FigureOutAvailableVariants( fragmentShader );

	// GL_CHECK_SCOPED("Shader");
	m_current = NULL;//new vsShaderVariant( vertexShader, fragmentShader, lit, texture, variantBits, vFilename, fFilename );
	// Compile( vertexShader, fragmentShader, lit, texture, m_variantBits );
	// m_current->Compile( vertexShader, fragmentShader, lit, texture, variantBits );
	// m_variant.AddItem(m_current);

	// make us compile up all requested bit patterns.
	// for ( int i = 0; i < g_shaderPreCompileBitPatterns.ItemCount(); i++ )
	// 	SetForVariantBits( g_shaderPreCompileBitPatterns[i] & m_variantBitsSupported );
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

			for ( int i = 0; i < g_shaderVariantDefinitions.ItemCount(); i++ )
			{
				if ( g_shaderVariantDefinitions[i].name == variant )
				{
					// vsLog("Has Variant: %s", variant);
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
#ifdef VS_OVERLOAD_ALLOCATORS
	// If we're using overloaded allocators and multiple heaps and stuff, our
	// 'system' shaders exist in system memory, not in game memory.  And so we
	// can't reload them while a game is running;  it'd put them in the wrong
	// heap.  So for now, just don't reload system shaders IF we're in a build
	// where we're using VS's custom allocators for detecting memory leaks.
	if ( m_system )
		return;

	// [Note]  If I ever really need to be able to reload system-owned shaders
	// at runtime in one of these builds (right now, this is only
	// default_v/default_f), this can probably be made to work by calling
	// `vsHeap::Push(g_globalHeap);` here at the start of the function, and
	// popping it back off at the bottom.  Requires leak checking afterward!  And
	// probably an interface on vsSystem or something to do that, since we don't
	// know about the global heap from over here right now.

#endif

	if ( !m_vertexShaderFile.empty() && !m_fragmentShaderFile.empty() )
	{
		// update our cached shader text, then tell our variants to rebuild
		// themselves using it.
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
		m_vertexShaderText = vString;
		m_fragmentShaderText = fString;

		delete vStore;
		delete fStore;

		for ( int i = 0; i < m_variant.ItemCount(); i++ )
			m_variant[i]->Reload( m_vertexShaderText, m_fragmentShaderText );
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

uint32_t
vsShader::GetVariantBitsFor( const vsShaderValues *values )
{
	uint32_t result = 0;
	if ( values )
	{
		for ( int i = 0; i < g_shaderAutoBitDefinitions.ItemCount(); i++ )
		{
			const vsShaderAutoBitDefinition& def = g_shaderAutoBitDefinitions[i];
			bool b = false;
			if ( values->UniformB( def.uniformUID, b ) )
			{
				if ( b )
					result |= BIT( def.bitId );
				// else
				// 	result &= ~BIT( def.bitId );
			}
		}
	}
	return result;
}

void
vsShader::SetForVariantBits( uint32_t bits )
{
	vsAssert( (bits & m_variantBitsSupported) == bits, "Client asked for bits we don't support??" );

	if ( m_current && m_current->GetVariantBits() == bits )
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
			vsString vString( m_vertexShaderText );
			vsString fString( m_fragmentShaderText );

			uint32_t variantBits = bits;
			vsShaderVariant *result =
				new vsShaderVariant(vString, fString, m_litBool, m_textureBool,
						variantBits, m_vertexShaderFile, m_fragmentShaderFile);

			m_current = result;
			m_variant.AddItem(m_current);
		}
	}
}

void
vsShader::SetFog( bool fog, const vsColor& color, float density )
{
	m_current->SetFog(fog, color, density);
}

void
vsShader::SetColor( const vsColor& color )
{
	m_current->SetColor(color);
}

void
vsShader::SetInstanceColors( vsRenderBuffer *colors )
{
	m_current->SetInstanceColors(colors);
}

void
vsShader::SetInstanceColors( const vsColor* color, int matCount )
{
	m_current->SetInstanceColors(color, matCount);
}

void
vsShader::SetTextures( vsTexture *texture[MAX_TEXTURE_SLOTS] )
{
	m_current->SetTextures(texture);
}

void
vsShader::SetLocalToWorld( vsRenderBuffer* buffer )
{
	m_current->SetLocalToWorld(buffer);
}

void
vsShader::SetLocalToWorld( const vsMatrix4x4* localToWorld, int matCount )
{
	m_current->SetLocalToWorld(localToWorld,matCount);
}

void
vsShader::SetWorldToView( const vsMatrix4x4& worldToView )
{
	m_current->SetWorldToView(worldToView);
}

void
vsShader::SetViewToProjection( const vsMatrix4x4& projection )
{
	m_current->SetViewToProjection(projection);
}

void
vsShader::SetViewport( const vsVector2D& dims )
{
	m_current->SetViewport(dims);
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
