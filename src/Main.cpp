#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "../include/Window.h"
#include "../include/Shader.h"
#include "../include/Texture.h"
#include "../include/Camera.h"
#include "../include/Light.h"
#include "../include/ShadowMap.h"
#include "../include/VertexArray.h"
#include "../include/VertexBuffer.h"
#include "../include/IndexBuffer.h"

extern Camera camera;
extern float deltaTime;
extern float lastFrame;

// 为非索引三角形网格生成切线（输入布局：pos3 + normal3 + uv2）
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

int main()
{
    // 初始化窗口
    Window win;
    if (!win.Initialize()) {
        return -1;
    }

    // 捕获鼠标
    glfwSetInputMode(win.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 加载着色器文件
    Shader shader("shaders/phong.vert", "shaders/phong.frag");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");
    Shader depthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");

    // ---------- 立方体（每顶点：位置3 + 法线3 + 纹理2）----------
    float cubeVertsBase[] = {
        // back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
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
    const std::vector<float> cubeVerts = BuildTangentVerticesFromTriangleList(
        cubeVertsBase,
        sizeof(cubeVertsBase) / sizeof(float));

    constexpr int meshStride = 11 * sizeof(float);
    VertexArray cubeVAO;
    cubeVAO.Bind();
    VertexBuffer cubeVBO(cubeVerts.data(), (unsigned int)(cubeVerts.size() * sizeof(float)));
    cubeVAO.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)0);
    cubeVAO.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)(3 * sizeof(float)));
    cubeVAO.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, meshStride, (void*)(6 * sizeof(float)));
    cubeVAO.AddAttribute(3, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)(8 * sizeof(float)));

    // ---------- 地面平面（阴影接收面）----------
    float planeVertsBase[] = {
        // positions            // normals         // texcoords
         6.0f, -0.5f,  6.0f,    0.0f, 1.0f, 0.0f,  6.0f, 0.0f,
        -6.0f, -0.5f,  6.0f,    0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        -6.0f, -0.5f, -6.0f,    0.0f, 1.0f, 0.0f,  0.0f, 6.0f,

         6.0f, -0.5f,  6.0f,    0.0f, 1.0f, 0.0f,  6.0f, 0.0f,
        -6.0f, -0.5f, -6.0f,    0.0f, 1.0f, 0.0f,  0.0f, 6.0f,
         6.0f, -0.5f, -6.0f,    0.0f, 1.0f, 0.0f,  6.0f, 6.0f,
    };
    const std::vector<float> planeVerts = BuildTangentVerticesFromTriangleList(
        planeVertsBase,
        sizeof(planeVertsBase) / sizeof(float));

    VertexArray planeVAO;
    planeVAO.Bind();
    VertexBuffer planeVBO(planeVerts.data(), (unsigned int)(planeVerts.size() * sizeof(float)));
    planeVAO.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)0);
    planeVAO.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)(3 * sizeof(float)));
    planeVAO.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, meshStride, (void*)(6 * sizeof(float)));
    planeVAO.AddAttribute(3, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)(8 * sizeof(float)));

    // ---------- 球体（UV球，32×32 分段）----------
    std::vector<float>        sphereVerts;
    std::vector<unsigned int> sphereIdxs;
    GenerateSphere(0.5f, 32, 32, sphereVerts, sphereIdxs);
    VertexArray  sphereVAO;
    sphereVAO.Bind();
    VertexBuffer sphereVBO(sphereVerts.data(), (unsigned int)(sphereVerts.size() * sizeof(float)));
    IndexBuffer  sphereEBO(sphereIdxs.data(), (unsigned int)sphereIdxs.size());
    sphereVAO.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)0);
    sphereVAO.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)(3 * sizeof(float)));
    sphereVAO.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, meshStride, (void*)(6 * sizeof(float)));
    sphereVAO.AddAttribute(3, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)(8 * sizeof(float)));


    // ---------- 纹理 ----------
    Texture diffuseMap("assets/brickwall.jpg");
    if (!diffuseMap.IsLoaded()) { std::cerr << "无法加载漫反射贴图。\n"; return -1; }
    diffuseMap.SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::Repeat);
    diffuseMap.SetFilterMode(TextureFilterMode::LinearMipmapLinear, TextureFilterMode::Linear);

    Texture normalMap("assets/brickwall_normal.jpg");
    if (!normalMap.IsLoaded()) { std::cerr << "无法加载法线贴图。\n"; return -1; }
    normalMap.SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::Repeat);
    normalMap.SetFilterMode(TextureFilterMode::LinearMipmapLinear, TextureFilterMode::Linear);

    // 高光图
    Texture specularMap("assets/container_specular.png");
    if (!specularMap.IsLoaded()) { std::cerr << "无法加载高光贴图。\n"; return -1; }
    specularMap.SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::Repeat);
    specularMap.SetFilterMode(TextureFilterMode::LinearMipmapLinear, TextureFilterMode::Linear);

    // ---------- 光照 ----------
    Light sceneLight(glm::vec3(2.0f, 1.5f, 2.0f));
    sceneLight.SetAmbient(glm::vec3(0.15f, 0.15f, 0.15f));
    sceneLight.SetDiffuse(glm::vec3(0.85f, 0.85f, 0.85f));
    sceneLight.SetSpecular(glm::vec3(1.0f, 1.0f, 1.0f));

    const glm::vec3 dirLightDirection(-0.2f, -1.0f, -0.3f);

    ShadowMap shadowMap(2048, 2048);
    if (!shadowMap.IsReady()) {
        std::cerr << "阴影贴图初始化失败。\n";
        return -1;
    }

    shadowMap.SetLightProjectionOrtho(
        -8.0f, 8.0f,
        -8.0f, 8.0f,
        1.0f, 20.0f);

    shader.Use();
    shader.SetInt("texture_diffuse",  0);
    shader.SetInt("texture_specular", 1);
    shader.SetInt("shadowMap", 2);
    shader.SetInt("texture_normal", 3);
    shader.SetFloat("shadowBiasSlope", 0.005f);
    shader.SetFloat("shadowBiasMin", 0.0005);
    shader.SetInt("shadowPcfRadius", 1);

    // 材质（Ka/Kd/Ks + shininess）
    shader.SetVec3("material.ka", glm::vec3(0.35f, 0.35f, 0.35f));
    shader.SetVec3("material.kd", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.SetVec3("material.ks", glm::vec3(0.65f, 0.65f, 0.65f));
    shader.SetFloat("material.shininess", 32.0f);

    // 点光源（复用现有 Light 数据）
    sceneLight.Apply(shader, "pointLight");
    shader.SetFloat("pointLight.constant", 1.0f);
    shader.SetFloat("pointLight.linear", 0.09f);
    shader.SetFloat("pointLight.quadratic", 0.032f);

    // 方向光
    shader.SetVec3("dirLight.direction", dirLightDirection);
    shader.SetVec3("dirLight.ambient", glm::vec3(0.06f, 0.06f, 0.07f));
    shader.SetVec3("dirLight.diffuse", glm::vec3(0.28f, 0.28f, 0.30f));
    shader.SetVec3("dirLight.specular", glm::vec3(0.35f, 0.35f, 0.40f));

    // 聚光灯
    shader.SetVec3("spotLight.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
    shader.SetVec3("spotLight.diffuse", glm::vec3(0.90f, 0.90f, 0.90f));
    shader.SetVec3("spotLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.SetFloat("spotLight.cutOff", cosf(glm::radians(12.5f)));
    shader.SetFloat("spotLight.outerCutOff", cosf(glm::radians(17.5f)));
    shader.SetFloat("spotLight.constant", 1.0f);
    shader.SetFloat("spotLight.linear", 0.09f);
    shader.SetFloat("spotLight.quadratic", 0.032f);

    lampShader.Use();
    lampShader.SetVec3("lightColor", sceneLight.GetSpecular());


    auto RenderSceneMeshes = [&](Shader& activeShader, float currentFrame) {
        // 地面平面（receiver）
        glm::mat4 modelPlane(1.0f);
        activeShader.SetMat4("model", modelPlane);
        planeVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 立方体
        glm::mat4 modelCube = glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.0f, 0.0f));
        modelCube = glm::rotate(modelCube, currentFrame, glm::vec3(0.5f, 1.0f, 0.0f));
        activeShader.SetMat4("model", modelCube);
        cubeVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 球体
        glm::mat4 modelSphere = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.0f, 0.0f));
        modelSphere = glm::rotate(modelSphere, currentFrame * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
        activeShader.SetMat4("model", modelSphere);
        sphereVAO.Bind();
        glDrawElements(GL_TRIANGLES, sphereEBO.GetCount(), GL_UNSIGNED_INT, 0);
    };

    // ---------- 渲染循环 ----------
    while (!win.ShouldClose())
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        win.ProcessInput();

        int framebufferWidth = static_cast<int>(Window::SCR_WIDTH);
        int framebufferHeight = static_cast<int>(Window::SCR_HEIGHT);
        glfwGetFramebufferSize(win.GetWindow(), &framebufferWidth, &framebufferHeight);
        if (framebufferHeight <= 0) {
            framebufferHeight = 1;
        }

        // Shadow Pass: 从平行光方向渲染深度贴图
        const glm::vec3 lightDirN = glm::normalize(dirLightDirection);
        const glm::vec3 virtualLightPos = -lightDirN * 8.0f;
        shadowMap.UpdateLightView(virtualLightPos, glm::vec3(0.0f));
        const glm::mat4 lightSpaceMatrix = shadowMap.GetLightSpaceMatrix();

        shadowMap.BeginRender();
        depthShader.Use();
        depthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        RenderSceneMeshes(depthShader, currentFrame);
        shadowMap.EndRender(static_cast<unsigned int>(framebufferWidth), static_cast<unsigned int>(framebufferHeight));

        // Lighting Pass: 主渲染阶段采样 shadow map
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();
        diffuseMap.Bind(0);
        specularMap.Bind(1);
        shadowMap.BindDepthTexture(2);
        normalMap.Bind(3);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight),
            0.1f, 100.0f);
        shader.SetMat4("view", view);
        shader.SetMat4("projection", projection);
        shader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        shader.SetVec3("viewPos", camera.Position);
        shader.SetFloat("shadowBiasSlope", 0.05f);
        shader.SetFloat("shadowBiasMin", 0.0005f);
        shader.SetInt("shadowPcfRadius", 1);

        // 点光源位置/颜色上传
        sceneLight.Apply(shader, "pointLight");

        // 聚光灯跟随摄像机
        shader.SetVec3("spotLight.position", camera.Position);
        shader.SetVec3("spotLight.direction", camera.Front);

        RenderSceneMeshes(shader, currentFrame);

        // 光源小立方体（lamp）
        lampShader.Use();
        lampShader.SetMat4("view", view);
        lampShader.SetMat4("projection", projection);
        glm::mat4 lampModel = glm::translate(glm::mat4(1.0f), sceneLight.GetPosition());
        lampModel = glm::scale(lampModel, glm::vec3(0.15f));
        lampShader.SetMat4("model", lampModel);
        cubeVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);

        win.SwapBuffers();
        win.PollEvents();
    }

    return 0;
}
