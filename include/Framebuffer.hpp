#pragma once

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    bool init(int width, int height);
    bool init(int width, int height, int samples);
    void destroy();

    void bind() const;
    void unbind() const;
    void resolve() const;

    void resize(int width, int height);

    unsigned int getTexture() const { return m_resolveTexture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getSamples() const { return m_samples; }

private:
    // MSAA framebuffer objects
    unsigned int m_msaaFBO = 0;
    unsigned int m_msaaColorRBO = 0;
    unsigned int m_msaaDepthRBO = 0;

    // Resolve (non-MSAA) framebuffer objects
    unsigned int m_resolveFBO = 0;
    unsigned int m_resolveTexture = 0;

    int m_width = 0;
    int m_height = 0;
    int m_samples = 4;
};
