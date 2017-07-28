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
#include "VS_RenderBuffer.h"
#include "VS_RenderPipeline.h"
#include "VS_RenderTarget.h"
#include "VS_Shader.h"
#include "VS_Screen.h"

extern const char *passv, *passf, *combinef, *row3v, *row3f, *normalf;
#define KERNEL_SIZE   (3)
static float kernel[KERNEL_SIZE] = { 1, 2, 1  };
static bool kernel_normalised = false;

class vsBloomBlurShader: public vsShader
{
	GLint m_locCoefficients;//, m_locOffsetX, m_locOffsetY;
public:
	vsBloomBlurShader():
		vsShader(row3v, row3f, false, false)
	{
		m_locCoefficients = glGetUniformLocation(m_shader, "coefficients");
		// m_locOffsetX = glGetUniformLocation(m_shader, "offsetx");
		// m_locOffsetY = glGetUniformLocation(m_shader, "offsety");
	}

	virtual void Prepare( vsMaterial *mat, vsShaderValues *values )
	{
		vsShader::Prepare( mat, values );
		// glUniform1f(m_locOffsetX, m_offset.x);
		// glUniform1f(m_locOffsetY, m_offset.y);
		glUniform1fv(m_locCoefficients, KERNEL_SIZE, kernel);
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

vsRenderPipelineStageBloom::vsRenderPipelineStageBloom( vsRenderTarget *from, vsRenderTarget *to, int passes ):
	m_passes(NULL),
	m_passCount(passes),
	m_hipassMaterial(NULL),
	m_fromMaterial(NULL),
	m_from(from),
	m_to(to),
	m_vertices(NULL),
	m_indices(NULL),
	m_bloomBlurShader(NULL)
{
	m_passes = new struct Pass[m_passCount];
	for ( int i = 0; i < m_passCount; i++ )
	{
		m_passes[i].m_pass = NULL;
		m_passes[i].m_pass2 = NULL;
		m_passes[i].m_horizontalBlurMaterial = NULL;
		m_passes[i].m_verticalBlurMaterial = NULL;
		m_passes[i].m_combinePassMaterial = NULL;
	}
}

vsRenderPipelineStageBloom::~vsRenderPipelineStageBloom()
{
	vsDelete( m_hipassMaterial );
	for ( int i = 0; i < m_passCount; i++ )
	{
		vsDelete( m_passes[i].m_horizontalBlurMaterial );
		vsDelete( m_passes[i].m_verticalBlurMaterial );
		vsDelete( m_passes[i].m_combinePassMaterial );
	}
	vsDeleteArray( m_passes );
	vsDelete( m_fromMaterial );
	vsDelete( m_vertices );
	vsDelete( m_indices );
	vsDelete( m_bloomBlurShader );
}

void
vsRenderPipelineStageBloom::PreparePipeline( vsRenderPipeline *pipeline )
{
	RenderTargetRequest req;
	req.type = RenderTargetRequest::Type_MipmapLevel;
	req.depth = false;
	req.stencil = false;
	req.linear = true;
	req.mipmaps = false;
	req.antialias = false;
	req.share = true;

	for ( int i = 0; i < m_passCount; i++ )
	{
		req.mipmapLevel = i;
		m_passes[i].m_pass = pipeline->RequestRenderTarget(req, this);
		m_passes[i].m_pass2 = pipeline->RequestRenderTarget(req, this);
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

	m_bloomBlurShader = new vsBloomBlurShader();

	for ( int i = 0; i < m_passCount; i++ )
	{
		float offsetx = 1.2f / m_passes[i].m_pass->GetWidth();
		float offsety = 1.2f / m_passes[i].m_pass->GetHeight();
		m_passes[i].m_horizontalBlurMaterial = new vsDynamicMaterial;
		m_passes[i].m_horizontalBlurMaterial->SetClampU(true);
		m_passes[i].m_horizontalBlurMaterial->SetClampV(true);
		m_passes[i].m_horizontalBlurMaterial->SetBlend(false);
		m_passes[i].m_horizontalBlurMaterial->SetTexture(0,m_passes[i].m_pass->GetTexture());
		m_passes[i].m_horizontalBlurMaterial->SetShader(m_bloomBlurShader);
		m_passes[i].m_horizontalBlurMaterial->GetResource()->m_shaderIsMine = false;
		m_passes[i].m_horizontalBlurMaterial->SetUniformF("offsetx", offsetx);
		m_passes[i].m_horizontalBlurMaterial->SetUniformF("offsety", 0.f);

		m_passes[i].m_verticalBlurMaterial = new vsDynamicMaterial;
		m_passes[i].m_verticalBlurMaterial->SetClampU(true);
		m_passes[i].m_verticalBlurMaterial->SetClampV(true);
		m_passes[i].m_verticalBlurMaterial->SetBlend(false);
		m_passes[i].m_verticalBlurMaterial->SetTexture(0,m_passes[i].m_pass2->GetTexture());
		m_passes[i].m_verticalBlurMaterial->SetShader(m_bloomBlurShader);
		m_passes[i].m_verticalBlurMaterial->GetResource()->m_shaderIsMine = false;
		m_passes[i].m_verticalBlurMaterial->SetUniformF("offsetx", 0.f);
		m_passes[i].m_verticalBlurMaterial->SetUniformF("offsety", offsety);

		m_passes[i].m_combinePassMaterial = new vsDynamicMaterial;
		m_passes[i].m_combinePassMaterial->SetBlend(true);
		m_passes[i].m_combinePassMaterial->SetColor(c_white);
		m_passes[i].m_combinePassMaterial->SetCullingType(Cull_None);
		m_passes[i].m_combinePassMaterial->SetZRead(false);
		m_passes[i].m_combinePassMaterial->SetZWrite(false);
		m_passes[i].m_combinePassMaterial->SetDrawMode(DrawMode_Add);
		m_passes[i].m_combinePassMaterial->SetGlow(false);
		m_passes[i].m_combinePassMaterial->SetClampU(true);
		m_passes[i].m_combinePassMaterial->SetClampV(true);
		m_passes[i].m_combinePassMaterial->SetTexture(0, m_passes[i].m_pass->GetTexture());
		m_passes[i].m_combinePassMaterial->SetShader(new vsBloomCombineShader);
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
	m_fromMaterial->SetTexture(0, m_from->GetTexture());
	m_fromMaterial->SetShader(new vsBloomPassShader);

	m_vertices = new vsRenderBuffer(vsRenderBuffer::Type_Static);
	m_indices = new vsRenderBuffer(vsRenderBuffer::Type_Static);

	float ar = vsScreen::Instance()->GetAspectRatio();
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
	uint16_t ind[4] = { 0, 1, 2, 3 };

	vsRenderBuffer::PT pt[4];
	for ( int i = 0; i < 4; i++ )
	{
		pt[i].position = v[i];
		pt[i].texel = t[i];
	}
	m_vertices->SetArray(pt,4);
	m_indices->SetArray(ind,4);
}

void
vsRenderPipelineStageBloom::Draw( vsDisplayList *list )
{
	// first thing is that I need to hipass 'from' into the top 'pass'.
	vsCamera2D cam;
	cam.SetFieldOfView(2.f);

	// float ar = cam.GetAspectRatio();

	list->ClearArrays();
	list->SetProjectionMatrix4x4(cam.GetProjectionMatrix());
	list->ResolveRenderTarget(m_from);
	list->SetMaterial(m_hipassMaterial);
	list->SetRenderTarget(m_passes[0].m_pass);
	list->BindBuffer(m_vertices);
	list->TriangleStripBuffer(m_indices);

	// list->BlitRenderTarget( m_passes[0].m_pass, m_to );
	// return;

	// Okay.  New plan.  We BLUR each pass, THEN blit down to the next one.
	//
	// This has two effects:  First, it eliminates aliasing on thin lines (since
	// we blur them at the high resolution BEFORE we copy down to a lower
	// resolution texture!), and second, it makes our blur BIGGER than otherwise,
	// without requiring as many blur passes.  This new approach looks BETTER
	// than the old one (particularly at low resolutions), and is also CHEAPER!
	//
	for ( int i = 0; i < m_passCount; i++ )
	{
		list->SetRenderTarget(m_passes[i].m_pass2);
		list->SetMaterial(m_passes[i].m_horizontalBlurMaterial);
		list->TriangleStripBuffer(m_indices);

		list->SetRenderTarget(m_passes[i].m_pass);
		list->SetMaterial(m_passes[i].m_verticalBlurMaterial);
		list->TriangleStripBuffer(m_indices);

		// Okay.  pass[i] has now been blurred, so blit down to the next level
		// (if there is another level)
		if ( i < m_passCount-1 )
			list->BlitRenderTarget(m_passes[i].m_pass, m_passes[i+1].m_pass);
	}

	// Now do the final combining of our stuff
	list->SetRenderTarget(m_to);
	list->SetMaterial(m_fromMaterial);
	list->TriangleStripBuffer(m_indices);

	for ( int i = 0; i < m_passCount; i++ )
	{
		list->SetMaterial(m_passes[i].m_combinePassMaterial);
		list->TriangleStripBuffer(m_indices);
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

const char *row3v = STRINGIFY( #version 330\n
		uniform mat4 viewToProjection;\n
		uniform float offsetx;
		uniform float offsety;
		in vec4 vertex;\n
		in vec2 texcoord;\n
		out vec2 fragment_texcoord[3];\n
		void main(void)\n
		{\n
			fragment_texcoord[0] = texcoord - vec2(offsetx, offsety);\n
			fragment_texcoord[1] = texcoord;\n
			fragment_texcoord[2] = texcoord + vec2(offsetx, offsety);\n
			gl_Position = viewToProjection * vertex;\n
		}\n
		);

	const char *row3f = STRINGIFY( #version 330\n
			uniform sampler2D textures[8];
			uniform float coefficients[3];
			in vec2 fragment_texcoord[3];
			out vec4 fragment_color;

			void main(void)
			{
			vec4 c;

			c = coefficients[0] * texture(textures[0], fragment_texcoord[0]);
			c += coefficients[1] * texture(textures[0], fragment_texcoord[1]);
			c += coefficients[2] * texture(textures[0], fragment_texcoord[2]);
			c *= 1.1; // a little extra "oomph" for the glow
			c.rgb = max(c.rgb, vec3(0));
			c.a = 1.0;

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
			fragment_color = color;\n
			}\n
			);


