#version 330
#ifdef TEXTURE
uniform sampler2D textures[8];
in vec2 texcoord_out;
uniform float alphaRef;
#endif // TEXTURE

in float fogFactor;
uniform vec3 fogColor;
uniform float glow;

#ifdef LIT
struct lightSourceParameters
{
	vec4 ambient;              // Aclarri
	vec4 diffuse;              // Dcli
	vec4 specular;             // Scli
	vec3 position;             // Ppli
	vec3 halfVector;           // Derived: Hi
};

uniform lightSourceParameters lightSource[4];
in vec3 fragNormal;
#endif // LIT

in vec4 frontColor;
out vec4 fragColor[2];

void main(void)
{
	vec4 color = frontColor;
#ifdef TEXTURE
	vec4 textureSample = texture(textures[0], texcoord_out.st);
	if ( textureSample.a < alphaRef )
		discard;
	color *= textureSample;
#endif // TEXTURE

#ifdef LIT
	/* The ambient term will always be present */
	vec4 ambientPart = color * lightSource[0].ambient;

	vec3 n;
	// vec3 halfV;
	float NdotL;
	// float NdotHV;

	/* re-normalise, since the normal may have gone out-of-whack during
	   vertex interpolation */
	n = normalize(fragNormal);

	/* compute the dot product between normal and ldir */
	// NdotL = max(dot(n,lightDir),0.0);
	NdotL = max(dot(n,lightSource[0].position)+1.0,0.0) * 0.5;
	vec4 diffusePart = color * lightSource[0].diffuse * NdotL;

	// if (shininess > 0.0 && NdotL > 0.0) {
	// 	halfV = normalize(halfVector);
	// 	NdotHV = max(dot(n,halfV),0.0);
	// 	color.rgb += specular.rgb *
	// 		lightSource[0].specular.rgb *
	// 		pow(NdotHV, shininess);
	// }
	color = ambientPart + diffusePart;
	color.r = 1.0;
#endif // LIT

	vec3 finalColor = mix(fogColor.rgb, color.rgb, fogFactor );
	fragColor[0].rgba = vec4(finalColor, color.a);
	fragColor[1].rgba = vec4(finalColor * glow, 1.0);
	// fragColor = vec4(1.0,0.0,0.0,1.0);
}

