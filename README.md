# OpenGLRenderer

基于 C++ 与 OpenGL 的学习型渲染器示例，展示现代渲染技术与实时调试工具。适合作为学习、实验与扩展的平台。

## 主要功能
- 延迟渲染（G-buffer）：用于多光源高效照明
- PBR（Cook–Torrance）：基于 Metalness/Roughness 的物理材质
- Blinn-Phong：用于对比和兼容的传统光照模型
- 阴影映射（Shadow Mapping）：方向光/点光的基础阴影实现
- 法线贴图与高度贴图：增强表面细节
- 帧缓冲与后处理：HDR / 曝光 / Gamma 调整（基础）
- 纹理加载：使用 stb_image 加载贴图
- 模型与网格支持：基础 OBJ/自定义模型加载与渲染
- ImGui 调试面板：实时调节材质、光照与渲染参数
- 可扩展的着色器管理：`shaders/` 目录配合 `Shader` 类

## 项目结构

```
OpenGLRenderer/
├─ src/
│  ├─ Main.cpp                # 程序入口
│  ├─ Core/                   # 窗口、相机等核心模块（Camera, Window）
│  ├─ Rendering/              # 渲染子系统（Shader, Texture, Framebuffer, DeferredRenderer, ShadowMap）
│  ├─ Lighting/               # 光源实现（Light）
│  ├─ UI/                     # ImGui 层（ImGuiLayer）
│  └─ Other/                  # 外部依赖源（glad）
├─ include/                   # 公共头文件与第三方头
├─ shaders/                   # GLSL 着色器（.vert/.frag）
├─ assets/                    # 示例模型、纹理与资源
├─ lib/                       # 第三方库（例如 GLFW）
├─ bin/                       # 编译输出（可执行文件）
└─ docs/                      # 文档、笔记与学习材料
```

## 快速启动（Windows）

### 环境要求
- Windows 10/11
- MinGW-w64（需要 `g++`，可在 PATH 中访问）
- `lib/` 目录中可用的 GLFW 库文件

### 一键构建并运行
在项目根目录执行：

```bat
run.bat
```

该命令会自动完成：
- 编译源码到 `bin/main.exe`
- 编译成功后立即启动程序

### 脚本参数
- 仅编译：

```bat
run.bat build
```

- 仅运行（需要已有 `bin/main.exe`）：

```bat
run.bat run
```

### 在 VS Code 中运行
- 调试启动：按 `F5`
- 仅构建：按 `Ctrl+Shift+B`

### 运行时常用按键
- `W/A/S/D`：移动相机
- 鼠标：旋转视角
- `F1`：切换 Forward / Deferred 渲染模式
- `F2`：开关 ImGui 调试面板