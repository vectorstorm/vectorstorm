/*
 *  VS_RendererShader.cpp
 *  MMORPG2
 *
 *  Created by Trevor Powell on 6/05/09.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#if !TARGET_OS_IPHONE

#include "VS_RendererShader.h"

#include "VS_MaterialInternal.h"

#include "VS_Shader.h"
#include "VS_ShaderSuite.h"

GLuint			vsRendererShader::s_normalProg = -1;
GLuint			vsRendererShader::s_litProg = -1;
GLuint			vsRendererShader::s_normalTexProg = -1;
GLuint			vsRendererShader::s_litTexProg = -1;

bool				vsRendererShader::s_shadersBuilt = false;


#define STRINGIFY(A)  #A

static const char *normalv = STRINGIFY(
									   varying float fogFactor;
									   void main(void)
									   {
										   gl_FrontColor = gl_Color;
										   gl_Position    = ftransform();

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
									   );

static const char *texv = STRINGIFY(
									varying float fogFactor;
									void main(void)
									{
										gl_TexCoord[0] = gl_MultiTexCoord0;
										gl_FrontColor = gl_Color;
										gl_Position    = ftransform();

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
									);

static const char *litv = STRINGIFY(
									varying vec4 diffuse;
									varying vec4 ambient;
									varying vec3 normal;
									varying vec3 lightDir;
									varying vec3 halfVector;
									varying float fogFactor;

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
										gl_FragColor.a = color.a;
									}
);

static GLint s_litTexfAlphaRef;
static GLint s_texfAlphaRef;


vsRendererShader::vsRendererShader()
{

}

vsRendererShader::~vsRendererShader()
{
}

void
vsRendererShader::InitPhaseTwo(int width, int height, int depth, bool fullscreen)
{
	Parent::InitPhaseTwo(width, height, depth, fullscreen);

	s_normalProg = Compile(normalv, normalf);
	s_litProg = Compile(litv, litf);
	s_normalTexProg = Compile(texv, texf);
	s_litTexProg = Compile(litv, litTexf);
	s_shadersBuilt = true;

	s_litTexfAlphaRef = glGetUniformLocation(s_litTexProg, "alphaRef");
	s_texfAlphaRef = glGetUniformLocation(s_normalTexProg, "alphaRef");
}

void
vsRendererShader::Deinit()
{
	Parent::Deinit();
}

void
vsRendererShader::SetMaterial( vsMaterialInternal *material )
{
	if ( material == m_currentMaterial )
	{
		return;
	}

	Parent::SetMaterial( material );

	if ( m_currentSettings.polygonOffsetUnits == 0.f )
	{
		m_state.SetBool( vsRendererState::Bool_PolygonOffsetFill, false );
	}
	else
	{
		m_state.SetBool( vsRendererState::Bool_PolygonOffsetFill, true );
		m_state.SetFloat( vsRendererState::Float_PolygonOffsetUnits, m_currentSettings.polygonOffsetUnits );
	}
    if ( m_currentSettings.writeColor )
    {
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,material->m_glow);
    }
    else
    {
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    }
    if ( m_currentSettings.writeDepth )
    {
        m_state.SetBool( vsRendererState::Bool_DepthMask, material->m_zWrite );
    }
    else
    {
        m_state.SetBool( vsRendererState::Bool_DepthMask, false );
    }
    m_currentShader = NULL;

	switch ( material->m_stencil )
	{
		case vsMaterialInternal::StencilOp_None:
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			break;
		case vsMaterialInternal::StencilOp_One:
			glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			break;
		case vsMaterialInternal::StencilOp_Zero:
			glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
			break;
		case vsMaterialInternal::StencilOp_Inc:
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
			break;
		case vsMaterialInternal::StencilOp_Dec:
			glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
			break;
		case vsMaterialInternal::StencilOp_Invert:
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
				if ( material->m_texture )
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::NormalTex) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::NormalTex);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
                        m_currentShader->SetAlphaRef( material->m_alphaRef );
					}
					else
					{
						glUseProgram( s_normalTexProg );
						glUniform1f( s_texfAlphaRef, material->m_alphaRef );
					}
				}
				else
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Normal) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Normal);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
					}
					else
					{
						glUseProgram( s_normalProg );
					}
				}
				break;
			case DrawMode_Lit:

				if ( material->m_texture )
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::LitTex) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::LitTex);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
                        m_currentShader->SetAlphaRef( material->m_alphaRef );
					}
					else
					{
						glUseProgram( s_litTexProg );
						glUniform1f( s_litTexfAlphaRef, material->m_alphaRef );
					}
				}
				else
				{
					if ( m_currentSettings.shaderSuite && m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Lit) )
					{
                        m_currentShader = m_currentSettings.shaderSuite->GetShader(vsShaderSuite::Lit);
						glUseProgram(m_currentShader->GetShaderId());
						m_currentShader->Prepare();
					}
					else
					{
						glUseProgram( s_litProg );
					}
				}
				break;
			default:
				vsAssert(0,"Unknown drawmode??");
		}
	}

    m_state.Flush();
}

bool
vsRendererShader::Supported(bool experimental)
{
	return Parent::Supported(experimental);
}

#endif // !TARGET_OS_IPHONE

