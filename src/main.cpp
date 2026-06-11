#include <iostream>
#include <string>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "EdgeLightingEffect/EffectManager.h"
#include "EdgeLightingEffect/EdgeLightingEffect.h"
#include "EdgeLightingEffect/MagneticWaveEffect.h"

enum class DemoMode {
    EdgeLighting,
    MagneticWaveSingle,
    MagneticWaveMulti,
    AllEffects,
    Count
};

static DemoMode s_currentMode = DemoMode::EdgeLighting;
static bool s_modeChanged = true;

static const char* modeName(DemoMode mode) {
    switch (mode) {
        case DemoMode::EdgeLighting:       return "Edge Lighting";
        case DemoMode::MagneticWaveSingle: return "Magnetic Wave (1)";
        case DemoMode::MagneticWaveMulti:  return "Magnetic Waves (3)";
        case DemoMode::AllEffects:         return "All Effects";
        default:                           return "Unknown";
    }
}

static void setupMode(DemoMode mode, EffectManager& manager,
                      EdgeLightingEffect& edgeLight,
                      MagneticWaveEffect& wave1,
                      MagneticWaveEffect& wave2,
                      MagneticWaveEffect& wave3,
                      GLFWwindow* window) {
    manager.clearEffects();

    switch (mode) {
        case DemoMode::EdgeLighting:
            manager.addEffect(&edgeLight);
            break;
        case DemoMode::MagneticWaveSingle:
            manager.addEffect(&wave1);
            break;
        case DemoMode::MagneticWaveMulti:
            manager.addEffect(&wave1);
            manager.addEffect(&wave2);
            manager.addEffect(&wave3);
            break;
        case DemoMode::AllEffects:
            manager.addEffect(&wave1);
            manager.addEffect(&wave2);
            manager.addEffect(&wave3);
            manager.addEffect(&edgeLight);
            break;
        default:
            break;
    }

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    manager.init(fbW, fbH);

    std::string title = "Edge Lighting Demo - [";
    title += modeName(mode);
    title += "]  (Left/Right arrows to switch)";
    glfwSetWindowTitle(window, title.c_str());
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_RIGHT) {
        int next = (static_cast<int>(s_currentMode) + 1) % static_cast<int>(DemoMode::Count);
        s_currentMode = static_cast<DemoMode>(next);
        s_modeChanged = true;
    } else if (key == GLFW_KEY_LEFT) {
        int count = static_cast<int>(DemoMode::Count);
        int prev = (static_cast<int>(s_currentMode) - 1 + count) % count;
        s_currentMode = static_cast<DemoMode>(prev);
        s_modeChanged = true;
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Edge Lighting Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glfwSetKeyCallback(window, keyCallback);

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    EdgeLightingEffect edgeLight;
    MagneticWaveEffect wave1, wave2, wave3;

    wave1.setSpeed(1.5f);
    wave1.setAmplitude(25.0f);
    wave1.setWaveCount(5);
    wave1.setLineWidth(5.0f);
    wave1.setColorShift(0.0f);

    wave2.setSpeed(2.0f);
    wave2.setAmplitude(18.0f);
    wave2.setWaveCount(4);
    wave2.setLineWidth(4.0f);
    wave2.setColorShift(0.33f);

    wave3.setSpeed(1.0f);
    wave3.setAmplitude(35.0f);
    wave3.setWaveCount(7);
    wave3.setLineWidth(6.0f);
    wave3.setColorShift(0.66f);

    EffectManager manager;
    double lastNotifyTime = glfwGetTime();
    srand(static_cast<unsigned int>(time(nullptr)));

    setupMode(s_currentMode, manager, edgeLight, wave1, wave2, wave3, window);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        if (dt > 0.05f) dt = 0.05f;

        int newFbW, newFbH;
        glfwGetFramebufferSize(window, &newFbW, &newFbH);
        if (newFbW != fbW || newFbH != fbH) {
            fbW = newFbW;
            fbH = newFbH;
            manager.onResize(fbW, fbH);
        }

        if (s_modeChanged) {
            setupMode(s_currentMode, manager, edgeLight, wave1, wave2, wave3, window);
            s_modeChanged = false;
        }

        if (s_currentMode == DemoMode::EdgeLighting || s_currentMode == DemoMode::AllEffects) {
            if (currentTime - lastNotifyTime > 4.0 + (rand() % 5)) {
                edgeLight.triggerNotification();
                lastNotifyTime = currentTime;
            }
        }

        glViewport(0, 0, fbW, fbH);
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        manager.update(dt);
        manager.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}