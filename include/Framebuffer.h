#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <glad/glad.h>

class Framebuffer {
public:
	Framebuffer();
	~Framebuffer();

	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;

	void Bind(GLenum target = GL_FRAMEBUFFER) const;
	static void Unbind(GLenum target = GL_FRAMEBUFFER);

	void AttachTexture2D(GLenum attachment, unsigned int textureID,
						 GLenum textarget = GL_TEXTURE_2D, int level = 0) const;
	void AttachRenderbuffer(GLenum attachment, unsigned int renderbufferID,
							GLenum renderbufferTarget = GL_RENDERBUFFER) const;
	void SetDrawReadBufferNone() const;

	bool IsComplete(GLenum target = GL_FRAMEBUFFER) const;

	unsigned int GetID() const { return m_FBO; }

private:
	unsigned int m_FBO;
};

#endif
