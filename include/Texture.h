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

    // 静态方法
    static bool IsFormatSupported(const std::string& filePath);
    static TextureFormat DetectFormat(const std::string& filePath);

private:
    unsigned int m_TextureID;
    std::string m_FilePath;
    int m_Width, m_Height, m_Channels;
    TextureFormat m_Format;
    std::string m_FormatString;
    bool m_IsLoaded;

    // 辅助方法
    TextureFormat DetermineFormat(const std::string& filePath);
    std::string GetFormatName(TextureFormat format);
    GLenum GetGLFormat(int channels);
};

#endif
