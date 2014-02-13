/*
 *  VS_RenderSchemeShader.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 6/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#if !TARGET_OS_IPHONE

#include "VS_RenderSchemeShader.h"

#include "VS_MaterialInternal.h"

#include "VS_Shader.h"
#include "VS_ShaderSuite.h"

GLuint			vsRenderSchemeShader::s_normalProg = -1;
GLuint			vsRenderSchemeShader::s_litProg = -1;
GLuint			vsRenderSchemeShader::s_normalTexProg = -1;
GLuint			vsRenderSchemeShader::s_litTexProg = -1;

GLuint			vsRenderSchemeShader::s_normalProgFogLoc = -1;
GLuint			vsRenderSchemeShader::s_litProgFogLoc = -1;
GLuint			vsRenderSchemeShader::s_normalTexProgFogLoc = -1;
GLuint			vsRenderSchemeShader::s_litTexProgFogLoc = -1;

bool				vsRenderSchemeShader::s_shadersBuilt = false;


#define STRINGIFY(A)  #A

static const char *normalv = STRINGIFY(
									   uniform bool fog;
									   varying float fogFactor;
									   void main(void)
									   {
										   gl_FrontColor = gl_Color;
										   gl_Position    = ftransform();

										   fogFactor = 1.0;
										   if ( fog )
										   {
										   const float LOG2 = 1.442695;
										   vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
										   float distance = length(vVertex);
										   fogFactor = exp2( -gl_Fog.density *
															gl_Fog.density *
															distance *
															distance *
															LOG2 );
										   fogFactor = clamp(fogFactor, 0.0, 1.0);
										   }
									   }
									   );

static const char *texv = STRINGIFY(
									uniform bool fog;
									varying float fogFactor;
									void main(void)
									{
										gl_TexCoord[0] = gl_MultiTexCoord0;
										gl_FrontColor = gl_Color;
										gl_Position    = ftransform();

										fogFactor = 1.0;
										if ( fog )
										{
										const float LOG2 = 1.442695;
										vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
										float distance = length(vVertex);
										fogFactor = exp2( -gl_Fog.density *
														 gl_Fog.density *
														 distance *
														 distance *
														 LOG2 );
										fogFactor = clamp(fogFactor, 0.0, 1.0);
										}
									}
									);

static const char *litv = STRINGIFY(
									varying vec4 diffuse;
									varying vec4 ambient;
									varying vec3 normal;
									varying vec3 lightDir;
									varying vec3 halfVector;
									varying float fogFactor;
									uniform bool fog;

									void main()
									{
										gl_TexCoord[0] = gl_MultiTexCoord0;
										/* first transform the normal into eye space and
										 normalize the result */
										normal = normalize(gl_NormalMatrix * gl_Normal);

										/* now normalize the light's direction. Note that
										 according to the OpenGL specification, the light
										 is stored in eye space. Also since we're talking about
										 a directional light, the position field is actually direction */
										lightDir = normalize(vec3(gl_LightSource[0].position));

										/* Normalize the halfVector to pass it to the fragment shader */
										halfVector = normalize(gl_LightSource[0].halfVector.xyz);

										/* Compute the diffuse, ambient and globalAmbient terms */
										diffuse = /*gl_FrontMaterial.diffuse*/ gl_Color * gl_LightSource[0].diffuse;
										ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
										ambient += gl_LightModel.ambient * gl_FrontMaterial.ambient;

										gl_Position = ftransform();

										fogFactor = 1.0;
										if ( fog )
										{
										const float LOG2 = 1.442695;
										vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
										float distance = length(vVertex);
										fogFactor = exp2( -gl_Fog.density *
														 gl_Fog.density *
														 distance *
														 distance *
														 LOG2 );
										fogFactor = clamp(fogFactor, 0.0, 1.0);
										}
									}
									);

static const char *litf = STRINGIFY(
									varying vec4 diffuse;
									varying vec4 ambient;
									varying vec3 normal;
									varying vec3 lightDir;
									varying vec3 halfVector;
									varying float fogFactor;

									void main()
									{
										vec3 n;
										vec3 halfV;
										float NdotL;
										float NdotHV;

										/* The ambient term will always be present */
										vec4 color = diffuse * ambient;

										/* a fragment shader can't write a varying variable, hence we need
										 a new variable to store the normalized interpolated normal */
										n = normalize(normal);

										/* compute the dot product between normal and ldir */
										NdotL = max(dot(n,lightDir)+1.0,0.0) * 0.5;
										color += diffuse * NdotL;

										if (NdotL > 0.0) {
											halfV = normalize(halfVector);
											NdotHV = max(dot(n,halfV),0.0);
											color.rgb += gl_FrontMaterial.specular.rgb *
													gl_LightSource[0].specular.rgb *
													pow(NdotHV, gl_FrontMaterial.shininess);
										}

										gl_FragColor.rgb = mix(gl_Fog.color.rgb, color.rgb, fogFactor );
										gl_FragColor.a = color.a;
									}
									);
