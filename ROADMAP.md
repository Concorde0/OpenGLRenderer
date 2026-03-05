# OpenGL渲染框架 15天学习路线图

## 项目目标
基于OpenGL底层API开发渲染框架，掌握现代图形渲染技术的核心原理。

---

## 代码架构设计

### 推荐的类结构

```
src/
├── Core/
│   ├── Window.h/cpp          # 窗口管理、输入处理
│   ├── Application.h/cpp     # 主应用程序类
│   └── Camera.h/cpp          # 相机控制
│
├── Rendering/
│   ├── Shader.h/cpp          # 着色器编译、管理
│   ├── Texture.h/cpp         # 纹理加载、管理
│   ├── Material.h/cpp        # 材质系统（Phong/PBR）
│   ├── Mesh.h/cpp            # 网格数据结构
│   ├── Model.h/cpp           # 模型加载（OBJ支持）
│   ├── VertexArray.h/cpp     # VAO封装
│   ├── VertexBuffer.h/cpp    # VBO封装
│   ├── IndexBuffer.h/cpp     # EBO封装
│   └── Framebuffer.h/cpp     # FBO封装（延迟渲染用）
│
├── Lighting/
│   ├── Light.h/cpp           # 光源基类
│   ├── DirectionalLight.h/cpp
│   ├── PointLight.h/cpp
│   ├── SpotLight.h/cpp
│   └── ShadowMap.h/cpp       # 阴影贴图
│
├── Renderer/
│   ├── Renderer.h/cpp        # 前向渲染器
│   ├── DeferredRenderer.h/cpp # 延迟渲染器
│   └── RenderCommand.h/cpp   # OpenGL命令抽象
│
└── UI/
    └── ImGuiLayer.h/cpp      # ImGui集成
```

---

## 15天学习计划

### **第1-2天：代码重构 + 基础封装**
**目标：** 将臃肿的Core.cpp重构为模块化架构

#### 任务清单：
- [x] 当前状态：单文件Core.cpp
- [ ] 创建Window类（窗口初始化、事件处理）
- [ ] 创建Shader类（编译、链接、uniform设置）
- [ ] 创建Texture类（加载、绑定、参数设置）
- [ ] 创建VertexArray、VertexBuffer、IndexBuffer类
- [ ] 创建Camera类（视角变换、键盘/鼠标控制）
- [ ] 重构Core.cpp使用新类

**学习目标：**
- 理解OpenGL对象的生命周期管理
- 掌握RAII封装技巧
- 学习MVP矩阵变换

**参考资料：**
- LearnOpenGL: Camera章节
- 3DMath基础：矩阵变换

---

### **第3-4天：完善纹理系统 + UV映射**
**目标：** 深入理解纹理映射原理

#### 任务清单：
- [ ] 扩展Texture类支持多种格式（JPG/PNG/BMP）
- [ ] 实现纹理坐标变换（平铺、镜像、钳位）
- [ ] 支持多重纹理绑定（漫反射+高光贴图）
- [ ] 实现mipmap生成与过滤模式
- [ ] 测试：渲染带纹理的立方体、球体

**学习目标：**
- 纹理坐标插值原理
- 纹理过滤（线性/最近邻）
- 多纹理单元管理

**着色器示例：**
```glsl
// Fragment Shader
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
in vec2 TexCoords;

void main() {
    vec3 diffuse = texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular = texture(texture_specular1, TexCoords).rgb;
}
```

---

### **第5-6天：Blinn-Phong光照模型**
**目标：** 实现经典光照模型

#### 任务清单：
- [ ] 创建Light基类和具体光源类
- [ ] 实现环境光（Ambient）
- [ ] 实现漫反射（Diffuse）- Lambert定律
- [ ] 实现镜面反射（Specular）- Blinn-Phong改进
- [ ] 支持多光源（点光源、方向光、聚光灯）
- [ ] 实现Phong材质系统（Ka/Kd/Ks参数）

**学习目标：**
- 向量点积/反射计算
- 光照衰减公式
- 法线变换（法线矩阵）

**关键公式：**
```
diffuse = max(dot(N, L), 0.0)
specular = pow(max(dot(N, H), 0.0), shininess)  // H = normalize(L + V)
```

---

### **第7-8天：Shadow Mapping（阴影映射）**
**目标：** 实现动态阴影

#### 任务清单：
- [ ] 创建Framebuffer类（FBO封装）
- [ ] 创建ShadowMap类
- [ ] 实现深度贴图生成（从光源视角渲染）
- [ ] 在主渲染Pass中采样阴影贴图
- [ ] 解决Shadow Acne（深度偏移）
- [ ] 实现PCF软阴影

**学习目标：**
- Framebuffer对象
- 深度测试原理
- 多Pass渲染

**着色器流程：**
1. **Shadow Pass:** 渲染到深度贴图
2. **Lighting Pass:** 比较片元深度与阴影贴图

---

### **第9-10天：法线贴图（Normal Mapping）**
**目标：** 提升细节表现力

#### 任务清单：
- [ ] 加载法线贴图
- [ ] 在顶点数据中添加切线（Tangent）
- [ ] 计算TBN矩阵（切线空间）
- [ ] 在片段着色器中进行法线扰动
- [ ] 测试：砖墙/岩石材质

**学习目标：**
- 切线空间与世界空间转换
- 法线映射原理

---

### **第11-12天：PBR材质系统（Cook-Torrance）**
**目标：** 实现物理准确的材质

#### 任务清单：
- [ ] 创建PBRMaterial类
- [ ] 实现微表面理论（D项：GGX法线分布）
- [ ] 实现几何遮蔽（G项：Smith方法）
- [ ] 实现菲涅尔效应（F项：Schlick近似）
- [ ] 支持金属度/粗糙度工作流
- [ ] 实现IBL（Image-Based Lighting，选做）

