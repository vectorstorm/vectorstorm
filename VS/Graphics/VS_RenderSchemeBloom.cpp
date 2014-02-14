/*
 *  VS_RendererBloom.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 4/01/08.
 *  Copyright 2009 Trevor Powell. All rights reserved.
 *
 */

#if !TARGET_OS_IPHONE

#include "VS_OpenGL.h"

#include "VS_RenderSchemeBloom.h"
#include "VS_RenderTarget.h"

#include "VS_Image.h"

vsRenderTarget *g_boundSurface = NULL;

bool g_renderSceneTexture = false;
bool g_renderOffscreenTexture = false;
int  g_renderOffscreenTextureId = 0;


#define STRINGIFY(A)  #A


const char *overlayv = STRINGIFY(
								 //uniform vec4 colorA;
								 //uniform vec4 colorB;
								 uniform vec4 pos;
								 uniform vec4 dir;
								 uniform float invDist;

								 varying float t;

								 void main(void)
								 {
								 vec4 localOffset;

								 localOffset = (gl_Vertex-pos) * invDist;
								 t = dot(localOffset,dir);

								 gl_FrontColor = gl_Color;
								 gl_BackColor = gl_Color;
								 gl_Position = ftransform();
								 }
);

const char *overlayf = STRINGIFY(
								 uniform vec4 colorA;
								 uniform vec4 colorB;
								 varying float t;

								 void main(void)
								 {
								 float ct = min(1.0,max(0.0,t));
								 float invCT = 1.0 - ct;
								 vec4  overlayColor = (colorA * invCT) + (colorB * ct);

								 gl_FragColor = gl_Color * overlayColor;
								 }
);


const char *passv = STRINGIFY(
							  void main(void)
							  {
							  gl_TexCoord[0] = gl_MultiTexCoord0;
							  gl_Position    = ftransform();
							  }
);

const char *combine4f = STRINGIFY(
								  uniform sampler2D Pass0;
								  uniform sampler2D Pass1;
								  uniform sampler2D Pass2;
								  uniform sampler2D Scene;

								  void main(void)
								  {
								  vec4 t0 = texture2D(Pass0, gl_TexCoord[0].st);
								  vec4 t1 = texture2D(Pass1, gl_TexCoord[0].st);
								  vec4 t2 = texture2D(Pass2, gl_TexCoord[0].st);
								  vec4 t3 = texture2D(Scene, gl_TexCoord[0].st);
								  gl_FragColor = (t0 + t1 + t2) + t3;
								  }
);

const char *combine5f = STRINGIFY(
								  uniform sampler2D Pass0;
								  uniform sampler2D Pass1;
								  uniform sampler2D Pass2;
								  uniform sampler2D Pass3;
								  uniform sampler2D Scene;

								  void main(void)
								  {
								  vec4 t0 = texture2D(Pass0, gl_TexCoord[0].st);
								  vec4 t1 = texture2D(Pass1, gl_TexCoord[0].st);
								  vec4 t2 = texture2D(Pass2, gl_TexCoord[0].st);
								  vec4 t3 = texture2D(Pass3, gl_TexCoord[0].st);
								  vec4 t4 = texture2D(Scene, gl_TexCoord[0].st);
								  gl_FragColor = t0 + t1 + t2 + t3 + t4;
								  }
);

