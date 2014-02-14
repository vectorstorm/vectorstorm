/*
 *  VS_Renderer_OpenGL2_DefaultShaders.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 14/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_Renderer_OpenGL2_DefaultShaders.h"

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

