#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    // 构造函数：读取并构建着色器
    Shader(const char* vertexPath, const char* fragmentPath);
    // 从字符串构造着色器
    Shader(const std::string& vertexSource, const std::string& fragmentSource, bool fromString);
    ~Shader();

    // 使用/激活程序
    void Use() const;
    
    // uniform工具函数
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;

    unsigned int GetID() const { return m_ProgramID; }

private:
    unsigned int m_ProgramID;

    // 检查编译/链接错误
    void CheckCompileErrors(unsigned int shader, const std::string& type);
};

#endif
