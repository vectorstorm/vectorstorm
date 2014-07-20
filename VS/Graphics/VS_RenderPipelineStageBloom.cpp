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
#include "VS_RenderPipeline.h"
#include "VS_RenderTarget.h"
#include "VS_Shader.h"

extern const char *passv, *passf, *combinef, *row3f, *normalf;
#define KERNEL_SIZE   (3)
static float kernel[KERNEL_SIZE] = { 4, 5, 4  };
static bool kernel_normalised = false;

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

		vsShader::Prepare();
	}
};

class vsBloomCombineShader: public vsShader
{
public:
	vsBloomCombineShader():
		vsShader(passv, combinef, false, false)
	{
	}
};

class vsBloomPassShader: public vsShader
{
public:
	vsBloomPassShader():
		vsShader(passv, passf, false, false)
	{
	}
};

vsRenderPipelineStageBloom::vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, int dims ):
	m_dims(dims),
	m_hipassMaterial(NULL),
	m_fromMaterial(NULL),
	m_from(from),
	m_to(to)
{
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		m_pass[i] = NULL;
		m_pass2[i] = NULL;
		m_horizontalBlurMaterial[i] = NULL;
		m_verticalBlurMaterial[i] = NULL;
		m_combinePassMaterial[i] = NULL;
	}
}

vsRenderPipelineStageBloom::~vsRenderPipelineStageBloom()
{
	vsDelete( m_hipassMaterial );
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		vsDelete( m_horizontalBlurMaterial[i] );
		vsDelete( m_verticalBlurMaterial[i] );
		vsDelete( m_combinePassMaterial[i] );
	}
	vsDelete( m_fromMaterial );
}

void
vsRenderPipelineStageBloom::PreparePipeline( vsRenderPipeline *pipeline )
{
	RenderTargetRequest req;
	req.type = RenderTargetRequest::Type_AbsoluteSize;
	req.depth = false;
	req.stencil = false;
	req.linear = true;
	req.mipmaps = false;
	req.antialias = false;
	req.share = true;

	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		req.width = m_dims >> i;
		req.height = m_dims >> i;
		m_pass[i] = pipeline->RequestRenderTarget(req, this);
		m_pass2[i] = pipeline->RequestRenderTarget(req, this);
	}

	if ( !kernel_normalised )
	{
		// Normalize kernel coefficients
		float sum = 0;
		for (int c = 0; c < KERNEL_SIZE; c++)
			sum += kernel[c];
		for (int c = 0; c < KERNEL_SIZE; c++)
			kernel[c] *= (1.f / sum);
		kernel_normalised = true;
	}

	m_hipassMaterial = new vsDynamicMaterial;
	m_hipassMaterial->SetBlend(false);
	m_hipassMaterial->SetDrawMode(DrawMode_Absolute);
	m_hipassMaterial->SetColor(c_white);
	m_hipassMaterial->SetCullingType(Cull_None);
	m_hipassMaterial->SetZRead(false);
	m_hipassMaterial->SetZWrite(false);
	m_hipassMaterial->SetGlow(false);
	m_hipassMaterial->SetTexture(0, m_from->GetTexture(1));
	m_hipassMaterial->SetShader(new vsShader(passv, normalf, false, false));

	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		float offset = 1.2f / m_pass[i]->GetWidth();
		m_horizontalBlurMaterial[i] = new vsDynamicMaterial;
		m_horizontalBlurMaterial[i]->SetClampU(true);
		m_horizontalBlurMaterial[i]->SetClampV(true);
		m_horizontalBlurMaterial[i]->SetBlend(false);
		m_horizontalBlurMaterial[i]->SetShader(new vsBloomBlurShader(vsVector2D(offset, 0.f)));
		m_horizontalBlurMaterial[i]->SetTexture(0,m_pass[i]->GetTexture());

		m_verticalBlurMaterial[i] = new vsDynamicMaterial;
		m_verticalBlurMaterial[i]->SetClampU(true);
		m_verticalBlurMaterial[i]->SetClampV(true);
		m_verticalBlurMaterial[i]->SetBlend(false);
		m_verticalBlurMaterial[i]->SetShader(new vsBloomBlurShader(vsVector2D(0.f,offset)));
		m_verticalBlurMaterial[i]->SetTexture(0,m_pass2[i]->GetTexture());

		m_combinePassMaterial[i] = new vsDynamicMaterial;
		m_combinePassMaterial[i]->SetBlend(true);
		m_combinePassMaterial[i]->SetColor(c_white);
		m_combinePassMaterial[i]->SetCullingType(Cull_None);
		m_combinePassMaterial[i]->SetZRead(false);
		m_combinePassMaterial[i]->SetZWrite(false);
		m_combinePassMaterial[i]->SetDrawMode(DrawMode_Add);
		m_combinePassMaterial[i]->SetGlow(false);
		m_combinePassMaterial[i]->SetClampU(true);
		m_combinePassMaterial[i]->SetClampV(true);
		m_combinePassMaterial[i]->SetShader(new vsBloomCombineShader);
		m_combinePassMaterial[i]->SetTexture(0, m_pass[i]->GetTexture());
	}

	m_fromMaterial = new vsDynamicMaterial;
	m_fromMaterial->SetBlend(false);
	m_fromMaterial->SetColor(c_white);
	m_fromMaterial->SetCullingType(Cull_None);
	m_fromMaterial->SetZRead(false);
	m_fromMaterial->SetZWrite(false);
	m_fromMaterial->SetGlow(false);
	m_fromMaterial->SetClampU(true);
	m_fromMaterial->SetClampV(true);
	m_fromMaterial->SetShader(new vsBloomPassShader);
	m_fromMaterial->SetTexture(0, m_from->GetTexture());
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

	list->ClearArrays();
	list->SetProjectionMatrix4x4(cam.GetProjectionMatrix());
	list->ResolveRenderTarget(m_from);
	list->SetMaterial(m_hipassMaterial);
	list->SetRenderTarget(m_pass[0]);
	list->VertexArray(v,4);
	list->TexelArray(t,4);
	list->TriangleStripArray(ind,4);

	// list->BlitRenderTarget( m_pass[0], m_to );
	// return;

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
		list->TriangleStripArray(ind,4);

		list->SetRenderTarget(m_pass[i]);
		list->ResolveRenderTarget(m_pass2[i]);
		list->SetMaterial(m_verticalBlurMaterial[i]);
		list->TriangleStripArray(ind,4);
	}
    //
	// resolve all primary passes.
	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		list->ResolveRenderTarget(m_pass[i]);
		// list->ResolveRenderTarget(m_pass2[i]);
	}

	// Now do the final combining of our stuff
	list->SetRenderTarget(m_to);
	list->SetMaterial(m_fromMaterial);
	list->TriangleStripArray(ind,4);

	for ( int i = 0; i < BLOOM_PASSES; i++ )
	{
		list->SetMaterial(m_combinePassMaterial[i]);
		list->TriangleStripArray(ind,4);
	}
	list->ClearArrays();
}


