/*
 *  VS_RenderPipelineStageBloom.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 24/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RenderPipelineStageBloom.h"
#include "VS_DisplayList.h"
#include "VS_DynamicMaterial.h"
#include "VS_RenderTarget.h"
#include "VS_Shader.h"

extern const char *passv, *combine7f, *row3f, *normalf;
#define KERNEL_SIZE   (3)

float kernel[KERNEL_SIZE] = { 4, 5, 4  };

#define BUFFER_HEIGHT (512)
#define BUFFER_WIDTH  (512)

class vsBloomFilterShader: public vsShader
{
	GLint m_locSource, m_locCoefficients, m_locOffsetX, m_locOffsetY;
public:
	vsBloomFilterShader():
		vsShader(passv, row3f, false, false)
	{
		m_locSource = glGetUniformLocation(m_shader, "source");
		m_locCoefficients = glGetUniformLocation(m_shader, "coefficients");
		m_locOffsetX = glGetUniformLocation(m_shader, "offsetx");
		m_locOffsetY = glGetUniformLocation(m_shader, "offsety");

	}
};

class vsBloomCombineShader: public vsShader
{
	GLint m_sceneLoc;
	GLint m_passInt[BLOOM_PASSES];
public:
	vsBloomCombineShader():
		vsShader(passv, combine7f, false, false)
	{
		m_sceneLoc = glGetUniformLocation(m_shader, "Scene");
		for ( int i = 0; i < BLOOM_PASSES; i++ )
		{
			char name[] = "Pass##";
			sprintf(name, "Pass%d", i);
			m_passInt[i] = glGetUniformLocation(m_shader, name);
		}
	}
};

vsRenderPipelineStageBloom::vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, vsRenderTarget *pass[BLOOM_PASSES] ):
	m_from(from),
	m_to(to)
{
	for ( int i = 0; i < BLOOM_PASSES; i++ )
		m_pass[i] = pass[i];

	// Normalize kernel coefficients
    float sum = 0;
    for (int c = 0; c < KERNEL_SIZE; c++)
        sum += kernel[c];
    for (int c = 0; c < KERNEL_SIZE; c++)
        kernel[c] *= (1.f / sum);

	m_combine = new vsBloomCombineShader();
	m_filter = new vsBloomFilterShader();
	m_hipass = new vsShader( passv, normalf, false, false );
}

vsRenderPipelineStageBloom::~vsRenderPipelineStageBloom()
{
	vsDelete( m_combine );
	vsDelete( m_filter );
	vsDelete( m_hipass );
}

void
vsRenderPipelineStageBloom::Draw( vsDisplayList *list )
{
	// first thing is that I need to hipass 'from' into the top 'pass'.

	list->ResolveRenderTarget(m_from);
	list->SetRenderTarget(m_pass[0]);
	list->SetMaterial( m_hipassMaterial );
	list->DrawFullScreen();
	// next, blit passes into each other.
	for ( int i = 0; i < BLOOM_PASSES-1; i++ )
	{
		list->BlitRenderTarget(m_pass[i], m_pass[i+1]);
	}
	// next, blur each pass
	//   BLUR GOES HERE
	//

	// Now do the final combining of our stuff
	list->SetRenderTarget(m_to);
	list->SetMaterial(m_combineMaterial);
	list->DrawFullScreen();
}


#define STRINGIFY(A)  #A


const char *passv = STRINGIFY(
							  void main(void)
							  {
							  gl_TexCoord[0] = gl_MultiTexCoord0;
							  gl_Position    = ftransform();
							  }
);

const char *combine7f = STRINGIFY(
								  uniform sampler2D Pass0;
								  uniform sampler2D Pass1;
								  uniform sampler2D Pass2;
								  uniform sampler2D Pass3;
								  uniform sampler2D Pass4;
								  uniform sampler2D Pass5;
								  uniform sampler2D Scene;

								  void main(void)
								  {
								  vec4 t0 = texture2D(Pass0, gl_TexCoord[0].st);
								  vec4 t1 = texture2D(Pass1, gl_TexCoord[0].st);
								  vec4 t2 = texture2D(Pass2, gl_TexCoord[0].st);
								  vec4 t3 = texture2D(Pass3, gl_TexCoord[0].st);
								  vec4 t4 = texture2D(Pass4, gl_TexCoord[0].st);
								  vec4 t5 = texture2D(Pass5, gl_TexCoord[0].st);
								  vec4 t6 = texture2D(Scene, gl_TexCoord[0].st);
								  gl_FragColor = 0.6 * (t0 + t1 + t2 + t3 + t4 + t5) + t6;
								  }
);

const char *row3f = STRINGIFY(
							  uniform sampler2D source;
							  uniform float coefficients[3];
							  uniform float offsetx;
							  uniform float offsety;

							  void main(void)
							  {
							  vec4 c;
							  vec2 tc = gl_TexCoord[0].st;
							  vec2 offset = vec2(offsetx, offsety);

							  c = coefficients[0] * texture2D(source, tc - offset);
							  c += coefficients[1] * texture2D(source, tc);
							  c += coefficients[2] * texture2D(source, tc + offset);

							  gl_FragColor = c;
							  }
);

const char *normalf = STRINGIFY(
								uniform sampler2D source;

								void main(void)
								{
								vec4 color = texture2D(source, gl_TexCoord[0].st);

								float bloom = color.a;

								color.xyz *= bloom;
								color.a = 1.0;

								gl_FragColor = color;
								}
);


