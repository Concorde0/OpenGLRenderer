#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Framebuffer.h"
#include "Light.h"
#include "Shader.h"

class DeferredRenderer {
public:
    DeferredRenderer(int width,
                     int height,
                     const char* geometryVertexPath,
                     const char* geometryFragmentPath,
                     const char* lightingVertexPath,
                     const char* lightingFragmentPath);
    ~DeferredRenderer();

    DeferredRenderer(const DeferredRenderer&) = delete;
    DeferredRenderer& operator=(const DeferredRenderer&) = delete;

    bool IsReady() const { return m_IsReady; }

    void Resize(int width, int height);

    void BeginGeometryPass(const glm::mat4& view, const glm::mat4& projection);
    void SetGeometryMaterial(const glm::vec3& albedo, float metallic, float roughness, float ao);
    void SetGeometryAlbedoTexture(unsigned int textureID, bool enabled);

    Shader& GetGeometryShader() { return m_GeometryShader; }
    const Shader& GetGeometryShader() const { return m_GeometryShader; }

    void RenderLightingPass(const glm::vec3& viewPos, const Light& light);

private:
    bool CreateGBuffer();
    void DestroyGBuffer();
    void SetupFullscreenQuad();

    int m_Width;
    int m_Height;
    bool m_IsReady;

    Framebuffer m_GBuffer;
    unsigned int m_GPosition;
    unsigned int m_GNormal;
    unsigned int m_GAlbedo;
    unsigned int m_GMaterial;
    unsigned int m_DepthRBO;

    unsigned int m_QuadVAO;
    unsigned int m_QuadVBO;

    Shader m_GeometryShader;
    Shader m_LightingShader;
};

#endif
