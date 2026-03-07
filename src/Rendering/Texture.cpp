#include "../include/Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <algorithm>

Texture::Texture(const std::string& path, bool flipVertically) 
    : m_TextureID(0), m_FilePath(path), m_Width(0), m_Height(0), m_Channels(0), 
      m_IsLoaded(false), m_Format(TextureFormat::Unknown)
{
    // 检测文件格式
    m_Format = DetermineFormat(path);
    m_FormatString = GetFormatName(m_Format);

    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, 0);
    
    if (data) {
        GLenum format = GetGLFormat(m_Channels);

        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);

        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        m_IsLoaded = true;

        std::cout << "✓ Texture loaded successfully!" << std::endl;
        std::cout << "  Path: " << path << std::endl;
        std::cout << "  Format: " << m_FormatString << std::endl;
        std::cout << "  Size: " << m_Width << "x" << m_Height << std::endl;
        std::cout << "  Channels: " << m_Channels << std::endl;
        std::cout << "  Memory: " << GetMemorySize() / 1024 << " KB" << std::endl;
    }
    else {
        std::cerr << "✗ Failed to load texture: " << path << std::endl;
        std::cerr << "  Error: " << stbi_failure_reason() << std::endl;
    }

    stbi_image_free(data);
}

Texture::~Texture()
{
    if (m_TextureID != 0) {
        glDeleteTextures(1, &m_TextureID);
    }
}

void Texture::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

TextureFormat Texture::DetermineFormat(const std::string& filePath)
{
    // 获取文件扩展名
    size_t dotPos = filePath.find_last_of(".");
    if (dotPos == std::string::npos) {
        return TextureFormat::Unknown;
    }

    std::string ext = filePath.substr(dotPos + 1);
    
    // 转换为小写
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "png") return TextureFormat::PNG;
    if (ext == "jpg" || ext == "jpeg") return TextureFormat::JPG;
    if (ext == "bmp") return TextureFormat::BMP;
    if (ext == "gif") return TextureFormat::GIF;
    if (ext == "psd") return TextureFormat::PSD;
    if (ext == "hdr") return TextureFormat::HDR;
    if (ext == "pic") return TextureFormat::PIC;

    return TextureFormat::Unknown;
}

std::string Texture::GetFormatName(TextureFormat format)
{
    switch (format) {
        case TextureFormat::PNG: return "PNG";
        case TextureFormat::JPG: return "JPEG";
        case TextureFormat::BMP: return "BMP";
        case TextureFormat::GIF: return "GIF";
        case TextureFormat::PSD: return "Photoshop";
        case TextureFormat::HDR: return "HDR";
        case TextureFormat::PIC: return "PIC";
        case TextureFormat::Unknown:
        default: return "Unknown";
    }
}

GLenum Texture::GetGLFormat(int channels)
{
    switch (channels) {
        case 1: return GL_RED;
        case 2: return GL_RG;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default: return GL_RGB;
    }
}

bool Texture::IsFormatSupported(const std::string& filePath)
{
    TextureFormat format = DetectFormat(filePath);
    return format != TextureFormat::Unknown;
}

TextureFormat Texture::DetectFormat(const std::string& filePath)
{
    size_t dotPos = filePath.find_last_of(".");
    if (dotPos == std::string::npos) {
        return TextureFormat::Unknown;
    }

    std::string ext = filePath.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "png") return TextureFormat::PNG;
    if (ext == "jpg" || ext == "jpeg") return TextureFormat::JPG;
    if (ext == "bmp") return TextureFormat::BMP;
    if (ext == "gif") return TextureFormat::GIF;
    if (ext == "psd") return TextureFormat::PSD;
    if (ext == "hdr") return TextureFormat::HDR;
    if (ext == "pic") return TextureFormat::PIC;

    return TextureFormat::Unknown;
}