const char *combine6f = STRINGIFY(
								  uniform sampler2D Pass0;
								  uniform sampler2D Pass1;
								  uniform sampler2D Pass2;
								  uniform sampler2D Pass3;
								  uniform sampler2D Pass4;
								  uniform sampler2D Scene;

								  void main(void)
								  {
								  vec4 t0 = texture2D(Pass0, gl_TexCoord[0].st);
								  vec4 t1 = texture2D(Pass1, gl_TexCoord[0].st);
								  vec4 t2 = texture2D(Pass2, gl_TexCoord[0].st);
								  vec4 t3 = texture2D(Pass3, gl_TexCoord[0].st);
								  vec4 t4 = texture2D(Pass4, gl_TexCoord[0].st);
								  vec4 t5 = texture2D(Scene, gl_TexCoord[0].st);
								  gl_FragColor = t0 + t1 + t2 + t3 + t4 + t5;
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

const char *row5f = STRINGIFY(
							  uniform sampler2D source;
							  uniform float coefficients[5];
							  uniform float offsetx;
							  uniform float offsety;

							  void main(void)
							  {
							  vec4 c;
							  vec2 tc = gl_TexCoord[0].st;
							  vec2 offset = vec2(offsetx, offsety);

							  c = coefficients[0] * texture2D(source, tc - (offset*2.0));
							  c += coefficients[1] * texture2D(source, tc - offset);
							  c += coefficients[2] * texture2D(source, tc);
							  c += coefficients[3] * texture2D(source, tc + offset);
							  c += coefficients[4] * texture2D(source, tc + (offset*2.0));

							  gl_FragColor = c;
							  }
);

const char *row7f = STRINGIFY(
							  uniform sampler2D source;
							  uniform float coefficients[7];
							  uniform float offsetx;
							  uniform float offsety;

							  void main(void)
							  {
							  vec4 c;
							  vec2 tc = gl_TexCoord[0].st;
							  vec2 offset = vec2(offsetx, offsety);

							  c  = coefficients[0] * texture2D(source, tc - (offset*3.0));
							  c += coefficients[1] * texture2D(source, tc - (offset*2.0));
							  c += coefficients[2] * texture2D(source, tc - offset);
							  c += coefficients[3] * texture2D(source, tc);
							  c += coefficients[4] * texture2D(source, tc + offset);
							  c += coefficients[5] * texture2D(source, tc + (offset*2.0));
							  c += coefficients[6] * texture2D(source, tc + (offset*3.0));

							  gl_FragColor = c;
							  }
);

const char *hipassf = STRINGIFY(
								uniform sampler2D source;

								void main(void)
								{
								vec4 color = texture2D(source, gl_TexCoord[0].st);

								//float intensity = (color.x*color.x) + (color.y*color.y) + (color.z*color.z);
//								float bloom = intensity > 0.8 ? intensity-0.8 * 5.0 : 0.0;
								//float bloom = intensity > 0.6 ? intensity-0.6 * 2.0 : 0.0;

								//color.xyz *= bloom;
								//color.x = intensity > 0.8 ? (color.x-0.9) * 10.0 : 0.0;
								//color.y = intensity > 0.8 ? (color.y-0.9) * 10.0 : 0.0;
								//color.z = intensity > 0.8 > 0.8 ? (color.z-0.9) * 10.0 : 0.0;
								color.xyz *= color.xyz;

								gl_FragColor = color;
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


#define KERNEL_SIZE   (3)
//#define KERNEL_SIZE   (5)

GLfloat black[4] = {0,0,0,0};
//float kernel[KERNEL_SIZE] = { 3, 7, 26, 41, 26, 7, 3  };
//float kernel[KERNEL_SIZE] = { 7, 26, 41, 26, 7  };
float kernel[KERNEL_SIZE] = { 4, 5, 4  };

#define BUFFER_HEIGHT (512)
#define BUFFER_WIDTH  (512)


vsRenderSchemeBloom::vsRenderSchemeBloom(vsRenderer *renderer):
	vsRenderScheme(renderer),
	m_window(NULL),
	m_scene(NULL)
{
	m_antialias = (glRenderbufferStorageMultisampleEXT != NULL);

	// Compile shaders
	m_combineProg = Compile(passv, combine7f);
	m_filterProg = Compile(passv, row3f);
	m_overlayProg = Compile(overlayv, overlayf);
	m_hipassProg = Compile(passv, normalf);

    m_locSource = glGetUniformLocation(m_filterProg, "source");
    m_locCoefficients = glGetUniformLocation(m_filterProg, "coefficients");
    m_locOffsetX = glGetUniformLocation(m_filterProg, "offsetx");
    m_locOffsetY = glGetUniformLocation(m_filterProg, "offsety");
    m_combineScene = glGetUniformLocation(m_combineProg, "Scene");

    for ( int i = 0; i < FILTER_COUNT; i++ )
    {
        char name[]="Pass#";
        sprintf(name, "Pass%d", i);
        m_passInt[i] = glGetUniformLocation(m_combineProg, name);
		m_pass[i] = m_pass2[i] = NULL;
    }



	// Normalize kernel coefficients
    float sum = 0;
    for (int c = 0; c < KERNEL_SIZE; c++)
        sum += kernel[c];
    for (int c = 0; c < KERNEL_SIZE; c++)
        kernel[c] *= (1.f / sum);

    glMatrixMode(GL_MODELVIEW);

	Resize();
}

vsRenderSchemeBloom::~vsRenderSchemeBloom()
{
	DestroyShader(m_combineProg);
	DestroyShader(m_filterProg);
	DestroyShader(m_overlayProg);
	DestroyShader(m_hipassProg);

	vsDelete( m_window );
	vsDelete( m_scene );
	for (int p = 0; p < FILTER_COUNT; p++)
	{
		vsDelete( m_pass[p] );
		vsDelete( m_pass2[p] );
	}
}

void
vsRenderSchemeBloom::Resize()
{
	vsDelete( m_window );
	vsDelete( m_scene );
	for (int p = 0; p < FILTER_COUNT; p++)
	{
		vsDelete( m_pass[p] );
		vsDelete( m_pass2[p] );
	}

	// Create Window Surface
	vsSurface::Settings settings;
	settings.depth = 0;
	settings.width = m_renderer->GetWidthPixels();
	settings.height = m_renderer->GetHeightPixels();
	settings.ortho = true;
	m_window = new vsRenderTarget( vsRenderTarget::Type_Window, settings );

    // Create 3D Scene Surface
	// we want to be big enough to hold our full m_window resolution, and set our viewport to match the window.

    settings.width = m_renderer->GetWidthPixels();
    settings.height = m_renderer->GetHeightPixels();
	settings.depth = true;
    settings.linear = true;
	settings.mipMaps = false;
	settings.ortho = true;
	settings.stencil = true;

	if ( m_antialias )
	{
		m_scene = new vsRenderTarget( vsRenderTarget::Type_Multisample, settings );
	}
	else
	{
		m_scene = new vsRenderTarget( vsRenderTarget::Type_Texture, settings );
	}
	m_renderer->SetViewportWidthPixels( m_scene->GetViewportWidth() );
	m_renderer->SetViewportHeightPixels( m_scene->GetViewportHeight() );

	int width = BUFFER_WIDTH;
	int height = BUFFER_HEIGHT;
	// Create Source Surfaces
	for (int p = 0; p < FILTER_COUNT; p++)
	{
		settings.width = width >> p;
		settings.height = height >> p;
		settings.depth = false;
		settings.linear = true;
		settings.mipMaps = false;
		settings.ortho = false;
		settings.stencil = false;
		m_pass[p] = new vsRenderTarget( vsRenderTarget::Type_Texture, settings );
		m_pass2[p] = new vsRenderTarget( vsRenderTarget::Type_Texture, settings );
	}
}

const char c_enums[][20] =
{
	"attachment",         // GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT........... All framebuffer attachment points are 'framebuffer attachment complete'.
	"missing attachment", // GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT....There is at least one image attached to the framebuffer.
	"",                   //
	"dimensions",         // GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT............All attached images have the same width and height.
	"formats",            // GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT...............All images attached to the attachment points COLOR_ATTACHMENT0_EXT through COLOR_ATTACHMENTn_EXT must have the same internal format.
	"draw buffer",        // GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT...........The value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT must not be NONE for any color attachment point(s) named by DRAW_BUFFERi.
	"read buffer",        // GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT...........If READ_BUFFER is not NONE, then the value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT must not be NONE for the color attachment point named by READ_BUFFER.
	"unsupported format"  // GL_FRAMEBUFFER_UNSUPPORTED_EXT......................The combination of internal formats of the attached images does not violate an implementation-dependent set of restrictions.
};

void
vsRenderSchemeBloom::CheckFBO()
{
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
        return;

    status -= GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
    vsAssert(status == GL_FRAMEBUFFER_COMPLETE_EXT,vsFormatString("incomplete framebuffer object due to %s", c_enums[status]));
}

GLuint
vsRenderSchemeBloom::Compile(const char *vert, const char *frag, int vLength, int fLength )
{
    GLchar buf[256];
    GLuint vertShader, fragShader, program;
    GLint success;

	GLint *vLengthPtr = NULL;
	GLint *fLengthPtr = NULL;

	if ( vLength > 0 )
		vLengthPtr = (GLint*)&vLength;
	if ( fLength > 0 )
		fLengthPtr = (GLint*)&fLength;

    vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, (const GLchar**) &vert, vLengthPtr);
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertShader, sizeof(buf), 0, buf);
        vsLog(buf);
        vsAssert(success,"Unable to compile vertex shader.\n");
    }

    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, (const GLchar**) &frag, fLengthPtr);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragShader, sizeof(buf), 0, buf);
        vsLog(buf);
        vsAssert(success,"Unable to compile fragment shader.\n");
    }

    program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, sizeof(buf), 0, buf);
        vsLog(buf);
        vsAssert(success,"Unable to link shaders.\n");
    }
	glDetachShader(program,vertShader);
	glDetachShader(program,fragShader);
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

    return program;
}

