#include <iostream>
#include <string>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "EdgeLightingEffect/EffectManager.h"
#include "EdgeLightingEffect/EdgePulseEffect.h"
#include "EdgeLightingEffect/MagneticWaveEffect.h"
#include "EdgeLightingEffect/MagneticRectangleEffect.h"
#include "EdgeLightingEffect/NeonWaveRingEffect.h"
#include "EdgeLightingEffect/AudioReactiveEdgeLightingEffect.h"

struct DemoEffects {
    EdgePulseEffect edgeLight;
    MagneticWaveEffect wave1, wave2, wave3;
    MagneticRectangleEffect rectWave[4];
    NeonWaveRingEffect neonRing;
    AudioReactiveEdgeLightingEffect audioReactive;
};

enum class DemoMode {
    EdgePulse,
    MagneticWaveMulti,
    MagneticRectangle,
    NeonWaveRing,
    AudioReactive,
    Count
};

static DemoMode s_currentMode = DemoMode::EdgePulse;
static bool s_modeChanged = true;

static const char* modeName(DemoMode mode) {
    switch (mode) {
        case DemoMode::EdgePulse:          return "Edge Pulse";
        case DemoMode::MagneticWaveMulti:  return "Magnetic Waves (3)";
        case DemoMode::MagneticRectangle:  return "Magnetic Rectangle";
        case DemoMode::NeonWaveRing:       return "Neon Wave Ring";
        case DemoMode::AudioReactive:      return "Audio Reactive Edge Lighting";
        default:                           return "Unknown";
    }
}