#define STRINGIFY(A)  #A


const char *passv = STRINGIFY( #version 330\n
		uniform mat4 viewToProjection;\n
		in vec4 vertex;\n
		in vec2 texcoord;\n
		out vec2 fragment_texcoord;\n
		void main(void)\n
		{\n
			fragment_texcoord = texcoord;\n
			gl_Position = viewToProjection * vertex;\n
		}\n
		);

	const char *passf = STRINGIFY( #version 330\n
			uniform sampler2D textures[8];
			in vec2 fragment_texcoord;
			out vec4 fragment_color;

			void main(void)
			{
				fragment_color = texture(textures[0], fragment_texcoord);
			}
			);

	const char *combinef = STRINGIFY( #version 330\n
			uniform sampler2D textures[8];
			in vec2 fragment_texcoord;
			out vec4 fragment_color;

			void main(void)
			{
				fragment_color = vec4(texture(textures[0], fragment_texcoord).rgb, 1.0);
			}
			);

	const char *row3f = STRINGIFY( #version 330\n
			uniform sampler2D textures[8];
			uniform float coefficients[3];
			uniform float offsetx;
			uniform float offsety;
			in vec2 fragment_texcoord;
			out vec4 fragment_color;

			void main(void)
			{
			vec4 c;
			vec2 tc = fragment_texcoord;
			vec2 offset = vec2(offsetx, offsety);

			c = coefficients[0] * texture(textures[0], tc - offset);
			c += coefficients[1] * texture(textures[0], tc);
			c += coefficients[2] * texture(textures[0], tc + offset);

			fragment_color = c;
			}
			);

	const char *normalf = STRINGIFY( #version 330\n
			uniform sampler2D textures[8];\n
			in vec2 fragment_texcoord;\n
			out vec4 fragment_color;\n

			void main(void)\n
			{\n
			vec4 color = texture(textures[0], fragment_texcoord);\n

			float bloom = color.a;\n

			color.xyz *= bloom;\n
			color.a = 1.0;\n

			fragment_color = color;\n
			}\n
			);


