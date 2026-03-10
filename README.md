# OpenGLRenderer

一个基于 **C++ + OpenGL** 的渲染学习项目，目标是快速实现一个可运行的 Demo，展示纹理映射、光照、阴影和材质调试。

##  功能特性
- [x] 纹理 UV 映射与插值 
- [x] Blinn-Phong 光照模型
- [x] Shadow Mapping 阴影投射
- [x] 法线贴图增强材质立体感
- [x] Cook-Torrance PBR 材质模型
- [ ] 延迟渲染管线（多光源优化）
- [ ] OBJ 模型导入与批量渲染
- [ ] ImGui 控制面板，实时调节材质参数

##  技术栈
- **C++17**
- **OpenGL 4.x**
- **GLFW / GLAD** 
- **ImGui** 
- **Assimp**	

## 项目结构

```
OpenGLRenderer/
├─ src/        # 源代码
├─ shaders/    # GLSL 着色器
├─ assets/     # 纹理与示例资源
├─ docs/       # 文档与指南
├─ lib/        # 第三方库（静态/动态库）
├─ bin/        # 可执行文件
└─ include/    # 头文件
```

