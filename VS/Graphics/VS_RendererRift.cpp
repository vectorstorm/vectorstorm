/*
 *  VS_RendererRift.cpp
 *  VectorStorm
 *
 *  Created by Trevor Powell on 16-07-2013.
 *  Copyright 2013 Trevor Powell.  All rights reserved.
 *
 */
#include "VS_RendererRift.h"
#include "VS_Rift.h"

#define STRINGIFY(A)  #A
static const char *passv = STRINGIFY(
							  void main(void)
							  {
							  gl_TexCoord[0] = gl_MultiTexCoord0;
							  gl_Position    = ftransform();
							  }
);

static const char *distortf = STRINGIFY(
								  uniform sampler2D Scene;

								  uniform vec2 LensCenter;
								  uniform vec2 ScreenCenter;
								  uniform vec2 Scale;
								  uniform vec2 ScaleIn;
								  uniform vec4 HmdWarpParam;

								  vec2 HmdWarp(vec2 tc)
								  {
								  vec2 theta = (tc - LensCenter) * ScaleIn; // Scales to [-1,1]
								  float rSq = theta.x * theta.x + theta.y * theta.y;
								  vec2 rvector = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq +
									  HmdWarpParam.z * rSq * rSq +
									  HmdWarpParam.w * rSq * rSq * rSq);
								  return LensCenter + Scale * rvector;
								  }

								  void main(void)
								  {
								  vec2 tc = HmdWarp(gl_TexCoord[0].st);
								  //gl_FragColor = vec4(gl_TexCoord[0].s, gl_TexCoord[0].t, 0.0, 1.0);
								  //gl_FragColor = vec4(Scale.x, Scale.y, 0.0, 1.0);
								  //vec2 tc = gl_TexCoord[0].st;
								  if ( tc.x < ScreenCenter.x - 0.25 || tc.x > ScreenCenter.x + 0.25 )
								  {
									  gl_FragColor = vec4(1.0,0.0,0.0,1.0);
								  }
								  else if ( tc.y < ScreenCenter.y - 0.5 || tc.y > ScreenCenter.y + 0.5 )
								  {
									  gl_FragColor = vec4(1.0,0.0,0.0,1.0);
								  }
								  else
								  {
									  vec4 t0 = texture2D(Scene, tc);
									  gl_FragColor = t0;
									  //gl_FragColor = vec4(0.5 + 0.5 * tc.s, 0.5 + 0.5 * tc.t, 0.0, 1.0);
								  }
								  //gl_FragColor = vec4(1.0,0.0,0.0,1.0);
								  }
);

vsRendererRift::vsRendererRift():
	vsRendererShader()
{
}

vsRendererRift::~vsRendererRift()
{
}

void
vsRendererRift::InitPhaseTwo(int width, int height, int depth, bool fullscreen)
{
	vsRendererShader::InitPhaseTwo(width,height,depth,fullscreen);
	m_distortProg = Compile(passv, distortf);
    m_lensCenter = glGetUniformLocation(m_distortProg, "LensCenter");
    m_screenCenter = glGetUniformLocation(m_distortProg, "ScreenCenter");
    m_scale = glGetUniformLocation(m_distortProg, "Scale");
    m_scaleIn = glGetUniformLocation(m_distortProg, "ScaleIn");
    m_hmdWarpParam = glGetUniformLocation(m_distortProg, "HmdWarpParam");
	m_textureSampler = glGetUniformLocation(m_distortProg, "Scene");
}

