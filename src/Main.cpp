#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <memory>
#include <vector>
#include "../include/Window.h"
#include "../include/Shader.h"
#include "../include/Texture.h"
#include "../include/PBRMaterial.h"
#include "../include/Camera.h"
#include "../include/DeferredRenderer.h"
#include "../include/Light.h"
#include "../include/VertexArray.h"
#include "../include/VertexBuffer.h"
#include "../include/IndexBuffer.h"
#include "../include/Model.h"
#include "../include/SceneNode.h"

extern Camera camera;
extern float deltaTime;
extern float lastFrame;

static std::vector<float> BuildTangentVerticesFromTriangleList(const float* src, size_t floatCount);

namespace {

constexpr int kMeshStrideFloats = 11;
constexpr int kMeshStrideBytes = kMeshStrideFloats * sizeof(float);

enum class RenderMode {
    Forward,
    Deferred
};

struct PerformanceStats {
    double totalMs = 0.0;
    int samples = 0;
    double averageMs = 0.0;

    void Add(double sampleMs)
    {
        totalMs += sampleMs;
        ++samples;

        if (samples >= 120) {
            averageMs = totalMs / static_cast<double>(samples);
            totalMs = 0.0;
            samples = 0;
        }
    }

    double CurrentAverage() const
    {
        if (samples > 0) {
            return totalMs / static_cast<double>(samples);
        }
        return averageMs;
    }
};

static const char* RenderModeToString(RenderMode mode)
{
    return mode == RenderMode::Forward ? "Forward" : "Deferred";
}

struct PBRTextureAtlas {
    std::unique_ptr<Texture> albedo;
    std::unique_ptr<Texture> metallic;
    std::unique_ptr<Texture> normal;
    std::unique_ptr<Texture> roughness;

    bool HasAnyMap() const
    {
        return albedo != nullptr || metallic != nullptr || normal != nullptr || roughness != nullptr;
    }
};

static bool FileExists(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

static void ConfigureSurfaceTexture(Texture& texture)
{
    texture.SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::Repeat);
    texture.SetFilterMode(TextureFilterMode::LinearMipmapLinear, TextureFilterMode::Linear);
}

static bool ValidateRequiredTexture(Texture& texture, const char* description)
{
    if (!texture.IsLoaded()) {
        std::cerr << "无法加载" << description << "。\n";
        return false;
    }

    ConfigureSurfaceTexture(texture);
    return true;
}

static std::vector<std::string> BuildAtlasCandidates(const std::string& atlasName,
                                                     std::initializer_list<const char*> suffixes)
{
    static const std::array<const char*, 3> kExtensions = { ".png", ".jpg", ".jpeg" };
    std::vector<std::string> candidates;

    for (const char* suffix : suffixes) {
        for (const char* extension : kExtensions) {
            candidates.emplace_back(std::string("assets/") + atlasName + "_" + suffix + extension);
        }
    }

    return candidates;
}

static std::unique_ptr<Texture> LoadOptionalTextureFromCandidates(const std::vector<std::string>& candidates,
                                                                  const char* mapLabel)
{
    for (const std::string& path : candidates) {
        if (!FileExists(path)) {
            continue;
        }

        std::unique_ptr<Texture> texture = std::make_unique<Texture>(path);
        if (texture->IsLoaded()) {
            ConfigureSurfaceTexture(*texture);
            std::cout << "[Atlas] Loaded " << mapLabel << " map: " << path << std::endl;
            return texture;
        }
    }

    std::cout << "[Atlas] " << mapLabel << " map is missing, fallback will be used." << std::endl;
    return nullptr;
}

static PBRTextureAtlas LoadPBRTextureAtlas(const std::string& atlasName)
{
    PBRTextureAtlas atlas;

    atlas.albedo = LoadOptionalTextureFromCandidates(
        BuildAtlasCandidates(atlasName, { "basecolor", "albedo", "diffuse", "color" }),
        "albedo");
    atlas.metallic = LoadOptionalTextureFromCandidates(
        BuildAtlasCandidates(atlasName, { "metallic", "metalness", "metal" }),
        "metallic");
    atlas.normal = LoadOptionalTextureFromCandidates(
        BuildAtlasCandidates(atlasName, { "normal", "normalgl", "nrm" }),
        "normal");
    atlas.roughness = LoadOptionalTextureFromCandidates(
        BuildAtlasCandidates(atlasName, { "roughness", "rough", "glossiness" }),
        "roughness");

    if (!atlas.HasAnyMap()) {
        std::cout << "[Atlas] No dedicated PBR atlas maps were found for " << atlasName
                  << ", using fallback textures." << std::endl;
    }

    return atlas;
}

static void ConfigurePBRMaterial(PBRMaterial& material,
                                 const PBRTextureAtlas& atlas,
                                 const Texture& fallbackDiffuse,
                                 const Texture& fallbackNormal,
                                 const Texture& fallbackAO)
{
    material.SetAlbedoMap(atlas.albedo ? atlas.albedo.get() : &fallbackDiffuse);
    material.SetNormalMap(atlas.normal ? atlas.normal.get() : &fallbackNormal);

    if (atlas.metallic) {
        material.SetMetallicMap(atlas.metallic.get());
        material.SetMetallicFactor(1.0f);
    }
    else {
        material.SetMetallicFactor(0.10f);
    }

    if (atlas.roughness) {
        material.SetRoughnessMap(atlas.roughness.get());
        material.SetRoughnessFactor(1.0f);
    }
    else {
        material.SetRoughnessFactor(0.45f);
    }

    material.SetAOMap(&fallbackAO);
    material.SetAlbedoFactor(glm::vec3(1.0f));
    material.SetAOFactor(1.0f);
}

static void ConfigureMeshAttributes(VertexArray& vao, VertexBuffer& vbo)
{
    vao.Bind();
    vbo.Bind();
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, kMeshStrideBytes, (void*)0);
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, kMeshStrideBytes, (void*)(3 * sizeof(float)));
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, kMeshStrideBytes, (void*)(6 * sizeof(float)));
    vao.AddAttribute(3, 3, GL_FLOAT, GL_FALSE, kMeshStrideBytes, (void*)(8 * sizeof(float)));
}

