#include "Application.h"
#include <iostream>
#include <glad/glad.h>

Application::Application(int width, int height, const char* title)
    : m_window(nullptr), m_width(width), m_height(height), m_title(title), m_lastTime(0.0), m_deltaTime(0.0f) {}

Application::~Application() {
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Application::init() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);
    m_lastTime = glfwGetTime();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    return true;
}

void Application::run() {
    while (!glfwWindowShouldClose(m_window)) {
        double currentTime = glfwGetTime();
        m_deltaTime = static_cast<float>(currentTime - m_lastTime);
        m_lastTime = currentTime;

        glfwPollEvents();
    }
}

float Application::getTime() const {
    return static_cast<float>(glfwGetTime());
}

bool Application::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->m_width = width;
        app->m_height = height;
    }
    glViewport(0, 0, width, height);
}
