# 快速开始指南

## 当前状态
你的项目已经添加了以下基础类：
- ✅ `Shader` - 着色器管理
- ✅ `Texture` - 纹理管理
- ✅ `Camera` - 相机控制
- ✅ `Main.cpp` - 重构后的示例代码

## 下一步操作

### 1. 更新编译配置

你需要更新 `.vscode/tasks.json` 中的编译任务，包含新的源文件：

```json
{
    "type": "cppbuild",
    "label": "C/C++: gcc.exe 生成活动文件",
    "command": "C:\\Users\\gp68\\mingw64\\bin\\g++.exe",
    "args": [
        "-fdiagnostics-color=always",
        "-g",
        "${workspaceFolder}\\src\\Main.cpp",
        "${workspaceFolder}\\src\\Shader.cpp",
        "${workspaceFolder}\\src\\Texture.cpp",
        "${workspaceFolder}\\src\\Camera.cpp",
        "${workspaceFolder}\\src\\glad.c",
        "-I",
        "${workspaceFolder}\\include",
        "-L",
        "${workspaceFolder}\\lib",
        "-lglfw3",
        "-lgdi32",
        "-lopengl32",
        "-o",
        "${workspaceFolder}\\bin\\main.exe"
    ],
    "options": {
        "cwd": "${fileDirname}"
    },
    "group": {
        "kind": "build",
        "isDefault": true
    },
    "detail": "调试器生成的任务。"
}
```

### 2. 测试新架构

编译并运行：
```bash
# 按 Ctrl+Shift+B 或
cd d:\VSCCode\OpenGLRenderer
g++ -g src/Main.cpp src/Shader.cpp src/Texture.cpp src/Camera.cpp src/glad.c -I include -L lib -lglfw3 -lgdi32 -lopengl32 -o bin/main.exe
```

### 3. 迁移现有代码

你可以选择：
- **方案A**: 使用 `src/Main.cpp` 作为新的入口点
- **方案B**: 逐步重构 `src/Core.cpp` 使用新类

推荐方案A，将 `Core.cpp` 重命名为 `Core_old.cpp` 备份。

### 4. 新增功能开发顺序

按照 `ROADMAP.md` 的计划，接下来应该创建：

#### Day 1-2（当前阶段）
- [x] Shader类
- [x] Texture类
- [x] Camera类
- [ ] VertexArray类
- [ ] VertexBuffer类
- [ ] IndexBuffer类

#### 示例：VertexBuffer类框架

**include/VertexBuffer.h:**
```cpp
#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include <glad/glad.h>

class VertexBuffer
{
public:
    VertexBuffer(const void* data, unsigned int size);
    ~VertexBuffer();

    void Bind() const;
    void Unbind() const;

private:
    unsigned int m_RendererID;
};

#endif
```

**src/VertexBuffer.cpp:**
```cpp
#include "../include/VertexBuffer.h"

VertexBuffer::VertexBuffer(const void* data, unsigned int size)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void VertexBuffer::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
```

### 5. 常见问题

#### Q: 编译报错找不到头文件？
A: 确保 `-I` 参数指向 `include` 目录。

#### Q: 链接错误？
A: 检查库文件路径和链接顺序，确保 `glfw3.lib` 在 `lib` 目录。

#### Q: 纹理加载失败？
A: 确保图片路径正确（相对于可执行文件）。

#### Q: 黑屏？
A: 检查着色器编译日志，Shader类会自动输出错误。

### 6. 学习资源

- **LearnOpenGL CN**: https://learnopengl-cn.github.io/
- **着色器部分**: 01 入门 → 05 着色器
- **相机部分**: 01 入门 → 09 摄像机
- **纹理部分**: 01 入门 → 06 纹理

### 7. 调试技巧

在 Main.cpp 中添加错误检查：
```cpp
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar* message, const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR) {
        fprintf(stderr, "GL ERROR: type = 0x%x, severity = 0x%x, message = %s\n",
                type, severity, message);
    }
}

// 在 gladLoadGLLoader 之后添加：
glEnable(GL_DEBUG_OUTPUT);
glDebugMessageCallback(MessageCallback, 0);
```

### 8. 今日目标（Day 1）

- [x] 创建基础类（Shader/Texture/Camera）
- [ ] 更新编译配置
- [ ] 成功运行Main.cpp
- [ ] 测试相机移动（WASD + 鼠标）

完成后，明天可以继续创建 VertexArray、VertexBuffer 等封装类！

---

**提示**: 遇到问题随时查看 `ROADMAP.md` 中的"可能遇到的坑"章节。