static std::vector<float> CreateCubeVerticesWithTangents()
{
    float cubeVertsBase[] = {
        // back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

        // front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        // left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        // bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        // top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    };

    return BuildTangentVerticesFromTriangleList(cubeVertsBase, sizeof(cubeVertsBase) / sizeof(float));
}

static std::vector<float> CreatePlaneVerticesWithTangents()
{
    float planeVertsBase[] = {
        // positions            // normals         // texcoords
         6.0f, -0.5f,  6.0f,    0.0f, 1.0f, 0.0f,  6.0f, 0.0f,
        -6.0f, -0.5f,  6.0f,    0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        -6.0f, -0.5f, -6.0f,    0.0f, 1.0f, 0.0f,  0.0f, 6.0f,

         6.0f, -0.5f,  6.0f,    0.0f, 1.0f, 0.0f,  6.0f, 0.0f,
        -6.0f, -0.5f, -6.0f,    0.0f, 1.0f, 0.0f,  0.0f, 6.0f,
         6.0f, -0.5f, -6.0f,    0.0f, 1.0f, 0.0f,  6.0f, 6.0f,
    };

    return BuildTangentVerticesFromTriangleList(planeVertsBase, sizeof(planeVertsBase) / sizeof(float));
}

static void ConfigurePBRShaderStaticUniforms(Shader& pbrShader,
                                             const Light& sceneLight,
                                             const glm::vec3& dirLightDirection)
{
    pbrShader.Use();
    pbrShader.SetInt("irradianceMap", 10);
    pbrShader.SetInt("prefilterMap", 11);
    pbrShader.SetInt("brdfLUT", 12);
    pbrShader.SetBool("enableIBL", true);

    pbrShader.SetVec3("dirLight.direction", dirLightDirection);
    pbrShader.SetVec3("dirLight.color", glm::vec3(1.0f, 0.98f, 0.95f));
    pbrShader.SetFloat("dirLight.intensity", 1.1f);

    pbrShader.SetFloat("pointLight.constant", 1.0f);
    pbrShader.SetFloat("pointLight.linear", 0.09f);
    pbrShader.SetFloat("pointLight.quadratic", 0.032f);
    pbrShader.SetVec3("pointLight.color", sceneLight.GetDiffuse());
    pbrShader.SetFloat("pointLight.intensity", 5.0f);

    pbrShader.SetFloat("spotLight.cutOff", cosf(glm::radians(12.5f)));
    pbrShader.SetFloat("spotLight.outerCutOff", cosf(glm::radians(17.5f)));
    pbrShader.SetFloat("spotLight.constant", 1.0f);
    pbrShader.SetFloat("spotLight.linear", 0.09f);
    pbrShader.SetFloat("spotLight.quadratic", 0.032f);
    pbrShader.SetVec3("spotLight.color", glm::vec3(1.0f));
    pbrShader.SetFloat("spotLight.intensity", 3.0f);
}

static void BindIBLTextures(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUT)
{
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, brdfLUT);
}

