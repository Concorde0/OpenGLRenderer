#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include "../include/Window.h"
#include "../include/Shader.h"
#include "../include/Texture.h"
#include "../include/Camera.h"
#include "../include/Light.h"
#include "../include/VertexArray.h"
#include "../include/VertexBuffer.h"
#include "../include/IndexBuffer.h"

extern Camera camera;
extern float deltaTime;
extern float lastFrame;

// 生成 UV 球体顶点数据（每顶点：位置xyz + 法线xyz + 纹理坐标uv）
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

            verts.push_back(x);
            verts.push_back(y);
            verts.push_back(z);
            verts.push_back(nx);
            verts.push_back(ny);
            verts.push_back(nz);
            verts.push_back((float)j / sectors);
            verts.push_back((float)i / stacks);
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

    // ---------- 立方体（每顶点：位置3 + 法线3 + 纹理2）----------
    float cubeVerts[] = {
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
    constexpr int meshStride = 8 * sizeof(float);
    VertexArray cubeVAO;
    cubeVAO.Bind();
    VertexBuffer cubeVBO(cubeVerts, sizeof(cubeVerts));
    cubeVAO.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)0);
    cubeVAO.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)(3 * sizeof(float)));
    cubeVAO.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, meshStride, (void*)(6 * sizeof(float)));

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


    // ---------- 纹理 ----------
    Texture diffuseMap("assets/container2.png");
    if (!diffuseMap.IsLoaded()) { std::cerr << "无法加载漫反射贴图。\n"; return -1; }
    diffuseMap.SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::Repeat);
    diffuseMap.SetFilterMode(TextureFilterMode::LinearMipmapLinear, TextureFilterMode::Linear);

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

    shader.Use();
    shader.SetInt("texture_diffuse",  0);
    shader.SetInt("texture_specular", 1);

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
    shader.SetVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
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

    // ---------- 渲染循环 ----------
    while (!win.ShouldClose())
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        win.ProcessInput();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();
        diffuseMap.Bind(0);
        specularMap.Bind(1);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)Window::SCR_WIDTH / (float)Window::SCR_HEIGHT,
            0.1f, 100.0f);
        shader.SetMat4("view", view);
        shader.SetMat4("projection", projection);
        shader.SetVec3("viewPos", camera.Position);

        // 点光源位置/颜色上传
        sceneLight.Apply(shader, "pointLight");

        // 聚光灯跟随摄像机
        shader.SetVec3("spotLight.position", camera.Position);
        shader.SetVec3("spotLight.direction", camera.Front);

        // 立方体
        glm::mat4 modelCube = glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.0f, 0.0f));
        modelCube = glm::rotate(modelCube, currentFrame, glm::vec3(0.5f, 1.0f, 0.0f));
        shader.SetMat4("model", modelCube);
        cubeVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 球体
        glm::mat4 modelSphere = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.0f, 0.0f));
        modelSphere = glm::rotate(modelSphere, currentFrame * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
        shader.SetMat4("model", modelSphere);
        sphereVAO.Bind();
        glDrawElements(GL_TRIANGLES, sphereEBO.GetCount(), GL_UNSIGNED_INT, 0);

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
