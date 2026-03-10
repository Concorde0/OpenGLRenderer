#include "../../include/Framebuffer.h"

#include <iostream>

namespace {
const char* FramebufferStatusToString(GLenum status)
{
	switch (status) {
	case GL_FRAMEBUFFER_COMPLETE:
		return "GL_FRAMEBUFFER_COMPLETE";
	case GL_FRAMEBUFFER_UNDEFINED:
		return "GL_FRAMEBUFFER_UNDEFINED";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "GL_FRAMEBUFFER_UNSUPPORTED";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
	default:
		return "GL_FRAMEBUFFER_UNKNOWN_STATUS";
	}
}
}

Framebuffer::Framebuffer()
	: m_FBO(0)
{
	glGenFramebuffers(1, &m_FBO);
}

Framebuffer::~Framebuffer()
{
	if (m_FBO != 0) {
		glDeleteFramebuffers(1, &m_FBO);
	}
}

void Framebuffer::Bind(GLenum target) const
{
	glBindFramebuffer(target, m_FBO);
}

void Framebuffer::Unbind(GLenum target)
{
	glBindFramebuffer(target, 0);
}

void Framebuffer::AttachTexture2D(GLenum attachment, unsigned int textureID,
								  GLenum textarget, int level) const
{
	GLint previousFbo = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

	Bind(GL_FRAMEBUFFER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textarget, textureID, level);

	glBindFramebuffer(GL_FRAMEBUFFER, static_cast<unsigned int>(previousFbo));
}

void Framebuffer::AttachRenderbuffer(GLenum attachment, unsigned int renderbufferID,
									 GLenum renderbufferTarget) const
{
	GLint previousFbo = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

	Bind(GL_FRAMEBUFFER);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, renderbufferTarget, renderbufferID);

	glBindFramebuffer(GL_FRAMEBUFFER, static_cast<unsigned int>(previousFbo));
}

void Framebuffer::SetDrawReadBufferNone() const
{
	GLint previousFbo = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

	Bind(GL_FRAMEBUFFER);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, static_cast<unsigned int>(previousFbo));
}

bool Framebuffer::IsComplete(GLenum target) const
{
	GLint previousFbo = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);

	Bind(target);
	const GLenum status = glCheckFramebufferStatus(target);
	const bool complete = (status == GL_FRAMEBUFFER_COMPLETE);

	if (!complete) {
		std::cerr << "[Framebuffer] Incomplete FBO (" << m_FBO << "): "
				  << FramebufferStatusToString(status) << std::endl;
	}

	glBindFramebuffer(target, static_cast<unsigned int>(previousFbo));
	return complete;
}
