#ifdef TEXTURE
uniform sampler2D texture[4];
uniform float alphaRef;
#endif // TEXTURE

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
	vec4 color = gl_Color;
#ifdef TEXTURE
	vec4 textureSample = texture2D(texture[0], gl_TexCoord[0].st);
	if ( textureSample.a < alphaRef )
		discard;
	color *= textureSample;
#endif // TEXTURE

#ifdef LIT
	/* The ambient term will always be present */
	// color *= ambient;
    //
	// vec3 n;
	// vec3 halfV;
	// float NdotL;
	// float NdotHV;
    //
	// /* a fragment shader can't write a varying variable, hence we need
	//    a new variable to store the normalized interpolated normal */
	// n = normalize(normal);
    //
	// /* compute the dot product between normal and ldir */
	// //NdotL = max(dot(n,lightDir),0.0);
	// NdotL = max(dot(n,lightDir)+1.0,0.0) * 0.5;
	// color += gl_Color * NdotL;
    //
	// if (gl_FrontMaterial.shininess > 0.0 && NdotL > 0.0) {
	// 	halfV = normalize(halfVector);
	// 	NdotHV = max(dot(n,halfV),0.0);
	// 	color.rgb += gl_FrontMaterial.specular.rgb *
	// 		gl_LightSource[0].specular.rgb *
	// 		pow(NdotHV, gl_FrontMaterial.shininess);
	// }
#endif // LIT

	gl_FragColor.rgb = mix(gl_Fog.color.rgb, color.rgb, fogFactor );
	gl_FragColor.a = color.a;
}