static void RenderSceneGeometry(Shader& shader,
                                const VertexArray& planeVAO,
                                const VertexArray& cubeVAO,
                                const VertexArray& sphereVAO,
                                const IndexBuffer& sphereEBO,
                                float currentFrame)
{
    shader.SetBool("useInstancing", false);

    glm::mat4 modelPlane(1.0f);
    shader.SetMat4("model", modelPlane);
    planeVAO.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glm::mat4 modelCube = glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.0f, 0.0f));
    modelCube = glm::rotate(modelCube, currentFrame, glm::vec3(0.5f, 1.0f, 0.0f));
    shader.SetMat4("model", modelCube);
    cubeVAO.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glm::mat4 modelSphere = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.0f, 0.0f));
    modelSphere = glm::rotate(modelSphere, currentFrame * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
    shader.SetMat4("model", modelSphere);
    sphereVAO.Bind();
    glDrawElements(GL_TRIANGLES, sphereEBO.GetCount(), GL_UNSIGNED_INT, 0);
}

static void RenderLamp(Shader& lampShader,
                       const VertexArray& cubeVAO,
                       const Light& sceneLight,
                       const glm::mat4& view,
                       const glm::mat4& projection)
{
    lampShader.Use();
    lampShader.SetMat4("view", view);
    lampShader.SetMat4("projection", projection);

    glm::mat4 lampModel = glm::translate(glm::mat4(1.0f), sceneLight.GetPosition());
    lampModel = glm::scale(lampModel, glm::vec3(0.15f));
    lampShader.SetMat4("model", lampModel);

    cubeVAO.Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

static std::vector<glm::mat4> BuildInstanceTransforms(int count, float radius, float yOffset)
{
    std::vector<glm::mat4> transforms;
    if (count <= 0) {
        return transforms;
    }

    transforms.reserve(static_cast<std::size_t>(count));
    const float twoPi = 6.28318530718f;

    for (int i = 0; i < count; ++i) {
        float angle = twoPi * static_cast<float>(i) / static_cast<float>(count);
        glm::vec3 position(
            radius * std::cos(angle),
            yOffset,
            radius * std::sin(angle));

        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::rotate(model, angle + 0.25f, glm::vec3(0.0f, 1.0f, 0.0f));

        float scale = 0.18f + 0.06f * std::sin(angle * 3.0f);
        model = glm::scale(model, glm::vec3(scale));

        transforms.push_back(model);
    }

    return transforms;
}

} // namespace

// 为非索引三角形网格生成切线（ pos3 + normal3 + uv2）
static std::vector<float> BuildTangentVerticesFromTriangleList(const float* src, size_t floatCount)
{
    constexpr size_t srcStride = 8;
    constexpr size_t dstStride = 11;
    std::vector<float> out;
    if (src == nullptr || floatCount == 0 || (floatCount % (srcStride * 3)) != 0) {
        return out;
    }

    out.reserve((floatCount / srcStride) * dstStride);

    for (size_t i = 0; i < floatCount; i += srcStride * 3) {
        const float* v0 = src + i;
        const float* v1 = src + i + srcStride;
        const float* v2 = src + i + srcStride * 2;

        glm::vec3 p0(v0[0], v0[1], v0[2]);
        glm::vec3 p1(v1[0], v1[1], v1[2]);
        glm::vec3 p2(v2[0], v2[1], v2[2]);
        glm::vec2 uv0(v0[6], v0[7]);
        glm::vec2 uv1(v1[6], v1[7]);
        glm::vec2 uv2(v2[6], v2[7]);

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        float det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        glm::vec3 tangent(1.0f, 0.0f, 0.0f);
        if (fabsf(det) > 1e-8f) {
            float invDet = 1.0f / det;
            tangent.x = invDet * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = invDet * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = invDet * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent = glm::normalize(tangent);
        }

        for (int v = 0; v < 3; ++v) {
            const float* srcV = src + i + srcStride * static_cast<size_t>(v);
            out.insert(out.end(), srcV, srcV + srcStride);
            out.push_back(tangent.x);
            out.push_back(tangent.y);
            out.push_back(tangent.z);
        }
    }

    return out;
}

