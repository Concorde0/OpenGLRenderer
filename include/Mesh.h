#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Shader.h"

struct MeshVertex {
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 TexCoord = glm::vec2(0.0f);
    glm::vec3 Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
};

struct MeshMaterial {
    std::string Name;
    glm::vec3 AlbedoTint = glm::vec3(1.0f);
    float Metallic = 0.0f;
    float Roughness = 0.8f;
};

class Mesh {
public:
    Mesh() = default;

    Mesh(std::vector<MeshVertex> vertices,
         std::vector<unsigned int> indices,
         MeshMaterial material = MeshMaterial())
        : m_Vertices(std::move(vertices)),
          m_Indices(std::move(indices)),
          m_Material(std::move(material))
    {
        SetupBuffers();
    }

    ~Mesh()
    {
        Release();
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept
    {
        MoveFrom(std::move(other));
    }

    Mesh& operator=(Mesh&& other) noexcept
    {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    void Draw(const Shader& shader, const glm::mat4& modelMatrix) const
    {
        if (m_VAO == 0 || m_Indices.empty()) {
            return;
        }

        shader.SetBool("useInstancing", false);
        shader.SetMat4("model", modelMatrix);

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void DrawInstanced(const Shader& shader,
                       const std::vector<glm::mat4>& instanceMatrices,
                       const glm::mat4& baseModel = glm::mat4(1.0f))
    {
        if (m_VAO == 0 || m_Indices.empty() || instanceMatrices.empty()) {
            return;
        }

        UploadInstanceTransforms(instanceMatrices);

        shader.SetMat4("model", baseModel);
        shader.SetBool("useInstancing", true);

        glBindVertexArray(m_VAO);
        glDrawElementsInstanced(GL_TRIANGLES,
                                static_cast<GLsizei>(m_Indices.size()),
                                GL_UNSIGNED_INT,
                                nullptr,
                                static_cast<GLsizei>(instanceMatrices.size()));
        glBindVertexArray(0);

        shader.SetBool("useInstancing", false);
    }

    const std::vector<MeshVertex>& GetVertices() const { return m_Vertices; }
    const std::vector<unsigned int>& GetIndices() const { return m_Indices; }
    const MeshMaterial& GetMaterial() const { return m_Material; }

private:
    void SetupBuffers()
    {
        if (m_Vertices.empty() || m_Indices.empty()) {
            return;
        }

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(m_Vertices.size() * sizeof(MeshVertex)),
                     m_Vertices.data(),
                     GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(m_Indices.size() * sizeof(unsigned int)),
                     m_Indices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, Position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, Normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, TexCoord));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, Tangent));

        glBindVertexArray(0);
    }

    void UploadInstanceTransforms(const std::vector<glm::mat4>& instanceMatrices)
    {
        glBindVertexArray(m_VAO);

        if (m_InstanceVBO == 0) {
            glGenBuffers(1, &m_InstanceVBO);
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(instanceMatrices.size() * sizeof(glm::mat4)),
                     instanceMatrices.data(),
                     GL_DYNAMIC_DRAW);

        if (!m_InstanceAttributesConfigured) {
            const GLsizei stride = static_cast<GLsizei>(sizeof(glm::mat4));
            for (unsigned int i = 0; i < 4; ++i) {
                glEnableVertexAttribArray(4 + i);
                glVertexAttribPointer(4 + i,
                                      4,
                                      GL_FLOAT,
                                      GL_FALSE,
                                      stride,
                                      (void*)(sizeof(glm::vec4) * i));
                glVertexAttribDivisor(4 + i, 1);
            }
            m_InstanceAttributesConfigured = true;
        }

        glBindVertexArray(0);
    }

    void MoveFrom(Mesh&& other) noexcept
    {
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_EBO = other.m_EBO;
        m_InstanceVBO = other.m_InstanceVBO;
        m_InstanceAttributesConfigured = other.m_InstanceAttributesConfigured;

        m_Vertices = std::move(other.m_Vertices);
        m_Indices = std::move(other.m_Indices);
        m_Material = std::move(other.m_Material);

        other.m_VAO = 0;
        other.m_VBO = 0;
        other.m_EBO = 0;
        other.m_InstanceVBO = 0;
        other.m_InstanceAttributesConfigured = false;
    }

    void Release()
    {
        if (m_InstanceVBO != 0) {
            glDeleteBuffers(1, &m_InstanceVBO);
            m_InstanceVBO = 0;
        }
        if (m_EBO != 0) {
            glDeleteBuffers(1, &m_EBO);
            m_EBO = 0;
        }
        if (m_VBO != 0) {
            glDeleteBuffers(1, &m_VBO);
            m_VBO = 0;
        }
        if (m_VAO != 0) {
            glDeleteVertexArrays(1, &m_VAO);
            m_VAO = 0;
        }
        m_InstanceAttributesConfigured = false;
    }

private:
    std::vector<MeshVertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    MeshMaterial m_Material;

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    unsigned int m_InstanceVBO = 0;
    bool m_InstanceAttributesConfigured = false;
};

#endif