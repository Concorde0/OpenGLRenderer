#include "../include/Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <algorithm>

Texture::Texture(const std::string& path, bool flipVertically) 
    : m_TextureID(0), m_FilePath(path), m_Width(0), m_Height(0), m_Channels(0), 
      m_IsLoaded(false), m_Format(TextureFormat::Unknown),
      m_WrapS(TextureWrapMode::Repeat), m_WrapT(TextureWrapMode::Repeat),
      m_MinFilter(TextureFilterMode::LinearMipmapLinear), m_MagFilter(TextureFilterMode::Linear),
      m_MipmapEnabled(true)
{
    // 初始化边界颜色（黑色）
    m_BorderColor[0] = 0.0f;
    m_BorderColor[1] = 0.0f;
    m_BorderColor[2] = 0.0f;
    m_BorderColor[3] = 1.0f;

    // 检测文件格式
    m_Format = DetermineFormat(path);
    m_FormatString = GetFormatName(m_Format);

    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, 0);
    
    if (data) {
        GLenum format = GetGLFormat(m_Channels);

        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);

        glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
        
        // 生成 Mipmap
        if (m_MipmapEnabled) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        // 应用纹理参数
        ApplyTextureParameters();

        m_IsLoaded = true;

        std::cout << "✓ Texture loaded successfully!" << std::endl;
        std::cout << "  Path: " << path << std::endl;
        std::cout << "  Format: " << m_FormatString << std::endl;
        std::cout << "  Size: " << m_Width << "x" << m_Height << std::endl;
        std::cout << "  Channels: " << m_Channels << std::endl;
        std::cout << "  Memory: " << GetMemorySize() / 1024 << " KB" << std::endl;
        std::cout << "  WrapMode: [S=" << GetWrapModeName(m_WrapS) << ", T=" << GetWrapModeName(m_WrapT) << "]" << std::endl;
        std::cout << "  FilterMode: [Min=" << GetFilterModeName(m_MinFilter) << ", Mag=" << GetFilterModeName(m_MagFilter) << "]" << std::endl;
        std::cout << "  Mipmap: " << (m_MipmapEnabled ? "Enabled" : "Disabled") << std::endl;
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

void Texture::SetWrapMode(TextureWrapMode wrapS, TextureWrapMode wrapT)
{
    m_WrapS = wrapS;
    m_WrapT = wrapT;

    if (m_TextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapS));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapT));
        glBindTexture(GL_TEXTURE_2D, 0);
        
        std::cout << "✓ Wrap mode updated: S=" << GetWrapModeName(wrapS) 
                  << ", T=" << GetWrapModeName(wrapT) << std::endl;
    }
}

void Texture::SetFilterMode(TextureFilterMode minFilter, TextureFilterMode magFilter)
{
    m_MinFilter = minFilter;
    m_MagFilter = magFilter;

    if (m_TextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(magFilter));
        glBindTexture(GL_TEXTURE_2D, 0);
        
        std::cout << "✓ Filter mode updated: Min=" << GetFilterModeName(minFilter) 
                  << ", Mag=" << GetFilterModeName(magFilter) << std::endl;
    }
}

void Texture::SetBorderColor(float r, float g, float b, float a)
{
    m_BorderColor[0] = r;
    m_BorderColor[1] = g;
    m_BorderColor[2] = b;
    m_BorderColor[3] = a;

    if (m_TextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, m_BorderColor);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        std::cout << "✓ Border color updated: RGBA(" << r << ", " << g << ", " 
                  << b << ", " << a << ")" << std::endl;
    }
}

void Texture::SetMipmapEnabled(bool enabled)
{
    m_MipmapEnabled = enabled;

    if (m_TextureID != 0) {
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
        
        if (enabled) {
            glGenerateMipmap(GL_TEXTURE_2D);
            // 设置默认的 mipmap 过滤模式
            if (m_MinFilter != TextureFilterMode::Nearest && 
                m_MinFilter != TextureFilterMode::Linear) {
                // 已经是 mipmap 模式，不需要改变
            } else {
                // 转换到 mipmap 模式
                SetFilterMode(TextureFilterMode::LinearMipmapLinear, m_MagFilter);
            }
            std::cout << "✓ Mipmap enabled" << std::endl;
        } else {
            // 禁用 mipmap，使用普通过滤模式
            SetFilterMode(TextureFilterMode::Linear, m_MagFilter);
            std::cout << "✓ Mipmap disabled" << std::endl;
        }
        
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Texture::ApplyTextureParameters()
{
    glBindTexture(GL_TEXTURE_2D, m_TextureID);
    
    // 应用寻址模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(m_WrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(m_WrapT));
    
    // 应用过滤模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(m_MinFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(m_MagFilter));
    
    // 应用边界颜色
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, m_BorderColor);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

TextureFormat Texture::DetermineFormat(const std::string& filePath)
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

const char* Texture::GetWrapModeName(TextureWrapMode mode)
{
    switch (mode) {
        case TextureWrapMode::Repeat: return "Repeat";
        case TextureWrapMode::MirroredRepeat: return "MirroredRepeat";
        case TextureWrapMode::ClampToEdge: return "ClampToEdge";
        case TextureWrapMode::ClampToBorder: return "ClampToBorder";
        default: return "Unknown";
    }
}

const char* Texture::GetFilterModeName(TextureFilterMode mode)
{
    switch (mode) {
        case TextureFilterMode::Nearest: return "Nearest";
        case TextureFilterMode::Linear: return "Linear";
        case TextureFilterMode::NearestMipmapNearest: return "NearestMipmapNearest";
        case TextureFilterMode::LinearMipmapNearest: return "LinearMipmapNearest";
        case TextureFilterMode::NearestMipmapLinear: return "NearestMipmapLinear";
        case TextureFilterMode::LinearMipmapLinear: return "LinearMipmapLinear";
        default: return "Unknown";
    }
}
