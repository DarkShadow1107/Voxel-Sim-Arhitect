#pragma once
#include <string>
#include <vector>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(int width, int height, const std::string& title, bool fullscreen = false, struct GLFWwindow* share = nullptr);
    void shutdown();
    void clear();
    void clear(float r, float g, float b);
    void swapBuffers();
    bool shouldClose();

    void setVSync(bool enabled);
    void setWireframe(bool enabled);
    void setFullscreen(bool enabled);
    void setBackfaceCulling(bool enabled);
    bool backfaceCullingEnabled() const { return m_backfaceCulling; }

    struct GLFWwindow* getWindow() { return m_window; }
    void setWindow(struct GLFWwindow* window) { m_window = window; }
    int getWindowWidth() const;
    int getWindowHeight() const;

private:
    struct GLFWwindow* m_window;
    bool m_backfaceCulling = false;
};
