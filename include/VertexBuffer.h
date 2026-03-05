#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

#include <glad/glad.h>

class VertexBuffer {
public:
    // size: 字节数（传 sizeof(vertices) 即可）
    VertexBuffer(const void* data, unsigned int size) {
        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    void Bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    }

    void Unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    ~VertexBuffer() {
        glDeleteBuffers(1, &m_VBO);
    }

private:
    unsigned int m_VBO;
};

#endif