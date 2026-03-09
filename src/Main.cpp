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
#include "../include/VertexArray.h"
#include "../include/VertexBuffer.h"
#include "../include/IndexBuffer.h"

extern Camera camera;
extern float deltaTime;
extern float lastFrame;

// 生成 UV 球体顶点数据（格式：位置xyz + 纹理坐标uv，每顶点 5 个浮点数）
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
            verts.push_back(xz * cosf(theta));    // x
            verts.push_back(y);                    // y
            verts.push_back(xz * sinf(theta));    // z
            verts.push_back((float)j / sectors);  // u
            verts.push_back((float)i / stacks);   // v
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

    // 顶点着色器源码
    const std::string vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        
        out vec2 TexCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    // 片段着色器源码（漫反射 + 高光贴图）
    const std::string fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec2 TexCoord;
        
        uniform sampler2D texture_diffuse;   // 漫反射贴图（slot 0）
        uniform sampler2D texture_specular;  // 高光贴图（slot 1）
        
        void main()
        {
            vec4 diffuse  = texture(texture_diffuse,  TexCoord);
            vec4 specular = texture(texture_specular, TexCoord);
            // 叠加：漫反射颜色 + 25% 高光强度
            FragColor = diffuse + specular * 0.25;
        }
    )";

    // 创建着色器（使用Shader类）
    Shader shader(vertexShaderSource, fragmentShaderSource, true);

    // ---------- 立方体（36顶点，6面×2三角形×3顶点，格式：位置(3)+纹理坐标(2)）----------
    float cubeVerts[] = {
        // 前面
        -0.5f,-0.5f, 0.5f, 0,0,   0.5f,-0.5f, 0.5f, 1,0,   0.5f, 0.5f, 0.5f, 1,1,
         0.5f, 0.5f, 0.5f, 1,1,  -0.5f, 0.5f, 0.5f, 0,1,  -0.5f,-0.5f, 0.5f, 0,0,
        // 后面
        -0.5f,-0.5f,-0.5f, 1,0,   0.5f,-0.5f,-0.5f, 0,0,   0.5f, 0.5f,-0.5f, 0,1,
         0.5f, 0.5f,-0.5f, 0,1,  -0.5f, 0.5f,-0.5f, 1,1,  -0.5f,-0.5f,-0.5f, 1,0,
        // 左面
        -0.5f, 0.5f, 0.5f, 1,1,  -0.5f, 0.5f,-0.5f, 0,1,  -0.5f,-0.5f,-0.5f, 0,0,
        -0.5f,-0.5f,-0.5f, 0,0,  -0.5f,-0.5f, 0.5f, 1,0,  -0.5f, 0.5f, 0.5f, 1,1,
        // 右面
         0.5f, 0.5f, 0.5f, 0,1,   0.5f, 0.5f,-0.5f, 1,1,   0.5f,-0.5f,-0.5f, 1,0,
         0.5f,-0.5f,-0.5f, 1,0,   0.5f,-0.5f, 0.5f, 0,0,   0.5f, 0.5f, 0.5f, 0,1,
        // 上面
        -0.5f, 0.5f,-0.5f, 0,1,   0.5f, 0.5f,-0.5f, 1,1,   0.5f, 0.5f, 0.5f, 1,0,
         0.5f, 0.5f, 0.5f, 1,0,  -0.5f, 0.5f, 0.5f, 0,0,  -0.5f, 0.5f,-0.5f, 0,1,
        // 下面
        -0.5f,-0.5f,-0.5f, 0,1,   0.5f,-0.5f,-0.5f, 1,1,   0.5f,-0.5f, 0.5f, 1,0,
         0.5f,-0.5f, 0.5f, 1,0,  -0.5f,-0.5f, 0.5f, 0,0,  -0.5f,-0.5f,-0.5f, 0,1,
    };
    constexpr int meshStride = 5 * sizeof(float);
    VertexArray cubeVAO;
    cubeVAO.Bind();
    VertexBuffer cubeVBO(cubeVerts, sizeof(cubeVerts));
    cubeVAO.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)0);
    cubeVAO.AddAttribute(1, 2, GL_FLOAT, GL_FALSE, meshStride, (void*)(3 * sizeof(float)));

    // ---------- 球体（UV球，32×32 分段）----------
    std::vector<float>        sphereVerts;
    std::vector<unsigned int> sphereIdxs;
    GenerateSphere(0.5f, 32, 32, sphereVerts, sphereIdxs);
    VertexArray  sphereVAO;
    sphereVAO.Bind();
    VertexBuffer sphereVBO(sphereVerts.data(), (unsigned int)(sphereVerts.size() * sizeof(float)));
    IndexBuffer  sphereEBO(sphereIdxs.data(), (unsigned int)sphereIdxs.size());
    sphereVAO.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, meshStride, (void*)0);
    sphereVAO.AddAttribute(1, 2, GL_FLOAT, GL_FALSE, meshStride, (void*)(3 * sizeof(float)));


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

    shader.Use();
    shader.SetInt("texture_diffuse",  0);
    shader.SetInt("texture_specular", 1);

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
        shader.SetMat4("view",       view);
        shader.SetMat4("projection", projection);

        // 立方体（左侧，持续自转）
        glm::mat4 modelCube = glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.0f, 0.0f));
        modelCube = glm::rotate(modelCube, currentFrame, glm::vec3(0.5f, 1.0f, 0.0f));
        shader.SetMat4("model", modelCube);
        cubeVAO.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 球体（右侧，持续自转）
        glm::mat4 modelSphere = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.0f, 0.0f));
        modelSphere = glm::rotate(modelSphere, currentFrame * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
        shader.SetMat4("model", modelSphere);
        sphereVAO.Bind();
        glDrawElements(GL_TRIANGLES, sphereEBO.GetCount(), GL_UNSIGNED_INT, 0);

        win.SwapBuffers();
        win.PollEvents();
    }

    return 0;
}
