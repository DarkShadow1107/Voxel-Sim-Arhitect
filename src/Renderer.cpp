#include "Renderer.hpp"

#include <iostream>

#include <glad/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

Renderer::Renderer() : m_window(nullptr) {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(int width, int height, const std::string& title, bool fullscreen, GLFWwindow* share) {
    std::cout << "Initializing Renderer: " << width << "x" << height << " - " << title << (fullscreen ? " (Fullscreen)" : "") << std::endl;

    if (m_window) {
        return true;
    }

    if (!glfwInit()) {
        std::cerr << "GLFW init failed" << std::endl;
        return false;
    }

    // Ask for a modern context. ImGui OpenGL3 backend supports GL 2.x+.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    if (fullscreen) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
    }

    m_window = glfwCreateWindow(width, height, title.c_str(), monitor, share);
    if (!m_window) {
        std::cerr << "GLFW window creation failed" << std::endl;
        // Don't terminate here if we are trying to create a second window
        return false;
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "GLAD load failed" << std::endl;
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
        return false;
    }

    setVSync(true);

    glfwSetFramebufferSizeCallback(
        m_window,
        [](GLFWwindow*, int w, int h) {
            glViewport(0, 0, w, h);
        });

    // Ensure viewport is correct for the initial framebuffer.
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(m_window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    setBackfaceCulling(false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void Renderer::shutdown() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
    }
}

void Renderer::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

void Renderer::setWireframe(bool enabled) {
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

void Renderer::setBackfaceCulling(bool enabled) {
    m_backfaceCulling = enabled;
    if (enabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void Renderer::setFullscreen(bool enabled) {
    if (!m_window) return;

    if (enabled) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(m_window, nullptr, 100, 100, 1280, 720, 0);
    }
}

void Renderer::clear() {
    glClearColor(0.45f, 0.55f, 0.60f, 1.0f); // Sky blue-ish
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::clear(float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::swapBuffers() {
    if (!m_window) {
        return;
    }
    glfwSwapBuffers(m_window);
    glfwPollEvents();
}

bool Renderer::shouldClose() {
    return m_window ? glfwWindowShouldClose(m_window) != 0 : true;
}

int Renderer::getWindowWidth() const {
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);
    return w;
}

int Renderer::getWindowHeight() const {
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);
    return h;
}
