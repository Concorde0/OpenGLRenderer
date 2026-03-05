#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>

class Texture
{
public:
    Texture(const std::string& path, bool flipVertically = true);
    ~Texture();

    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    unsigned int GetID() const { return m_TextureID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    unsigned int m_TextureID;
    std::string m_FilePath;
    int m_Width, m_Height, m_Channels;
};

#endif