void
vsRenderSchemeBloom::DestroyShader(GLuint shader)
{
	glDeleteProgram(shader);
}


void
vsRenderSchemeBloom::Blur(vsRenderTarget **sources, vsRenderTarget **dests, int count, Direction dir)
{
    int p;

    // Set up the filter.
    glUseProgram(m_filterProg);

    glUniform1i(m_locSource, 0);
    glUniform1fv(m_locCoefficients, KERNEL_SIZE, kernel);
    glUniform1f(m_locOffsetX, 0);
    glUniform1f(m_locOffsetY, 0);

	/*float top = -1.f;
	float left = -1.f;
	float right = (2.0f * m_scene.texWidth) - 1.f;	// texWidth is the fraction of our width that we're actually using.
	float bot = (2.0f * m_scene.texHeight) - 1.f;	// texWidth is the fraction of our width that we're actually using.
	*/

    // Perform the blurring.
    for (p = 0; p < count; p++)
    {
		float offset = 1.2f / sources[p]->GetWidth();
		glUniform1f(m_locOffsetX, offset);
		glUniform1f(m_locOffsetY, 0.f);
		dests[p]->Bind();
		dests[p]->Clear();
		glBindTexture(GL_TEXTURE_2D, sources[p]->GetTexture()->GetResource()->GetTexture());
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glUniform1f(m_locOffsetX, 0);

    // Perform the blurring.
    for (p = 0; p < count; p++)
    {
		float offset = 1.2f / dests[p]->GetWidth();
		glUniform1f(m_locOffsetX, 0.f);
		glUniform1f(m_locOffsetY, offset);
		sources[p]->Bind();
		sources[p]->Clear();
		glBindTexture(GL_TEXTURE_2D, dests[p]->GetTexture()->GetResource()->GetTexture());
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}


void
vsRenderSchemeBloom::PreRender(const vsRenderer::Settings &s)
{
	m_scene->Bind();

	if ( m_antialias )
	{
		m_renderer->GetState()->SetBool( vsRendererState::Bool_Multisample, true );
	}
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );

	m_renderer->GetState()->SetBool( vsRendererState::Bool_DepthMask, true );
	m_renderer->GetState()->Flush();
}

void
vsRenderSchemeBloom::PostRender()
{
	m_renderer->GetState()->SetBool( vsRendererState::Bool_Fog, false );
	//	m_renderer->GetState()->SetBool( vsRendererState::Bool_AlphaTest, false );
	m_renderer->GetState()->SetBool( vsRendererState::Bool_CullFace, false );
	m_renderer->GetState()->SetBool( vsRendererState::Bool_Lighting, false );
	m_renderer->GetState()->SetBool( vsRendererState::Bool_ColorMaterial, false );
	m_renderer->GetState()->SetBool( vsRendererState::Bool_Multisample, false );
	m_renderer->GetState()->Flush();

	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);	// opaque

	m_renderer->CheckGLError("PrePostRender");

	m_scene->Resolve();

	int p;

	glUseProgram(0);
	glBlendFunc( GL_ONE, GL_ZERO );

	vsTuneable float glowBrightness = 1.0f;
	// don't hi-pass;  just scale down colors.
	glColor4f(glowBrightness,glowBrightness,glowBrightness,1.f);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scene->GetTexture()->GetResource()->GetTexture());
    glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	m_pass[0]->Bind();


	float top = -1.f;
	float left = -1.f;
	float right = (2.0f * m_scene->GetTexWidth()) - 1.f;	// texWidth is the fraction of our width that we're actually using.
	float bottom = (2.0f * m_scene->GetTexHeight()) - 1.f;	// texWidth is the fraction of our width that we're actually using.

