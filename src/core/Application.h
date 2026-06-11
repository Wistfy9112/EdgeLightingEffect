#ifndef APPLICATION_H
#define APPLICATION_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    bool init();
    void run();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getTime() const;
    float getDeltaTime() const { return m_deltaTime; }
    bool shouldClose() const;
    GLFWwindow* getWindow() const { return m_window; }

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    double m_lastTime;
    float m_deltaTime;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};

#endif