static void setupMode(DemoMode mode, EffectManager& manager,
                      DemoEffects& effects, GLFWwindow* window) {
    manager.clearEffects();

    switch (mode) {
        case DemoMode::EdgePulse:
            manager.addEffect(&effects.edgeLight);
            break;
        case DemoMode::MagneticWaveMulti:
            manager.addEffect(&effects.wave1);
            manager.addEffect(&effects.wave2);
            manager.addEffect(&effects.wave3);
            break;
        case DemoMode::MagneticRectangle:
            for (int i = 0; i < 4; i++)
                manager.addEffect(&effects.rectWave[i]);
            break;
        case DemoMode::NeonWaveRing:
            manager.addEffect(&effects.neonRing);
            break;
        case DemoMode::AudioReactive:
            manager.addEffect(&effects.audioReactive);
            break;
        default:
            break;
    }

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    manager.init(fbW, fbH);

    std::string title = "Edge Pulse Demo - [";
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Edge Pulse Demo", nullptr, nullptr);
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

    DemoEffects effects;

    effects.wave1.setSpeed(1.5f);
    effects.wave1.setAmplitude(25.0f);
    effects.wave1.setWaveCount(5);
    effects.wave1.setLineWidth(5.0f);
    effects.wave1.setColorShift(0.0f);

    effects.wave2.setSpeed(2.0f);
    effects.wave2.setAmplitude(18.0f);
    effects.wave2.setWaveCount(4);
    effects.wave2.setLineWidth(4.0f);
    effects.wave2.setColorShift(0.33f);

    effects.wave3.setSpeed(1.0f);
    effects.wave3.setAmplitude(35.0f);
    effects.wave3.setWaveCount(7);
    effects.wave3.setLineWidth(6.0f);
    effects.wave3.setColorShift(0.66f);

    for (int i = 0; i < 4; i++) {
        float t = i / 3.0f;
        effects.rectWave[i].setSpeed(6.0f + t * 10.0f);
        effects.rectWave[i].setAmplitude(15.0f + t * 10.0f);
        effects.rectWave[i].setLineWidth(5.0f - t * 2.0f);
        effects.rectWave[i].setCornerRadius(40.0f);
        effects.rectWave[i].setColorShift(t * 0.5f);
        effects.rectWave[i].setScale(0.3f + t * 0.35f);
    }

    effects.neonRing.setRadius(200.0f);
    effects.neonRing.setAmplitude(25.0f);
    effects.neonRing.setSpeed(2.5f);
    effects.neonRing.setWaveCount(6.0f);
    effects.neonRing.setLineWidth(6.0f);
    effects.neonRing.setGlowWidth(150.0f);
    effects.neonRing.setGlowIntensity(1.5f);

    effects.audioReactive.setCornerRadius(40.0f);
    effects.audioReactive.setLineWidth(20.0f);
    effects.audioReactive.setGlowIntensity(1.20f);
    effects.audioReactive.setSensitivity(1.0f);

    // Simulated audio state
    float audioPhase[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    double lastKickTime = glfwGetTime();
    float spectrum[64];

    EffectManager manager;
    double lastNotifyTime = glfwGetTime();
    srand(static_cast<unsigned int>(time(nullptr)));

    setupMode(s_currentMode, manager, effects, window);

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
            setupMode(s_currentMode, manager, effects, window);
            s_modeChanged = false;
        }

        if (s_currentMode == DemoMode::EdgePulse) {
            if (currentTime - lastNotifyTime > 4.0 + (rand() % 5)) {
                effects.edgeLight.triggerNotification();
                lastNotifyTime = currentTime;
            }
        }

        if (s_currentMode == DemoMode::AudioReactive) {
            float f = static_cast<float>(currentTime);

            // Random beat every 0.5-2.5 seconds
            bool kick = false;
            if (currentTime - lastKickTime > 0.5f + (rand() % 20) * 0.1f) {
                kick = true;
                lastKickTime = currentTime;
            }

            // Bass spikes on kick, drifts between kicks
            static float bassTarget = 0.3f;
            static float bassCurrent = 0.3f;
            if (kick) {
                bassTarget = 0.8f + (rand() % 20) * 0.01f;
            } else if (rand() % 120 == 0) {
                bassTarget = 0.2f + (rand() % 60) * 0.01f;
            }
            bassCurrent += (bassTarget - bassCurrent) * 0.1f;
            float bass = bassCurrent;

            // Mid: random burst pattern
            static float midTarget = 0.2f;
            static float midCurrent = 0.2f;
            if (rand() % 60 == 0) midTarget = 0.1f + (rand() % 60) * 0.01f;
            midCurrent += (midTarget - midCurrent) * 0.08f;
            float mid = midCurrent;

            // Treble: random shimmer
            static float trebTarget = 0.1f;
            static float trebCurrent = 0.1f;
            if (rand() % 45 == 0) trebTarget = 0.1f + (rand() % 50) * 0.01f;
            trebCurrent += (trebTarget - trebCurrent) * 0.15f;
            float treble = trebCurrent;
            if (kick) treble += 0.3f;

            // Simulated spectrum (64 bands) — random walk per band
            static float specVal[64];
            static bool specInit = false;
            if (!specInit) {
                for (int i = 0; i < 64; i++) specVal[i] = 0.5f;
                specInit = true;
            }
            for (int i = 0; i < 64; i++) {
                float t = static_cast<float>(i) / 64.0f;
                float dr = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.06f;
                specVal[i] += dr;
                if (specVal[i] < 0.4f) specVal[i] = 0.4f;
                if (specVal[i] > 0.95f) specVal[i] = 0.95f;
                float band = specVal[i];
                if (t < 0.2f) band = std::min(band + bass * 0.3f, 1.0f);
                if (kick) band = std::min(band + 0.4f, 1.0f);
                spectrum[i] = band;
            }

            effects.audioReactive.setSpectrum(spectrum, 64);
            effects.audioReactive.setBass(bass);
            effects.audioReactive.setMid(mid);
            effects.audioReactive.setTreble(treble);
            effects.audioReactive.setKick(kick);
            effects.audioReactive.setVolume(0.3f + 0.7f * bass);
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