// 生成 UV 球体顶点数据（每顶点：位置xyz + 法线xyz + 纹理坐标uv + 切线xyz）
static void GenerateSphere(float radius, int stacks, int sectors,
                           std::vector<float>& verts, std::vector<unsigned int>& idxs)
{
    const float PI = 3.14159265359f;
    for (int i = 0; i <= stacks; ++i) {
        float phi = PI / 2.0f - i * (PI / stacks);
        float y   = radius * sinf(phi);
        float xz  = radius * cosf(phi);
        for (int j = 0; j <= sectors; ++j) {
            float theta = j * (2.0f * PI / sectors);
            float x = xz * cosf(theta);
            float z = xz * sinf(theta);
            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;
            float tx = -sinf(theta);
            float ty = 0.0f;
            float tz = cosf(theta);

            verts.push_back(x);
            verts.push_back(y);
            verts.push_back(z);
            verts.push_back(nx);
            verts.push_back(ny);
            verts.push_back(nz);
            verts.push_back((float)j / sectors);
            verts.push_back((float)i / stacks);
            verts.push_back(tx);
            verts.push_back(ty);
            verts.push_back(tz);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            unsigned int k1 = i * (sectors + 1) + j;
            unsigned int k2 = k1 + sectors + 1;
            idxs.push_back(k1);     idxs.push_back(k2);     idxs.push_back(k1 + 1);
            idxs.push_back(k1 + 1); idxs.push_back(k2);     idxs.push_back(k2 + 1);
        }
    }
}

static unsigned int CreateSolidColorCubemap(const glm::vec3& color)
{
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    unsigned char pixel[3] = {
        static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f)
    };

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0,
                     GL_RGB,
                     1,
                     1,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     pixel);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return tex;
}

static unsigned int CreateFallbackBRDFLUT(int width = 128, int height = 128)
{
    std::vector<float> data(static_cast<size_t>(width) * static_cast<size_t>(height) * 2u);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float ndotV = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            float roughness = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
            size_t index = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 2u;

            data[index + 0] = ndotV;
            data[index + 1] = (1.0f - roughness) * 0.35f;
        }
    }

    unsigned int lut = 0;
    glGenTextures(1, &lut);
    glBindTexture(GL_TEXTURE_2D, lut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return lut;
}

