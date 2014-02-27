#version 110
uniform bool fog;
uniform mat4 localToWorld;
uniform mat4 worldToView;
uniform mat4 viewToProjection;
varying float fogFactor;

#ifdef LIT
varying vec4 diffuse;
varying vec4 ambient;
varying vec3 normal;
varying vec3 lightDir;
varying vec3 halfVector;
#endif // LIT

void main(void)
{
#ifdef TEXTURE
	gl_TexCoord[0] = gl_MultiTexCoord0;
#endif // TEXTURE

	gl_FrontColor = gl_Color;
	//gl_Position    = ftransform();
	gl_Position = viewToProjection * worldToView * localToWorld * gl_Vertex;
	//gl_Position = gl_Vertex * localToWorld * worldToView * viewToProjection;

#ifdef LIT
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

#endif // LIT

	fogFactor = 1.0;
	if ( fog )
	{
		const float LOG2 = 1.442695;
		vec3 vVertex = vec3(gl_Position);
		float distance = length(vVertex);
		fogFactor = exp2( -gl_Fog.density *
				gl_Fog.density *
				distance *
				distance *
				LOG2 );
		fogFactor = clamp(fogFactor, 0.0, 1.0);
	}
}
