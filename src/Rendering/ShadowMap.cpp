#include "../../include/ShadowMap.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

ShadowMap::ShadowMap(unsigned int width, unsigned int height)
	: m_DepthMap(0),
	  m_Width(width),
	  m_Height(height),
	  m_Initialized(false),
	  m_LightProjection(1.0f),
	  m_LightView(1.0f)
{
	// A practical default volume for directional-light shadow mapping.
	SetLightProjectionOrtho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 25.0f);
	UpdateLightView(glm::vec3(-2.0f, 4.0f, -1.0f), glm::vec3(0.0f));

	m_Initialized = Initialize();
}

ShadowMap::~ShadowMap()
{
	ReleaseDepthTexture();
}

bool ShadowMap::Initialize()
{
	ReleaseDepthTexture();

	if (!CreateDepthTexture()) {
		m_Initialized = false;
		return false;
	}

	m_Framebuffer.AttachTexture2D(GL_DEPTH_ATTACHMENT, m_DepthMap);
	m_Framebuffer.SetDrawReadBufferNone();
	m_Initialized = m_Framebuffer.IsComplete();

	if (!m_Initialized) {
		std::cerr << "[ShadowMap] Framebuffer setup failed." << std::endl;
		return false;
	}

	std::cout << "[ShadowMap] Initialized: " << m_Width << "x" << m_Height << std::endl;
	return true;
}

bool ShadowMap::Resize(unsigned int width, unsigned int height)
{
	if (width == 0 || height == 0) {
		std::cerr << "[ShadowMap] Resize ignored: invalid size." << std::endl;
		return false;
	}

	if (width == m_Width && height == m_Height) {
		return true;
	}

	m_Width = width;
	m_Height = height;
	return Initialize();
}

void ShadowMap::BeginRender() const
{
	if (!m_Initialized) {
		std::cerr << "[ShadowMap] BeginRender failed: shadow map is not initialized." << std::endl;
		return;
	}

	m_Framebuffer.Bind();
	glViewport(0, 0, static_cast<int>(m_Width), static_cast<int>(m_Height));
	glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::EndRender(unsigned int viewportWidth, unsigned int viewportHeight) const
{
	Framebuffer::Unbind();
	glViewport(0, 0, static_cast<int>(viewportWidth), static_cast<int>(viewportHeight));
}

void ShadowMap::BindDepthTexture(unsigned int slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_DepthMap);
}

void ShadowMap::SetLightProjectionOrtho(float left, float right, float bottom, float top,
										float nearPlane, float farPlane)
{
	m_LightProjection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

void ShadowMap::UpdateLightView(const glm::vec3& lightPosition,
								const glm::vec3& target,
								const glm::vec3& up)
{
	m_LightView = glm::lookAt(lightPosition, target, up);
}

glm::mat4 ShadowMap::GetLightSpaceMatrix() const
{
	return m_LightProjection * m_LightView;
}

bool ShadowMap::CreateDepthTexture()
{
	glGenTextures(1, &m_DepthMap);
	if (m_DepthMap == 0) {
		std::cerr << "[ShadowMap] Failed to create depth texture." << std::endl;
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, m_DepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
				 static_cast<int>(m_Width), static_cast<int>(m_Height),
				 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

void ShadowMap::ReleaseDepthTexture()
{
	if (m_DepthMap != 0) {
		glDeleteTextures(1, &m_DepthMap);
		m_DepthMap = 0;
	}
}