/*
static const char *litTexv = STRINGIFY(
									   void main(void)
									   {
										   vec3 Normal = gl_NormalMatrix * gl_Normal;

										   vec3 Light = gl_LightSource[0].position.xyz;//normalize();

										   float Diffuse = max(dot(Normal, Light),0.0);

										   gl_TexCoord[0] = gl_MultiTexCoord0;
										   gl_FrontColor = gl_Color;
										   gl_FrontColor.rgb *= Diffuse;
										   gl_Position = ftransform();
									   }
									   );
*/
static const char *litTexf = STRINGIFY(
									   uniform sampler2D source;
									   uniform float alphaRef;

									   varying vec4 diffuse;
									   varying vec4 ambient;
									   varying vec3 normal;
									   varying vec3 lightDir;
									   varying vec3 halfVector;
									   varying float fogFactor;

									   void main()
									   {
										   vec3 n;
										   vec3 halfV;
										   float NdotL;
										   float NdotHV;

										   /* The ambient term will always be present */
										   vec4 color = diffuse * ambient;
										   vec4 textureSample = texture2D(source, gl_TexCoord[0].st);
										   if ( textureSample.a < alphaRef )
											   discard;

										   /* a fragment shader can't write a varying variable, hence we need
											a new variable to store the normalized interpolated normal */
										   n = normalize(normal);

										   /* compute the dot product between normal and ldir */
										   //NdotL = max(dot(n,lightDir),0.0);
										   NdotL = max(dot(n,lightDir)+1.0,0.0) * 0.5;
										   color += diffuse * NdotL;

										   if (gl_FrontMaterial.shininess > 0.0 && NdotL > 0.0) {
											   halfV = normalize(halfVector);
											   NdotHV = max(dot(n,halfV),0.0);
											   color.rgb += gl_FrontMaterial.specular.rgb *
											   gl_LightSource[0].specular.rgb *
											   pow(NdotHV, gl_FrontMaterial.shininess);
										   }

										   gl_FragColor.rgb = mix(gl_Fog.color.rgb, color.rgb * textureSample.rgb, fogFactor );
										   gl_FragColor.a = color.a * textureSample.a;
									   }
);


static const char *normalf = STRINGIFY(
									   varying float fogFactor;
									   void main(void)
									   {
										   gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_Color.rgb, fogFactor );
										   gl_FragColor.a = gl_Color.a;
									   }
);

static const char *texf = STRINGIFY(
									uniform sampler2D source;
									uniform float alphaRef;
									varying float fogFactor;

									void main(void)
									{
										vec4 color = texture2D(source, gl_TexCoord[0].st);
										if ( color.a < alphaRef )
											discard;
										gl_FragColor.rgb = mix(gl_Fog.color.rgb, color.rgb * gl_Color.rgb, fogFactor );
										gl_FragColor.a = color.a * gl_Color.a;
									}
);

static GLint s_litTexfAlphaRef;
static GLint s_texfAlphaRef;


vsRenderSchemeShader::vsRenderSchemeShader( vsRenderer *renderer ):
	vsRenderSchemeBloom(renderer)
{
	s_normalProg = Compile(normalv, normalf);
	s_litProg = Compile(litv, litf);
	s_normalTexProg = Compile(texv, texf);
	s_litTexProg = Compile(litv, litTexf);
	s_shadersBuilt = true;

	s_litTexfAlphaRef = glGetUniformLocation(s_litTexProg, "alphaRef");
	s_texfAlphaRef = glGetUniformLocation(s_normalTexProg, "alphaRef");

	s_normalProgFogLoc = glGetUniformLocation(s_normalProg, "fog");
	s_litProgFogLoc = glGetUniformLocation(s_litProg, "fog");
	s_normalTexProgFogLoc = glGetUniformLocation(s_normalTexProg, "fog");
	s_litTexProgFogLoc = glGetUniformLocation(s_litTexProg, "fog");
}