**学习目标：**
- BRDF方程
- 能量守恒定律
- 金属/非金属材质差异

**PBR参数：**
- Albedo（基础颜色）
- Metallic（金属度）
- Roughness（粗糙度）
- AO（环境光遮蔽）

---

### **第13天：延迟渲染管线（Deferred Rendering）**
**目标：** 优化多光源性能

#### 任务清单：
- [ ] 创建DeferredRenderer类
- [ ] 实现G-Buffer（位置、法线、Albedo、材质参数）
- [ ] Geometry Pass：写入G-Buffer
- [ ] Lighting Pass：全屏四边形计算光照
- [ ] 对比前向渲染性能差异

**学习目标：**
- MRT（Multiple Render Targets）
- 屏幕空间计算
- 延迟渲染的优缺点

**G-Buffer布局：**
```
Attachment 0: RGB(Position), A(unused)
Attachment 1: RGB(Normal), A(unused)
Attachment 2: RGB(Albedo), A(Specular)
Attachment 3: RGB(Material), A(unused)
```

---

### **第14天：OBJ模型加载 + 批量渲染**
**目标：** 渲染复杂场景

#### 任务清单：
- [ ] 集成Assimp库（或手写简单OBJ解析器）
- [ ] 创建Model类（管理多个Mesh）
- [ ] 创建Mesh类（顶点、索引、材质）
- [ ] 实现场景图（Scene Graph，简单版本）
- [ ] 实现Instancing批量渲染（选做）

**学习目标：**
- 网格数据结构
- 材质与网格的关联
- 绘制调用优化

---

### **第15天：ImGui集成 + 场景调试**
**目标：** 实时参数调节

#### 任务清单：
- [ ] 集成ImGui库
- [ ] 创建ImGuiLayer类
- [ ] 添加控制面板：
  - 光源参数（位置、颜色、强度）
  - 材质参数（粗糙度、金属度）
  - 相机参数
  - 渲染模式切换（前向/延迟）
  - FPS显示
- [ ] 添加场景对象选择器

**学习目标：**
- GUI与渲染循环整合
- 实时调试技巧

---

## 每日学习建议

### 学习节奏
- **上午（3-4h）：** 理论学习 + 示例代码阅读
- **下午（3-4h）：** 动手实现 + 调试
- **晚上（1-2h）：** 总结笔记 + 代码优化

### 推荐资源
1. **LearnOpenGL CN**（中文版）- 必读
   - https://learnopengl-cn.github.io/
2. **《Real-Time Rendering 4th》** - PBR章节
3. **Shadertoy** - 着色器实验
4. **RenderDoc** - 帧调试工具

### 调试技巧
- 使用**glGetError()**检查OpenGL错误
- 使用**RenderDoc**捕获帧进行分析
- 逐步验证：先单光源再多光源
- 分离着色器调试：输出中间结果

---

## 代码风格建议

### 命名规范
```cpp
// 类名：大驼峰
class VertexBuffer {};

// 成员变量：m_前缀
unsigned int m_RendererID;

// 函数：大驼峰
void Bind() const;

// 局部变量：小驼峰
int vertexCount = 0;
```

### 资源管理
- 使用RAII管理OpenGL对象
- 析构函数释放资源
- 避免裸指针，优先使用智能指针

```cpp
class Texture {
public:
    Texture(const std::string& path);
    ~Texture() { glDeleteTextures(1, &m_RendererID); }
    
    void Bind(unsigned int slot = 0) const;
private:
    unsigned int m_RendererID;
};
```

---

## 阶段性检查点

### Day 5 检查
- [ ] 能渲染带纹理的多个立方体
- [ ] 相机可自由移动
- [ ] 实现了Phong光照

### Day 10 检查
- [ ] 场景有动态阴影
- [ ] 使用法线贴图的材质
- [ ] 至少3个光源同时工作

### Day 15 检查
- [ ] 完整的PBR材质球展示
- [ ] 延迟渲染管线运行
- [ ] 可加载OBJ模型
- [ ] ImGui实时调节参数
- [ ] FPS > 60（场景复杂度适中）

---

## 可能遇到的坑

### 常见错误
1. **黑屏问题**
   - 检查着色器编译错误
   - 验证MVP矩阵是否正确
   - 确认纹理加载成功

2. **阴影错误**
   - Shadow Acne：增加深度偏移
   - Peter Panning：调整偏移量
   - 阴影边缘锯齿：使用PCF

3. **法线贴图无效**
   - 检查TBN矩阵正交性
   - 验证切线计算
   - 法线贴图格式（0-1映射到-1到1）

4. **PBR过暗/过亮**
   - 检查光照单位（HDR）
   - Gamma校正
   - 能量守恒验证

---

## 扩展方向（15天后）

- **高级阴影**：CSM（级联阴影）、PCSS（软阴影）
- **全局光照**：SSR（屏幕空间反射）、SSAO（环境光遮蔽）
- **后处理**：Bloom、HDR、色调映射
- **抗锯齿**：MSAA、FXAA、TAA
- **粒子系统**
- **天空盒/HDR环境贴图**

---

## 总结

这个路线图循序渐进，从基础重构到高级渲染技术。关键是：
1. **前2天的重构是基础** - 好的架构让后续开发事半功倍
2. **每个阶段都要测试** - 及时发现问题
3. **不要跳跃式学习** - 每个技术都有依赖关系
4. **动手比看书重要** - 实践中理解原理

15天很紧张，但如果每天投入8小时，完全可以达成目标。加油！🚀
