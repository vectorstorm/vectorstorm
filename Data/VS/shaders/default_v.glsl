#version 330
uniform bool fog;
uniform float fogDensity;
// uniform mat4 localToWorld;
in mat4 localToWorldAttrib;
uniform mat4 worldToView;
uniform mat4 viewToProjection;
uniform vec4 universal_color;

out float fogFactor;
in vec4 vertex;
in vec3 normal;
in vec4 color;
in vec2 texcoord;

out vec4 frontColor;

#ifdef TEXTURE
out vec2 texcoord_out;
#endif

#ifdef LIT
// out vec4 diffuse;
// out vec4 ambient;
out vec3 fragNormal;
// out vec3 lightDir;
// out vec3 halfVector;
//
struct lightSourceParameters
{
	vec4 ambient;              // Aclarri
	vec4 diffuse;              // Dcli
	vec4 specular;             // Scli
	vec3 position;             // Ppli
	vec3 halfVector;           // Derived: Hi
};

uniform lightSourceParameters lightSource[4];

#endif // LIT

void main(void)
{
#ifdef TEXTURE
	texcoord_out = texcoord;
#endif // TEXTURE

	frontColor = universal_color * color;
	//gl_Position    = ftransform();
	gl_Position = viewToProjection * worldToView * localToWorldAttrib * vertex;
	// gl_Position = vertex * localToWorld * worldToView * viewToProjection;

#ifdef LIT
	/* first transform the normal into world space and
	   normalize the result */
	fragNormal = normalize((localToWorldAttrib * vec4(normal,0.0)).xyz);

	/* currently hardcoded to directional light, so the position field is
	 * actually direction */
	// lightDir = normalize(vec3(1.0,1.0,1.0));
	// lightDir = normalize(vec3(lightSource[0].position));

	/* Normalize the halfVector to pass it to the fragment shader */
	//halfVector = normalize(lightSource[0].halfVector.xyz);

	/* Compute the diffuse, ambient and globalAmbient terms */
	// diffuse = color * lightSource[0].diffuse;
	// ambient = color * lightSource[0].ambient;
	// ambient = vec4(0.1,0.1,0.1,1.0); //lightSource[0].ambient;
	/* ambient += gl_LightModel.ambient * gl_FrontMaterial.ambient; */

#endif // LIT

	fogFactor = 1.0;
	if ( fog )
	{
		const float LOG2 = 1.442695;
		vec3 vVertex = vec3(gl_Position);
		float distance = length(vVertex);
		fogFactor = exp2( -fogDensity *
				fogDensity *
				distance *
				distance *
				LOG2 );
		fogFactor = clamp(fogFactor, 0.0, 1.0);
	}
}