//	glUseProgram(0);
    glUseProgram(m_hipassProg);

	float v[8] = {
		left, top,
		right, top,
		left, bottom,
		right, bottom
	};
	float wv[8] = {
		0.f,0.f,
		(float)m_window->GetWidth(), 0.f,
		0.f,(float)m_window->GetHeight(),
		(float)m_window->GetWidth(),(float)m_window->GetHeight()
	};
	float t[8] = {
		0.f, 0.f,
		m_scene->GetTexWidth(), 0.f,
		0.f, m_scene->GetTexHeight(),
		m_scene->GetTexWidth(), m_scene->GetTexHeight()
	};

	m_renderer->GetState()->SetBool( vsRendererState::ClientBool_VertexArray, true );
	m_renderer->GetState()->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );
	m_renderer->GetState()->Flush();

	glVertexPointer( 2, GL_FLOAT, 0, v );
	glTexCoordPointer( 2, GL_FLOAT, 0, t );

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram( 0 );

	glColor4f(1.f,1.f,1.f,1.f);

    for (p = 1; p < FILTER_COUNT; p++)
    {
		m_pass[p-1]->BlitTo( m_pass[p] );
    }

	// Perform the blur operations.
	Blur(m_pass, m_pass2, FILTER_COUNT, HORIZONTAL);


	m_window->Bind();

    glUseProgram(m_combineProg);

    for (p = 0; p < FILTER_COUNT; p++)
    {
        glActiveTexture(GL_TEXTURE0 + p);
        glBindTexture(GL_TEXTURE_2D, m_pass[p]->GetTexture()->GetResource()->GetTexture());
        glEnable(GL_TEXTURE_2D);

        glUniform1i(m_passInt[p], p);
    }

	glActiveTexture(GL_TEXTURE0 + FILTER_COUNT);
	glBindTexture(GL_TEXTURE_2D, m_scene->GetTexture()->GetResource()->GetTexture());
    glEnable(GL_TEXTURE_2D);
    glUniform1i(m_combineScene, FILTER_COUNT);

	glVertexPointer( 2, GL_FLOAT, 0, wv );
	/*
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2i(0, 0); glVertex2i(0, 0);
    glTexCoord2f(m_scene.texWidth, 0); glVertex2i(m_window.width, 0);
    glTexCoord2f(0, m_scene.texHeight); glVertex2i(0, m_window.height);
    glTexCoord2f(m_scene.texWidth, m_scene.texHeight); glVertex2i(m_window.width, m_window.height);
    glEnd();
	*/
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	m_renderer->GetState()->SetBool( vsRendererState::ClientBool_VertexArray, false );
	m_renderer->GetState()->SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
	m_renderer->GetState()->Flush();

    glUseProgram(0);

    for (p = 0; p < FILTER_COUNT; p++)
    {
        glActiveTexture(GL_TEXTURE0 + p);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0 + FILTER_COUNT);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);

