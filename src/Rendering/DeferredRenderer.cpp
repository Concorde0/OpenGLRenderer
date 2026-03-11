#include "../../include/DeferredRenderer.h"

#include <array>
#include <iostream>

DeferredRenderer::DeferredRenderer(int width,
                                   int height,
                                   const char* geometryVertexPath,
                                   const char* geometryFragmentPath,
                                   const char* lightingVertexPath,
                                   const char* lightingFragmentPath)
    : m_Width(width),
      m_Height(height),
      m_IsReady(false),
      m_GPosition(0),
      m_GNormal(0),
      m_GAlbedo(0),
      m_GMaterial(0),
      m_DepthRBO(0),
      m_QuadVAO(0),
      m_QuadVBO(0),
      m_GeometryShader(geometryVertexPath, geometryFragmentPath),
      m_LightingShader(lightingVertexPath, lightingFragmentPath)
{
    SetupFullscreenQuad();
    m_IsReady = CreateGBuffer();

    m_LightingShader.Use();
    m_LightingShader.SetInt("gPosition", 0);
    m_LightingShader.SetInt("gNormal", 1);
    m_LightingShader.SetInt("gAlbedo", 2);
    m_LightingShader.SetInt("gMaterial", 3);

    SetGeometryMaterial(glm::vec3(1.0f), 0.10f, 0.45f, 1.0f);
    SetGeometryAlbedoTexture(0, false);
}

DeferredRenderer::~DeferredRenderer()
{
    DestroyGBuffer();

    if (m_QuadVBO != 0) {
        glDeleteBuffers(1, &m_QuadVBO);
    }
    if (m_QuadVAO != 0) {
        glDeleteVertexArrays(1, &m_QuadVAO);
    }
}

void DeferredRenderer::Resize(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    if (width == m_Width && height == m_Height) {
        return;
    }

    m_Width = width;
    m_Height = height;

    DestroyGBuffer();
    m_IsReady = CreateGBuffer();
}

void DeferredRenderer::BeginGeometryPass(const glm::mat4& view, const glm::mat4& projection)
{
    m_GBuffer.Bind();
    glViewport(0, 0, m_Width, m_Height);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_GeometryShader.Use();
    m_GeometryShader.SetMat4("view", view);
    m_GeometryShader.SetMat4("projection", projection);
}

void DeferredRenderer::SetGeometryMaterial(const glm::vec3& albedo, float metallic, float roughness, float ao)
{
    m_GeometryShader.Use();
    m_GeometryShader.SetVec3("albedoColor", albedo);
    m_GeometryShader.SetFloat("metallic", metallic);
    m_GeometryShader.SetFloat("roughness", roughness);
    m_GeometryShader.SetFloat("ao", ao);
}

void DeferredRenderer::SetGeometryAlbedoTexture(unsigned int textureID, bool enabled)
{
    m_GeometryShader.Use();
    m_GeometryShader.SetBool("useAlbedoMap", enabled);
    m_GeometryShader.SetInt("albedoMap", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, enabled ? textureID : 0);
}

void DeferredRenderer::RenderLightingPass(const glm::vec3& viewPos, const Light& light, float lightIntensity)
{
    Framebuffer::Unbind();
    glViewport(0, 0, m_Width, m_Height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    m_LightingShader.Use();
    m_LightingShader.SetVec3("viewPos", viewPos);
    m_LightingShader.SetVec3("light.position", light.GetPosition());
    m_LightingShader.SetVec3("light.color", light.GetDiffuse());
    m_LightingShader.SetFloat("light.intensity", lightIntensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_GPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_GNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_GAlbedo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_GMaterial);

    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

bool DeferredRenderer::CreateGBuffer()
{
    if (m_Width <= 0 || m_Height <= 0) {
        std::cerr << "[DeferredRenderer] Invalid G-Buffer size." << std::endl;
        return false;
    }

    m_GBuffer.Bind();

    glGenTextures(1, &m_GPosition);
    glBindTexture(GL_TEXTURE_2D, m_GPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_GBuffer.AttachTexture2D(GL_COLOR_ATTACHMENT0, m_GPosition);

    glGenTextures(1, &m_GNormal);
    glBindTexture(GL_TEXTURE_2D, m_GNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_GBuffer.AttachTexture2D(GL_COLOR_ATTACHMENT1, m_GNormal);

    glGenTextures(1, &m_GAlbedo);
    glBindTexture(GL_TEXTURE_2D, m_GAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_GBuffer.AttachTexture2D(GL_COLOR_ATTACHMENT2, m_GAlbedo);

    glGenTextures(1, &m_GMaterial);
    glBindTexture(GL_TEXTURE_2D, m_GMaterial);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_GBuffer.AttachTexture2D(GL_COLOR_ATTACHMENT3, m_GMaterial);

    const std::array<GLenum, 4> attachments = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3
    };
    glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data());

    glGenRenderbuffers(1, &m_DepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_Width, m_Height);
    m_GBuffer.AttachRenderbuffer(GL_DEPTH_ATTACHMENT, m_DepthRBO);

    const bool complete = m_GBuffer.IsComplete();
    Framebuffer::Unbind();
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (!complete) {
        std::cerr << "[DeferredRenderer] G-Buffer creation failed." << std::endl;
    }

    return complete;
}

void DeferredRenderer::DestroyGBuffer()
{
    if (m_GPosition != 0) {
        glDeleteTextures(1, &m_GPosition);
        m_GPosition = 0;
    }
    if (m_GNormal != 0) {
        glDeleteTextures(1, &m_GNormal);
        m_GNormal = 0;
    }
    if (m_GAlbedo != 0) {
        glDeleteTextures(1, &m_GAlbedo);
        m_GAlbedo = 0;
    }
    if (m_GMaterial != 0) {
        glDeleteTextures(1, &m_GMaterial);
        m_GMaterial = 0;
    }
    if (m_DepthRBO != 0) {
        glDeleteRenderbuffers(1, &m_DepthRBO);
        m_DepthRBO = 0;
    }
}

void DeferredRenderer::SetupFullscreenQuad()
{
    const float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);

    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}