void
vsRendererRift::PostBloom()
{
	// Resolve implicitly does a 'Bind'.  D'oh!
	m_scene->Resolve();

	m_state.SetBool( vsRendererState::ClientBool_VertexArray, true );
	m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, true );
	m_state.Flush();

	float top = -1.f;
	float left = -1.f;
	float right = (2.0f * m_scene->GetTexWidth()) - 1.f;	// texWidth is the fraction of our width that we're actually using.
	float bot = (2.0f * m_scene->GetTexHeight()) - 1.f;	// texWidth is the fraction of our width that we're actually using.
	float lrmid = left + ((right-left) * 0.5f);
	float tbmid = top + bot * 0.5f;
	float aspectRatio = (m_window->GetWidth() * 0.5f) / m_window->GetHeight();

	glUseProgram(m_distortProg);
	{
		float v[8] = {
			left, top,
			lrmid, top,
			left, bot,
			lrmid, bot
		};
		float t[8] = {
			0.f, 0.f,
			m_scene->GetTexWidth()*0.5f, 0.f,
			0.f, m_scene->GetTexHeight(),
			m_scene->GetTexWidth()*0.5f, m_scene->GetTexHeight()
		};

		float texLeft = 0.f;
		float texRight = m_scene->GetTexWidth() * 0.5f;
		float texLRMid = (texRight + texLeft) * 0.5f;
		float texTop = 0.f;
		float texBot = m_scene->GetTexHeight();
		float texTBMid = (texTop + texBot) * 0.5f;

		float texWidth = texRight - texLeft;
		float texHeight = texBot - texTop;
		vsRift *r = vsRift::Instance();
		struct EyeDistortionParams edp = r->GetDistortionParams(RiftEye_Left);

		//vsVector2D lensCenter(texLRMid,texTBMid);
		vsVector2D lensCenter(texLeft + (texWidth + edp.XCenterOffset * 0.5f) * 0.5f,edp.YCenterOffset + texTBMid);
		vsVector2D screenCenter(texLRMid, texTBMid);
		// scaleIn needs to take the width/height trom (texRight-texLeft) to 2.0
		vsVector2D scaleIn(2.0 / (texWidth), 2.0 / (texHeight) / aspectRatio);
		float scaleFactor = 1.0f / edp.Scale;
		vsVector2D scale(((texWidth)/2.0f) * scaleFactor, ((texHeight)/ 2.0f) * scaleFactor * aspectRatio);
		//scale *= 0.8f;
		vsVector4D distortionK = r->GetDistortionK();
		glUniform1i(m_textureSampler, 0);
		glUniform2fv(m_lensCenter, 1, (float*)&lensCenter);
		glUniform2fv(m_screenCenter, 1, (float*)&screenCenter);
		glUniform2fv(m_scale, 1, (float*)&scale);
		glUniform2fv(m_scaleIn, 1, (float*)&scaleIn);
		glUniform4fv(m_hmdWarpParam, 1, (float*)&distortionK);

		m_scene->Bind();
		//glUseProgram(0);
		glVertexPointer( 2, GL_FLOAT, 0, v );
		glTexCoordPointer( 2, GL_FLOAT, 0, t );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_scene->GetTexture()->GetResource()->GetTexture());
		glEnable(GL_TEXTURE_2D);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisable(GL_TEXTURE_2D);
	}
	{
		float v[8] = {
			lrmid, top,
			right, top,
			lrmid, bot,
			right, bot
		};
		float t[8] = {
			m_scene->GetTexWidth()*0.5f, 0.f,
			m_scene->GetTexWidth(), 0.f,
			m_scene->GetTexWidth()*0.5f, m_scene->GetTexHeight(),
			m_scene->GetTexWidth(), m_scene->GetTexHeight()
		};

		float texLeft = m_scene->GetTexWidth() * 0.5f;
		float texRight = m_scene->GetTexWidth();
		float texLRMid = (texRight + texLeft) * 0.5f;
		float texTop = 0.f;
		float texBot = m_scene->GetTexHeight();
		float texTBMid = (texTop + texBot) * 0.5f;
		float texWidth = texRight - texLeft;
		float texHeight = texBot - texTop;
		vsRift *r = vsRift::Instance();
		struct EyeDistortionParams edp = r->GetDistortionParams(RiftEye_Right);

		//vsVector2D lensCenter(texLRMid,texTBMid);
		vsVector2D lensCenter(texLeft + (texWidth + edp.XCenterOffset * 0.5f) * 0.5f,edp.YCenterOffset + texTBMid);
		vsVector2D screenCenter(texLRMid, texTBMid);
		// scaleIn needs to take the width/height trom (texRight-texLeft) to 2.0
		vsVector2D scaleIn(2.0 / (texWidth), 2.0 / (texHeight) / aspectRatio);
		float scaleFactor = 1.0f / edp.Scale;
		vsVector2D scale(((texWidth)/2.0f) * scaleFactor, ((texHeight)/ 2.0f) * scaleFactor * aspectRatio);
		//scale *= 0.8f;
		vsVector4D distortionK = r->GetDistortionK();
		glUniform1i(m_textureSampler, 0);
		glUniform2fv(m_lensCenter, 1, (float*)&lensCenter);
		glUniform2fv(m_screenCenter, 1, (float*)&screenCenter);
		glUniform2fv(m_scale, 1, (float*)&scale);
		glUniform2fv(m_scaleIn, 1, (float*)&scaleIn);
		glUniform4fv(m_hmdWarpParam, 1, (float*)&distortionK);

		m_scene->Bind();
		//glUseProgram(0);
		glVertexPointer( 2, GL_FLOAT, 0, v );
		glTexCoordPointer( 2, GL_FLOAT, 0, t );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_scene->GetTexture()->GetResource()->GetTexture());
		glEnable(GL_TEXTURE_2D);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisable(GL_TEXTURE_2D);
	}

	m_scene->Resolve();
	CheckGLError("PostBloom");

	m_state.SetBool( vsRendererState::ClientBool_VertexArray, false );
	m_state.SetBool( vsRendererState::ClientBool_TextureCoordinateArray, false );
	m_state.Flush();

    glUseProgram(0);

	m_scene->Resolve();
}

bool
vsRendererRift::Supported(bool experimental)
{
	//return false;
	return vsRendererShader::Supported(experimental);
}
