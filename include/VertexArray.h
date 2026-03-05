#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H

#include <glad/glad.h>

class VertexArray {
public:
    VertexArray() {
        glGenVertexArrays(1, &m_VAO);
    }

    void Bind() const {
        glBindVertexArray(m_VAO);
    }

    void Unbind() const {
        glBindVertexArray(0);
    }

    // 调用前需已绑定 VAO 和对应的 VBO
    void AddAttribute(unsigned int index, int size, GLenum type,
                      GLboolean normalized, int stride, const void* offset) const {
        glVertexAttribPointer(index, size, type, normalized, stride, offset);
        glEnableVertexAttribArray(index);
    }

    ~VertexArray() {
        glDeleteVertexArrays(1, &m_VAO);
    }

private:
    unsigned int m_VAO;
};


#endif