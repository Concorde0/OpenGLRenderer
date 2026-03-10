#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "Framebuffer.h"

#include <glm/glm.hpp>

class ShadowMap {
public:
	ShadowMap(unsigned int width = 2048, unsigned int height = 2048);
	~ShadowMap();

	ShadowMap(const ShadowMap&) = delete;
	ShadowMap& operator=(const ShadowMap&) = delete;

	bool Initialize();
	bool Resize(unsigned int width, unsigned int height);

	bool IsReady() const { return m_Initialized; }

	void BeginRender() const;
	void EndRender(unsigned int viewportWidth, unsigned int viewportHeight) const;

	void BindDepthTexture(unsigned int slot = 0) const;

	void SetLightProjectionOrtho(float left, float right, float bottom, float top,
								 float nearPlane, float farPlane);
	void UpdateLightView(const glm::vec3& lightPosition,
						 const glm::vec3& target,
						 const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 GetLightProjection() const { return m_LightProjection; }
	glm::mat4 GetLightView() const { return m_LightView; }
	glm::mat4 GetLightSpaceMatrix() const;

	unsigned int GetDepthTextureID() const { return m_DepthMap; }
	unsigned int GetWidth() const { return m_Width; }
	unsigned int GetHeight() const { return m_Height; }

private:
	bool CreateDepthTexture();
	void ReleaseDepthTexture();

private:
	Framebuffer m_Framebuffer;
	unsigned int m_DepthMap;
	unsigned int m_Width;
	unsigned int m_Height;
	bool m_Initialized;

	glm::mat4 m_LightProjection;
	glm::mat4 m_LightView;
};

#endif
