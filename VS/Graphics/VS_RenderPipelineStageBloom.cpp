/*
 *  VS_RenderPipelineStageBloom.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 24/02/2014
 *  Copyright 2014 Trevor Powell.  All rights reserved.
 *
 */

#include "VS_RenderPipelineStageBloom.h"
#include "VS_Camera.h"
#include "VS_DisplayList.h"
#include "VS_DynamicMaterial.h"
#include "VS_RenderTarget.h"
#include "VS_Shader.h"

extern const char *passv, *combine7f, *row3f, *normalf;
#define KERNEL_SIZE   (3)

float kernel[KERNEL_SIZE] = { 4, 5, 4  };

#define BUFFER_HEIGHT (512)
#define BUFFER_WIDTH  (512)

class vsBloomBlurShader: public vsShader
{
	vsVector2D m_offset;
	GLint m_locCoefficients, m_locOffsetX, m_locOffsetY;
public:
	vsBloomBlurShader( const vsVector2D& offset ):
		vsShader(passv, row3f, false, false),
		m_offset(offset)
	{
		m_locCoefficients = glGetUniformLocation(m_shader, "coefficients");
		m_locOffsetX = glGetUniformLocation(m_shader, "offsetx");
		m_locOffsetY = glGetUniformLocation(m_shader, "offsety");
	}

	virtual void Prepare()
	{
		glUniform1f(m_locOffsetX, m_offset.x);
		glUniform1f(m_locOffsetY, m_offset.y);
		glUniform1fv(m_locCoefficients, KERNEL_SIZE, kernel);
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

vsRenderPipelineStageBloom::vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, vsRenderTarget *pass[BLOOM_PASSES], vsRenderTarget *pass2[BLOOM_PASSES] ):
	m_from(from),
	m_to(to)
{
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		m_pass[i] = pass[i];
		m_pass2[i] = pass2[i];
	}

	// Normalize kernel coefficients
	float sum = 0;
	for (int c = 0; c < KERNEL_SIZE; c++)
		sum += kernel[c];
	for (int c = 0; c < KERNEL_SIZE; c++)
		kernel[c] *= (1.f / sum);

	m_hipassMaterial = new vsDynamicMaterial;
	m_hipassMaterial->SetDrawMode(DrawMode_Absolute);
	m_hipassMaterial->SetColor(c_white);
	m_hipassMaterial->SetCullingType(Cull_None);
	m_hipassMaterial->SetZRead(false);
	m_hipassMaterial->SetZWrite(false);
	m_hipassMaterial->SetGlow(false);
	m_hipassMaterial->SetTexture(0, m_from->GetTexture());
	m_hipassMaterial->SetShader(new vsShader(passv, normalf, false, false));

	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		float offset = 1.2f / m_pass[i]->GetWidth();
		m_horizontalBlurMaterial[i] = new vsDynamicMaterial;
		m_horizontalBlurMaterial[i]->SetDrawMode(DrawMode_Absolute);
		m_horizontalBlurMaterial[i]->SetShader(new vsBloomBlurShader(vsVector2D(offset, 0.f)));
		m_horizontalBlurMaterial[i]->SetTexture(0,m_pass[i]->GetTexture());

		m_verticalBlurMaterial[i] = new vsDynamicMaterial;
		m_verticalBlurMaterial[i]->SetDrawMode(DrawMode_Absolute);
		m_verticalBlurMaterial[i]->SetShader(new vsBloomBlurShader(vsVector2D(0.f,offset)));
		m_verticalBlurMaterial[i]->SetTexture(0,m_pass2[i]->GetTexture());
	}

	m_combineMaterial = new vsDynamicMaterial;
	m_combineMaterial->SetShader(new vsBloomCombineShader);
	m_combineMaterial->SetTexture(0, m_from->GetTexture());
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		m_combineMaterial->SetTexture(1+i, m_pass[i]->GetTexture());
	}

	m_white = new vsMaterial("White");

	m_straight = new vsDynamicMaterial();
	m_straight->SetDrawMode(DrawMode_Absolute);
	m_straight->SetColor(c_white);
	m_straight->SetCullingType(Cull_None);
	m_straight->SetZRead(false);
	m_straight->SetZWrite(false);
	m_straight->SetGlow(false);
	m_straight->SetTexture(0, m_from->GetTexture());
}

