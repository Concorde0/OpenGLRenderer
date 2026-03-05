#ifndef INDEXBUFFER_H
#define INDEXBUFFER_H

#include <glad/glad.h>

class IndexBuffer {
public:
    IndexBuffer(const unsigned int* indices, unsigned int count)
        : m_Count(count) {
        glGenBuffers(1, &m_EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    }

    void Bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    }

    void Unbind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    unsigned int GetCount() const { return m_Count; }

    ~IndexBuffer() {
        glDeleteBuffers(1, &m_EBO);
    }

private:
    unsigned int m_EBO;
    unsigned int m_Count;
};

#endif
