#pragma once

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    bool init(int width, int height);
    void destroy();

    void bind() const;
    void unbind() const;

    void resize(int width, int height);

    unsigned int getTexture() const { return m_texture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    unsigned int m_fbo = 0;
    unsigned int m_texture = 0;
    unsigned int m_rbo = 0;
    int m_width = 0;
    int m_height = 0;
};