vsRenderPipelineStageBloom::~vsRenderPipelineStageBloom()
{
	vsDelete( m_hipassMaterial );
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		vsDelete( m_horizontalBlurMaterial[i] );
		vsDelete( m_verticalBlurMaterial[i] );
	}
	vsDelete( m_combineMaterial );
	vsDelete( m_straight );
	vsDelete( m_white );
}

void
vsRenderPipelineStageBloom::Draw( vsDisplayList *list )
{
	// first thing is that I need to hipass 'from' into the top 'pass'.
	vsCamera2D cam;
	cam.SetFieldOfView(2.f);

	float ar = cam.GetAspectRatio();
	vsVector3D v[4] = {
		vsVector3D(-ar,-1.f,0.f),
		vsVector3D(-ar,1.f,0.f),
		vsVector3D(ar,-1.f,0.f),
		vsVector3D(ar,1.f,0.f)
	};
	vsVector2D t[4] = {
		vsVector2D(0.f,1.f),
		vsVector2D(0.f,0.f),
		vsVector2D(1.f,1.f),
		vsVector2D(1.f,0.f)
	};
	int ind[4] = { 0, 1, 2, 3 };

	list->SetMaterial(m_hipassMaterial);
	list->SetProjectionMatrix4x4(cam.GetProjectionMatrix());
	list->SetWorldToViewMatrix4x4(vsMatrix4x4::Identity);
	list->SetMatrix4x4(vsMatrix4x4::Identity);
	list->ResolveRenderTarget(m_from);
	list->SetRenderTarget(m_pass[0]);
	list->VertexArray(v,4);
	list->TexelArray(t,4);
	list->TriangleStrip(ind,4);

	// next, blit passes into each other.
	//
	for ( int i = 0; i < BLOOM_PASSES-1; i++ )
	{
		list->BlitRenderTarget(m_pass[i], m_pass[i+1]);
	}

	// next, blur each pass
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		list->SetRenderTarget(m_pass2[i]);
		list->ResolveRenderTarget(m_pass[i]);
		list->SetMaterial(m_horizontalBlurMaterial[i]);
		list->TriangleStrip(ind,4);

		list->SetRenderTarget(m_pass[i]);
		list->ResolveRenderTarget(m_pass2[i]);
		list->SetMaterial(m_verticalBlurMaterial[i]);
		list->TriangleStrip(ind,4);
	}

	// resolve all primary passes.
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		list->ResolveRenderTarget(m_pass[i]);
		list->ResolveRenderTarget(m_pass2[i]);
	}
	// 	// Now do the final combining of our stuff
	list->SetRenderTarget(m_to);
	list->SetMaterial(m_combineMaterial);
	list->TriangleStrip(ind,4);
	list->ClearArrays();
}


#define STRINGIFY(A)  #A


const char *passv = STRINGIFY(
		uniform mat4 localToWorld;
		uniform mat4 worldToView;
		uniform mat4 viewToProjection;
		void main(void)
		{
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_Position = viewToProjection * worldToView * localToWorld * gl_Vertex;
		}
		);

	const char *combine7f = STRINGIFY(
			uniform sampler2D texture[8];

			void main(void)
			{
				vec4 c = texture2D(texture[0], gl_TexCoord[0].st);
				vec4 glow;
				for ( int i = 1; i < 7; i++ )
				{
					glow += texture2D(texture[i], gl_TexCoord[0].st);
				}
				gl_FragColor = c + 0.6 * glow;
			}
			);

	const char *row3f = STRINGIFY(
			uniform sampler2D texture[8];
			uniform float coefficients[3];
			uniform float offsetx;
			uniform float offsety;

			void main(void)
			{
			vec4 c;
			vec2 tc = gl_TexCoord[0].st;
			vec2 offset = vec2(offsetx, offsety);

			c = coefficients[0] * texture2D(texture[0], tc - offset);
			c += coefficients[1] * texture2D(texture[0], tc);
			c += coefficients[2] * texture2D(texture[0], tc + offset);

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


