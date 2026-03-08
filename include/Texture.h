#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>

enum class TextureFormat
{
    Unknown,
    PNG,
    JPG,
    BMP,
    GIF,
    PSD,
    HDR,
    PIC
};

/**
 * 纹理坐标寻址模式
 * 控制当纹理坐标超出 [0,1] 范围时的采样行为
 */
enum class TextureWrapMode
{
    Repeat = GL_REPEAT,                    // 平铺：重复纹理
    MirroredRepeat = GL_MIRRORED_REPEAT,   // 镜像：镜像重复纹理
    ClampToEdge = GL_CLAMP_TO_EDGE,        // 钳位：使用边缘像素
    ClampToBorder = GL_CLAMP_TO_BORDER     // 边界钳位：使用指定边界颜色
};

/**
 * 纹理过滤模式
 * 控制纹素采样方式
 */
enum class TextureFilterMode
{
    Nearest = GL_NEAREST,                          // 最近邻滤波（速度快，质量低）
    Linear = GL_LINEAR,                            // 线性滤波（速度慢，质量高）
    NearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST, // 使用最邻近的多级渐远纹理来匹配像素大小，并使用邻近插值进行纹理采样
    LinearMipmapNearest = GL_LINEAR_MIPMAP_NEAREST, // 使用最邻近的多级渐远纹理级别，并使用线性插值进行采样
    NearestMipmapLinear = GL_NEAREST_MIPMAP_LINEAR, // 在两个最匹配像素大小的多级渐远纹理之间进行线性插值，使用邻近插值进行采样
    LinearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR   // 在两个邻近的多级渐远纹理之间使用线性插值，并使用线性插值进行采样
};

class Texture
{
public:
    // 构造函数
    Texture(const std::string& path, bool flipVertically = true);
    ~Texture();

    // 绑定和解绑
    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    // Getter 方法
    unsigned int GetID() const { return m_TextureID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetChannels() const { return m_Channels; }
    const std::string& GetFilePath() const { return m_FilePath; }
    TextureFormat GetFormat() const { return m_Format; }
    const std::string& GetFormatString() const { return m_FormatString; }
    
    // 纹理信息
    bool IsLoaded() const { return m_IsLoaded; }
    size_t GetMemorySize() const { return m_Width * m_Height * m_Channels; }

    // ==================== 纹理坐标变换设置 ====================
    
    /**
     * 设置纹理寻址模式（S方向和T方向）
     * @param wrapS S坐标寻址模式（默认 Repeat）
     * @param wrapT T坐标寻址模式（默认 Repeat）
     */
    void SetWrapMode(TextureWrapMode wrapS, TextureWrapMode wrapT = TextureWrapMode::Repeat);
    
    /**
     * 设置纹理过滤模式
     * @param minFilter 缩小过滤模式（默认 LinearMipmapLinear）
     * @param magFilter 放大过滤模式（默认 Linear）
     */
    void SetFilterMode(TextureFilterMode minFilter, TextureFilterMode magFilter = TextureFilterMode::Linear);
    
    /**
     * 设置纹理边界颜色（仅当使用 ClampToBorder 时有效）
     * @param r 红色通道 [0.0f, 1.0f]
     * @param g 绿色通道 [0.0f, 1.0f]
     * @param b 蓝色通道 [0.0f, 1.0f]
     * @param a Alpha 通道 [0.0f, 1.0f]
     */
    void SetBorderColor(float r, float g, float b, float a = 1.0f);
    
    /**
     * 启用或禁用 Mipmap
     * @param enabled 是否启用 Mipmap
     */
    void SetMipmapEnabled(bool enabled);

    // 获取当前设置
    TextureWrapMode GetWrapModeS() const { return m_WrapS; }
    TextureWrapMode GetWrapModeT() const { return m_WrapT; }
    TextureFilterMode GetMinFilter() const { return m_MinFilter; }
    TextureFilterMode GetMagFilter() const { return m_MagFilter; }
    bool IsMipmapEnabled() const { return m_MipmapEnabled; }

    // 静态方法
    static bool IsFormatSupported(const std::string& filePath);
    static TextureFormat DetectFormat(const std::string& filePath);
    static const char* GetWrapModeName(TextureWrapMode mode);
    static const char* GetFilterModeName(TextureFilterMode mode);

private:
    unsigned int m_TextureID;
    std::string m_FilePath;
    int m_Width, m_Height, m_Channels;
    TextureFormat m_Format;
    std::string m_FormatString;
    bool m_IsLoaded;

    // ==================== 纹理参数 ====================
    TextureWrapMode m_WrapS;
    TextureWrapMode m_WrapT;
    TextureFilterMode m_MinFilter;
    TextureFilterMode m_MagFilter;
    bool m_MipmapEnabled;
    float m_BorderColor[4];

    // 辅助方法
    TextureFormat DetermineFormat(const std::string& filePath);
    std::string GetFormatName(TextureFormat format);
    GLenum GetGLFormat(int channels);
    void ApplyTextureParameters();
};

#endif
