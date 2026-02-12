#include "Framebuffer.hpp"
#include <glad/gl.h>
#include <iostream>

Framebuffer::~Framebuffer() {
    destroy();
}

bool Framebuffer::init(int width, int height) {
    return init(width, height, 4);
}

bool Framebuffer::init(int width, int height, int samples) {
    destroy();
    m_width = width;
    m_height = height;
    m_samples = samples;

    // Clamp samples to GPU maximum
    int maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (m_samples > maxSamples) {
        m_samples = maxSamples;
    }
    if (m_samples < 1) {
        m_samples = 1;
    }

    // --- Create the MSAA framebuffer ---
    glGenFramebuffers(1, &m_msaaFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFBO);

    // Multisampled color renderbuffer
    glGenRenderbuffers(1, &m_msaaColorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaColorRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msaaColorRBO);

    // Multisampled depth/stencil renderbuffer
    glGenRenderbuffers(1, &m_msaaDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_msaaDepthRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_msaaDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "MSAA Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    // --- Create the resolve (non-MSAA) framebuffer ---
    glGenFramebuffers(1, &m_resolveFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFBO);

    // Regular texture for resolved result (used by ImGui::Image)
    glGenTextures(1, &m_resolveTexture);
    glBindTexture(GL_TEXTURE_2D, m_resolveTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_resolveTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Resolve Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void Framebuffer::destroy() {
    if (m_msaaFBO)       glDeleteFramebuffers(1, &m_msaaFBO);
    if (m_msaaColorRBO)  glDeleteRenderbuffers(1, &m_msaaColorRBO);
    if (m_msaaDepthRBO)  glDeleteRenderbuffers(1, &m_msaaDepthRBO);
    if (m_resolveFBO)    glDeleteFramebuffers(1, &m_resolveFBO);
    if (m_resolveTexture) glDeleteTextures(1, &m_resolveTexture);
    m_msaaFBO = 0;
    m_msaaColorRBO = 0;
    m_msaaDepthRBO = 0;
    m_resolveFBO = 0;
    m_resolveTexture = 0;
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaaFBO);
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resolve() const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFBO);
    glBlitFramebuffer(0, 0, m_width, m_height,
                      0, 0, m_width, m_height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    init(width, height, m_samples);
}