vsRenderSchemeShader::~vsRenderSchemeShader()
{
}

void
vsRenderSchemeShader::PreRender( const vsRenderer::Settings &s )
{
	vsRenderSchemeBloom::PreRender(s);
	m_currentMaterial = NULL;
}

void
vsRenderSchemeShader::SetMaterial( vsMaterialInternal *material )
{
	if ( material == m_currentMaterial )
	{
		return;
	}
	m_currentMaterial = material;

	//if ( m_renderer->GetCurrentSettings().polygonOffsetUnits == 0.f )
	//{
		//m_renderer->GetState()->SetBool( vsRendererState::Bool_PolygonOffsetFill, false );
	//}
	//else
	//{
		//m_renderer->GetState()->SetBool( vsRendererState::Bool_PolygonOffsetFill, true );
		//m_renderer->GetState()->SetFloat( vsRendererState::Float_PolygonOffsetUnits, m_renderer->GetCurrentSettings().polygonOffsetUnits );
	//}
    if ( m_renderer->GetCurrentSettings().writeColor )
    {
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,material->m_glow);
    }
    else
    {
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    }
    if ( m_renderer->GetCurrentSettings().writeDepth )
    {
        m_renderer->GetState()->SetBool( vsRendererState::Bool_DepthMask, material->m_zWrite );
    }
    else
    {
        m_renderer->GetState()->SetBool( vsRendererState::Bool_DepthMask, false );
    }
    m_currentShader = NULL;

	switch ( material->m_stencil )
	{
		case StencilOp_None:
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			break;
		case StencilOp_One:
			glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			break;
		case StencilOp_Zero:
			glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
			break;
		case StencilOp_Inc:
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			break;
		case StencilOp_Dec:
			glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
			break;
		case StencilOp_Invert:
			glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
			break;
		default:
			vsAssert(0, vsFormatString("Unhandled stencil type: %d", material->m_stencil));
	}

	if ( material->m_shader )
	{
		glUseProgram( material->m_shader->GetShaderId() );
		material->m_shader->Prepare();
        m_currentShader = material->m_shader;
	}
	else
	{
		switch( material->m_drawMode )
		{
			case DrawMode_Add:
			case DrawMode_Subtract:
			case DrawMode_Normal:
			case DrawMode_Absolute:
				if ( material->m_texture[0] )
				{
					if ( m_renderer->GetCurrentSettings().shaderSuite && m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::NormalTex) )
					{
                        m_currentShader = m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::NormalTex);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
                        m_currentShader->SetAlphaRef( material->m_alphaRef );
					}
					else
					{
						glUseProgram( s_normalTexProg );
						glUniform1f( s_normalTexProgFogLoc, material->m_fog );
						glUniform1f( s_texfAlphaRef, material->m_alphaRef );
					}
				}
				else
				{
					if ( m_renderer->GetCurrentSettings().shaderSuite && m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::Normal) )
					{
                        m_currentShader = m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::Normal);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
					}
					else
					{
						glUseProgram( s_normalProg );
						glUniform1f( s_normalProgFogLoc, 0 );
					}
				}
				break;
			case DrawMode_Lit:

				if ( material->m_texture[0] )
				{
					if ( m_renderer->GetCurrentSettings().shaderSuite && m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::LitTex) )
					{
                        m_currentShader = m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::LitTex);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
                        m_currentShader->SetAlphaRef( material->m_alphaRef );
					}
					else
					{
						glUseProgram( s_litTexProg );
						glUniform1f( s_litTexProgFogLoc, material->m_fog );
						glUniform1f( s_litTexfAlphaRef, material->m_alphaRef );
					}
				}
				else
				{
					if ( m_renderer->GetCurrentSettings().shaderSuite && m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::Lit) )
					{
                        m_currentShader = m_renderer->GetCurrentSettings().shaderSuite->GetShader(vsShaderSuite::Lit);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
					}
					else
					{
						glUseProgram( s_litProg );
						glUniform1f( s_litProgFogLoc, 1 );
						//glUniform1f( s_litProgFogLoc, material->m_fog );
					}
				}
				break;
			default:
				vsAssert(0,"Unknown drawmode??");
		}
	}

    m_renderer->GetState()->Flush();
}

bool
vsRenderSchemeShader::Supported(bool experimental)
{
	return Parent::Supported(experimental);
}

#endif // !TARGET_OS_IPHONE

