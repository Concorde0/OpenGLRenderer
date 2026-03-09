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

    // 顶点着色器
    const std::string vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        
        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    // 片段着色器（基础 Phong：环境光 + 漫反射 + 高光）
    const std::string fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;

        struct Light {
            vec3 position;
            vec3 ambient;
            vec3 diffuse;
            vec3 specular;
        };

        uniform Light light;
        uniform vec3 viewPos;
        
        uniform sampler2D texture_diffuse;
        uniform sampler2D texture_specular;
        
        void main()
        {
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(light.position - FragPos);
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);

            float diff = max(dot(norm, lightDir), 0.0);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

            vec3 albedo = texture(texture_diffuse, TexCoord).rgb;
            vec3 specMap = texture(texture_specular, TexCoord).rgb;

            vec3 ambient  = light.ambient * albedo;
            vec3 diffuse  = light.diffuse * diff * albedo;
            vec3 specular = light.specular * spec * specMap;

            FragColor = vec4(ambient + diffuse + specular, 1.0);
        }
    )";

    // 创建着色器
    Shader shader(vertexShaderSource, fragmentShaderSource, true);

    // ---------- 立方体（每顶点：位置3 + 法线3 + 纹理2）----------
    float cubeVerts[] = {
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
    sceneLight.Apply(shader);

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
        sceneLight.Apply(shader);

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

        win.SwapBuffers();
        win.PollEvents();
    }

    return 0;
}