#if defined(_DEBUG)

	if ( g_renderOffscreenTexture || g_renderSceneTexture )
	{
		m_window->Bind();
		m_window->Clear();
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		if ( g_renderOffscreenTexture )
		{
			int passId = g_renderOffscreenTextureId;
			glBindTexture(GL_TEXTURE_2D, m_pass[passId]->GetTexture()->GetResource()->GetTexture());
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_scene->GetTexture()->GetResource()->GetTexture());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//	glGenerateMipmapEXT(GL_TEXTURE_2D);
		}

		glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex2i(0, 0);
		glTexCoord2f(m_scene->GetTexWidth(), 0); glVertex2i(m_window->GetWidth(), 0);
		glTexCoord2f(m_scene->GetTexWidth(), m_scene->GetTexHeight()); glVertex2i(m_window->GetWidth(), m_window->GetHeight());
		glTexCoord2f(0, m_scene->GetTexHeight()); glVertex2i(0, m_window->GetHeight());
		glEnd();

		glDisable(GL_TEXTURE_2D);
	}
#endif // _DEBUG

}

bool
vsRenderSchemeBloom::PreRenderTarget( const vsRenderer::Settings &s, vsRenderTarget *target )
{
	target->Bind();
	if ( m_antialias )
	{
		m_renderer->GetState()->SetBool( vsRendererState::Bool_Multisample, true );
	}
//	glBlendFunc( GL_ONE, GL_ZERO );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	//ClearSurface();
	//glEnable(GL_MULTISAMPLE);

	//glDepthMask(GL_TRUE);
	m_renderer->GetState()->SetBool( vsRendererState::Bool_DepthMask, true );
	m_renderer->GetState()->Flush();
	glClearColor(0.f,0.f,0.f,0.f);
	glClearDepth(1.f);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	return true;
}