int main()
{
    Window win;
    if (!win.Initialize()) {
        return -1;
    }

    glfwSetInputMode(win.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Shader pbrShader("shaders/pbr.vert", "shaders/pbr.frag");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");
    DeferredRenderer deferredRenderer(static_cast<int>(Window::SCR_WIDTH),
                                      static_cast<int>(Window::SCR_HEIGHT),
                                      "shaders/deferred_geometry.vert",
                                      "shaders/deferred_geometry.frag",
                                      "shaders/deferred_lighting.vert",
                                      "shaders/deferred_lighting.frag");
    if (!deferredRenderer.IsReady()) {
        std::cerr << "Deferred renderer initialization failed." << std::endl;
        return -1;
    }

    const std::vector<float> cubeVerts = CreateCubeVerticesWithTangents();
    VertexArray cubeVAO;
    VertexBuffer cubeVBO(cubeVerts.data(), (unsigned int)(cubeVerts.size() * sizeof(float)));
    ConfigureMeshAttributes(cubeVAO, cubeVBO);

    const std::vector<float> planeVerts = CreatePlaneVerticesWithTangents();
    VertexArray planeVAO;
    VertexBuffer planeVBO(planeVerts.data(), (unsigned int)(planeVerts.size() * sizeof(float)));
    ConfigureMeshAttributes(planeVAO, planeVBO);

    std::vector<float> sphereVerts;
    std::vector<unsigned int> sphereIdxs;
    GenerateSphere(0.5f, 32, 32, sphereVerts, sphereIdxs);
    VertexArray sphereVAO;
    VertexBuffer sphereVBO(sphereVerts.data(), (unsigned int)(sphereVerts.size() * sizeof(float)));
    IndexBuffer sphereEBO(sphereIdxs.data(), (unsigned int)sphereIdxs.size());
    ConfigureMeshAttributes(sphereVAO, sphereVBO);

    Model objSceneModel;
    if (!objSceneModel.LoadFromOBJ("assets/models/demo_scene.obj")) {
        std::cerr << "[Model] Falling back to procedural model." << std::endl;
        objSceneModel = Model::CreateFallbackModel();
    }

    SceneNode sceneRoot("Root");
    SceneNode* spinningNode = sceneRoot.CreateChild(
        "SpinningModel",
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.8f)),
        &objSceneModel);
    SceneNode* orbitNode = sceneRoot.CreateChild(
        "OrbitModel",
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.4f, 0.0f, 1.8f)),
        &objSceneModel);

    const bool enableInstancing = true;
    std::vector<glm::mat4> instanceTransforms = BuildInstanceTransforms(24, 4.6f, -0.2f);

    Texture diffuseMap("assets/brickwall.jpg");
    if (!ValidateRequiredTexture(diffuseMap, "漫反射贴图")) {
        return -1;
    }

    Texture normalMap("assets/brickwall_normal.jpg");
    if (!ValidateRequiredTexture(normalMap, "法线贴图")) {
        return -1;
    }

    Texture aoMap("assets/container_specular.png");
    if (!ValidateRequiredTexture(aoMap, "AO 贴图（暂用容器高光图）")) {
        return -1;
    }

    const PBRTextureAtlas atlas = LoadPBRTextureAtlas("rustediron2");

    PBRMaterial pbrMaterial;
    ConfigurePBRMaterial(pbrMaterial, atlas, diffuseMap, normalMap, aoMap);

    Light sceneLight(glm::vec3(2.0f, 1.5f, 2.0f));
    sceneLight.SetAmbient(glm::vec3(0.15f, 0.15f, 0.15f));
    sceneLight.SetDiffuse(glm::vec3(0.85f, 0.85f, 0.85f));
    sceneLight.SetSpecular(glm::vec3(1.0f, 1.0f, 1.0f));

    const glm::vec3 dirLightDirection(-0.2f, -1.0f, -0.3f);
    unsigned int irradianceMap = CreateSolidColorCubemap(glm::vec3(0.22f, 0.25f, 0.30f));
    unsigned int prefilterMap = CreateSolidColorCubemap(glm::vec3(0.45f, 0.47f, 0.50f));
    unsigned int brdfLUT = CreateFallbackBRDFLUT();

    ConfigurePBRShaderStaticUniforms(pbrShader, sceneLight, dirLightDirection);
    deferredRenderer.SetGeometryMaterial(glm::vec3(1.0f), 0.10f, 0.45f, 1.0f);

    lampShader.Use();
    lampShader.SetVec3("lightColor", sceneLight.GetSpecular());

    RenderMode renderMode = RenderMode::Deferred;
    bool f1PressedLastFrame = false;

    PerformanceStats forwardStats;
    PerformanceStats deferredStats;
    double lastPerfReport = glfwGetTime();

    std::cout << "Render mode: Deferred. Press F1 to toggle Forward/Deferred." << std::endl;

    while (!win.ShouldClose())
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        win.ProcessInput();

        int framebufferWidth = static_cast<int>(Window::SCR_WIDTH);
        int framebufferHeight = static_cast<int>(Window::SCR_HEIGHT);
        glfwGetFramebufferSize(win.GetWindow(), &framebufferWidth, &framebufferHeight);
        if (framebufferWidth <= 0) {
            framebufferWidth = 1;
        }
        if (framebufferHeight <= 0) {
            framebufferHeight = 1;
        }

        bool f1PressedNow = glfwGetKey(win.GetWindow(), GLFW_KEY_F1) == GLFW_PRESS;
        if (f1PressedNow && !f1PressedLastFrame) {
            renderMode = (renderMode == RenderMode::Forward) ? RenderMode::Deferred : RenderMode::Forward;
            std::cout << "Switched to " << RenderModeToString(renderMode) << " renderer." << std::endl;
        }
        f1PressedLastFrame = f1PressedNow;

        deferredRenderer.Resize(framebufferWidth, framebufferHeight);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight),
            0.1f, 100.0f);

        const auto renderStart = std::chrono::high_resolution_clock::now();

        glm::mat4 spinningTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.8f));
        spinningTransform = glm::rotate(spinningTransform, currentFrame * 0.45f, glm::vec3(0.0f, 1.0f, 0.0f));
        spinningNode->SetLocalTransform(spinningTransform);

        glm::mat4 orbitTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.4f, 0.0f, 1.8f));
        orbitTransform = glm::rotate(orbitTransform, -currentFrame * 0.30f, glm::vec3(0.0f, 1.0f, 0.0f));
        orbitTransform = glm::scale(orbitTransform, glm::vec3(0.8f));
        orbitNode->SetLocalTransform(orbitTransform);

        if (renderMode == RenderMode::Forward) {
            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            pbrShader.Use();
            pbrMaterial.Bind(pbrShader, "material", 0);
            BindIBLTextures(irradianceMap, prefilterMap, brdfLUT);

            pbrShader.SetMat4("view", view);
            pbrShader.SetMat4("projection", projection);
            pbrShader.SetVec3("camPos", camera.Position);

            pbrShader.SetVec3("pointLight.position", sceneLight.GetPosition());
            pbrShader.SetVec3("spotLight.position", camera.Position);
            pbrShader.SetVec3("spotLight.direction", camera.Front);

            RenderSceneGeometry(pbrShader, planeVAO, cubeVAO, sphereVAO, sphereEBO, currentFrame);
            sceneRoot.Draw(pbrShader);
            if (enableInstancing) {
                objSceneModel.DrawInstanced(pbrShader, instanceTransforms);
            }
            RenderLamp(lampShader, cubeVAO, sceneLight, view, projection);
        }
        else {
            deferredRenderer.SetGeometryAlbedoTexture(diffuseMap.GetID(), true);
            deferredRenderer.BeginGeometryPass(view, projection);
            RenderSceneGeometry(deferredRenderer.GetGeometryShader(),
                                planeVAO,
                                cubeVAO,
                                sphereVAO,
                                sphereEBO,
                                currentFrame);
            sceneRoot.Draw(deferredRenderer.GetGeometryShader());
            if (enableInstancing) {
                objSceneModel.DrawInstanced(deferredRenderer.GetGeometryShader(), instanceTransforms);
            }

            glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            deferredRenderer.RenderLightingPass(camera.Position, sceneLight);
        }

        glFinish();
        const auto renderEnd = std::chrono::high_resolution_clock::now();
        const double renderMs = std::chrono::duration<double, std::milli>(renderEnd - renderStart).count();

        if (renderMode == RenderMode::Forward) {
            forwardStats.Add(renderMs);
        }
        else {
            deferredStats.Add(renderMs);
        }

        const double now = glfwGetTime();
        if (now - lastPerfReport >= 1.0) {
            const double forwardAvg = forwardStats.CurrentAverage();
            const double deferredAvg = deferredStats.CurrentAverage();

            std::cout << std::fixed << std::setprecision(3)
                      << "[Perf] Forward: " << forwardAvg << " ms"
                      << " | Deferred: " << deferredAvg << " ms";

            if (forwardAvg > 0.0 && deferredAvg > 0.0) {
                const double speedup = forwardAvg / deferredAvg;
                std::cout << " | Ratio(F/D): " << speedup << "x";
            }

            std::cout << " | Mode: " << RenderModeToString(renderMode) << std::endl;
            lastPerfReport = now;
        }

        win.SwapBuffers();
        win.PollEvents();
    }

    glDeleteTextures(1, &irradianceMap);
    glDeleteTextures(1, &prefilterMap);
    glDeleteTextures(1, &brdfLUT);

    return 0;
}
