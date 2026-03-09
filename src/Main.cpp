#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "../include/Window.h"
#include "../include/Shader.h"
#include "../include/Texture.h"
#include "../include/Camera.h"
#include "../include/VertexArray.h"
#include "../include/VertexBuffer.h"
#include "../include/IndexBuffer.h"

// 来自 Window.cpp 的全局变量（相机与时间由 Window 模块管理）
extern Camera camera;
extern float deltaTime;
extern float lastFrame;

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
        layout (location = 1) in vec3 aColor;   // 保留属性槽，顶点数据不变
        layout (location = 2) in vec2 aTexCoord;
        
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

    // 顶点数据
    float vertices[] = {
        // 位置              颜色              纹理坐标
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // 创建 VAO / VBO / EBO（绑定顺序不能乱：先绑 VAO，构造函数内自动绑 VBO/EBO）
    VertexArray vao;
    vao.Bind();

    VertexBuffer vbo(vertices, sizeof(vertices));
    IndexBuffer  ebo(indices, sizeof(indices) / sizeof(unsigned int));

    // 设置顶点属性（需在 VAO 和 VBO 已绑定的状态下调用）
    constexpr int stride = 8 * sizeof(float);
    vao.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);                     // 位置
    vao.AddAttribute(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));   // 颜色
    vao.AddAttribute(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));   // 纹理坐标


    // 加载漫反射贴图（slot 0）
    Texture diffuseMap("assets/container2.png");
    if(!diffuseMap.IsLoaded()) {
        std::cerr << "无法加载漫反射贴图，退出程序。" << std::endl;
        return -1;
    }
    diffuseMap.SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::Repeat);
    diffuseMap.SetFilterMode(TextureFilterMode::LinearMipmapLinear, TextureFilterMode::Linear);

    // 加载高光贴图（slot 1）
    Texture specularMap("assets/container_specular.png");
    if(!specularMap.IsLoaded()) {
        std::cerr << "无法加载高光贴图，退出程序。" << std::endl;
        return -1;
    }
    specularMap.SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::Repeat);
    specularMap.SetFilterMode(TextureFilterMode::LinearMipmapLinear, TextureFilterMode::Linear);

    // 绑定着色器并告知每个 sampler2D 使用哪个纹理槽
    shader.Use();
    shader.SetInt("texture_diffuse",  0);
    shader.SetInt("texture_specular", 1);

    // 渲染循环
    while (!win.ShouldClose())
    {
        // 计算deltaTime
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 输入
        win.ProcessInput();

        // 渲染
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 使用着色器
        shader.Use();

        // 创建变换矩阵
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, static_cast<float>(glfwGetTime()), glm::vec3(0.5f, 1.0f, 0.0f));

        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)Window::SCR_WIDTH / (float)Window::SCR_HEIGHT,
            0.1f, 100.0f);

        // 传递矩阵到着色器
        shader.SetMat4("model", model);
        shader.SetMat4("view", view);
        shader.SetMat4("projection", projection);

        // 绑定漫反射贴图到 slot 0，高光贴图到 slot 1
        diffuseMap.Bind(0);
        specularMap.Bind(1);
        vao.Bind();
        glDrawElements(GL_TRIANGLES, ebo.GetCount(), GL_UNSIGNED_INT, 0);

        win.SwapBuffers();
        win.PollEvents();
    }

    return 0;
}