bool
vsRenderSchemeBloom::PostRenderTarget( vsRenderTarget *target )
{
	m_scene->Bind();
	return true;
}

bool
vsRenderSchemeBloom::Supported(bool experimental)
{
	glewExperimental = experimental;

	if ( glGenRenderbuffersEXT && glBindRenderbufferEXT && glRenderbufferStorageEXT && glGenFramebuffersEXT &&
		glBindFramebufferEXT && glFramebufferTexture2DEXT && glCreateShader && glShaderSource && glCompileShader &&
		glGetShaderiv && glCreateProgram && glAttachShader && glLinkProgram )
	{
		GLint textureUnits;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &textureUnits);
		if ( textureUnits < 7 )
		{
			vsLog("Need 7 texture units for bloom shader.  Only %d supported.", textureUnits);
			return false;
		}
		if ( !experimental )
			vsLog("Successfully detected all required extensions for bloom shader.");
		else if ( experimental )
		{
			vsLog("Detected all required extensions for bloom shader, though driver did not declare them all publicly.");
			vsLog("Warning:  some of these extensions may be experimental or non-functional in your video driver!");
			vsLog("If you experience difficulty or crashes, please modify \"Data/Preferences/global.prefs\" and switch the line \"Bloom 1 0 1\" to \"Bloom 0 0 1\" to turn the bloom shader back off again.");
		}
		return true;
	}

	vsLog("OpenGL extensions not supported for bloom shader:");
	if ( !glGenRenderbuffersEXT )
		vsLog("glGenRenderBuffersEXT");
	if ( !glBindRenderbufferEXT )
		vsLog("glBindRenderbufferEXT");
	if ( !glRenderbufferStorageEXT )
		vsLog("glRenderbufferStorageEXT");
	if ( !glGenFramebuffersEXT )
		vsLog("glGenFramebuffersEXT");
	if ( !glBindFramebufferEXT )
		vsLog("glBindFramebufferEXT");
	if ( !glFramebufferTexture2DEXT )
		vsLog("glFramebufferTexture2DEXT" );
	if ( !glCreateShader )
		vsLog("glCreateShader");
	if ( !glShaderSource )
		vsLog("glShaderSource");
	if ( !glCompileShader )
		vsLog("glCompileShader");
	if ( !glGetShaderiv )
		vsLog("glGetShaderiv");
	if ( !glCreateProgram )
		vsLog("glCreateProgram");
	if ( !glAttachShader )
		vsLog("glAttachShader");
	if ( !glLinkProgram )
		vsLog("glLinkProgram");

	if ( !experimental )
	{
		vsLog("Failed to find all OpenGL functions required for bloom shader.");
		vsLog("Trying a deep scan for OpenGL functions improperly exposed by the driver.");
		return Supported(true);
	}

	return false;
}

vsImage *
vsRenderSchemeBloom::Screenshot()
{
	size_t width = m_window->GetViewportWidth();
	size_t height = m_window->GetViewportHeight();
	const size_t bytesPerPixel = 3;	// RGB
	const size_t imageSizeInBytes = bytesPerPixel * width * height;

	uint8_t* pixels = new uint8_t[imageSizeInBytes];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	m_window->Bind();
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	m_renderer->CheckGLError("glReadPixels");

	vsImage *image = new vsImage( width, height );

	for ( size_t y = 0; y < height; y++ )
	{
		int rowStart = y * width * bytesPerPixel;

		for ( size_t x = 0; x < width; x++ )
		{
			int rInd = rowStart + (x*bytesPerPixel);
			int gInd = rInd+1;
			int bInd = rInd+2;

			int rVal = pixels[rInd];
			int gVal = pixels[gInd];
			int bVal = pixels[bInd];

			image->Pixel(x,y).Set( rVal/255.f, gVal/255.f, bVal/255.f, 1.0f );
		}
	}

	vsDeleteArray( pixels );

	return image;
}


vsImage *
vsRenderSchemeBloom::ScreenshotDepth()
{
	size_t width = m_window->GetViewportWidth();
	size_t height = m_window->GetViewportHeight();
	int imageSize = sizeof(float) * width * height;

	float* pixels = new float[imageSize];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	m_scene->Bind();
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
	m_renderer->CheckGLError("glReadPixels");


	vsImage *image = new vsImage( width, height );

	for ( size_t y = 0; y < height; y++ )
	{
		int rowStart = y * width;

		for ( size_t x = 0; x < width; x++ )
		{
			int ind = rowStart + x;

			float val = pixels[ind];

			image->Pixel(x,y).Set( val, val, val, 1.0f );
		}
	}

	vsDeleteArray( pixels );

	return image;
}

vsImage *
vsRenderSchemeBloom::ScreenshotAlpha()
{
	size_t width = m_window->GetViewportWidth();
	size_t height = m_window->GetViewportHeight();
	int imageSize = sizeof(float) * width * height;

	float* pixels = new float[imageSize];

	// glReadPixels can align the first pixel in each row at 1-, 2-, 4- and 8-byte boundaries. We
	// have allocated the exact size needed for the image so we have to use 1-byte alignment
	// (otherwise glReadPixels would write out of bounds)
	m_scene->Bind();
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_ALPHA, GL_FLOAT, pixels);
	m_renderer->CheckGLError("glReadPixels");


	vsImage *image = new vsImage( width, height );

	for ( size_t y = 0; y < height; y++ )
	{
		int rowStart = y * width;

		for ( size_t x = 0; x < width; x++ )
		{
			int ind = rowStart + x;

			float val = pixels[ind];

			image->Pixel(x,y).Set( val, val, val, 1.0f );
		}
	}

	vsDeleteArray( pixels );

	return image;
}

#endif // TARGET_OS_IPHONE

