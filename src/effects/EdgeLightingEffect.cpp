#include "EdgeLightingEffect.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <glad/glad.h>

EdgeLightingEffect::EdgeLightingEffect()
    : m_app(nullptr), m_perimeter(nullptr),
      m_elapsedTime(0.0f),
      m_notifTimer(2.0f), m_shakeIntensity(0.0f), m_pulseIntensity(0.0f),
      m_baseGlowWidth(50.0f), m_baseCoreWidth(10.0f), m_glowAlphaScale(0.15f),
      m_coreVAO(0), m_coreVBO(0), m_coreVertexCount(0),
      m_glowVAO(0), m_glowVBO(0), m_glowVertexCount(0) {}

EdgeLightingEffect::~EdgeLightingEffect() {
    delete m_perimeter;
    if (m_coreVAO) glDeleteVertexArrays(1, &m_coreVAO);
    if (m_coreVBO) glDeleteBuffers(1, &m_coreVBO);
    if (m_glowVAO) glDeleteVertexArrays(1, &m_glowVAO);
    if (m_glowVBO) glDeleteBuffers(1, &m_glowVBO);
}

bool EdgeLightingEffect::init(Application* app) {
    m_app = app;

    if (!m_lineShader.load("assets/shaders/line.vert", "assets/shaders/line.frag")) {
        std::cerr << "Failed to load line shaders" << std::endl;
        return false;
    }

    float w = static_cast<float>(m_app->getWidth());
    float h = static_cast<float>(m_app->getHeight());
    m_perimeter = new Perimeter(w * 0.9f, h * 0.9f);

    updateProjection();

    // Core line VAO
    glGenVertexArrays(1, &m_coreVAO);
    glGenBuffers(1, &m_coreVBO);
    glBindVertexArray(m_coreVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_coreVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Glow line VAO
    glGenVertexArrays(1, &m_glowVAO);
    glGenBuffers(1, &m_glowVBO);
    glBindVertexArray(m_glowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_glowVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return true;
}

void EdgeLightingEffect::updateProjection() {
    float w = static_cast<float>(m_app->getWidth());
    float h = static_cast<float>(m_app->getHeight());
    m_projection = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
}

glm::vec4 EdgeLightingEffect::getGradientColor(float t, float time, float pulse) const {
    float shiftedT = fmod(t + time * 0.06f, 1.0f);

    // Magnetic field color palette: cyan -> blue -> purple -> white flash on pulse
    struct MagStop { float pos; glm::vec3 color; };
    static const MagStop stops[] = {
        {0.00f, glm::vec3(0.0f, 0.8f, 1.0f)},   // Cyan
        {0.33f, glm::vec3(0.0f, 0.3f, 1.0f)},   // Blue
        {0.66f, glm::vec3(0.4f, 0.0f, 0.8f)},   // Purple
        {1.00f, glm::vec3(0.0f, 0.8f, 1.0f)},   // Cyan again
    };
    static const int numStops = sizeof(stops) / sizeof(stops[0]);

    float alpha = 0.7f + 0.3f * sin(t * 12.0f + time * 2.0f);

    glm::vec3 col;
    if (shiftedT <= 0.0f) col = stops[0].color;
    else if (shiftedT >= 1.0f) col = stops[numStops - 1].color;
    else {
        for (int i = 0; i < numStops - 1; i++) {
            if (shiftedT >= stops[i].pos && shiftedT < stops[i + 1].pos) {
                float s = (shiftedT - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
                col = glm::mix(stops[i].color, stops[i + 1].color, s);
                break;
            }
        }
    }

    // Pulse flash - wash to white
    if (pulse > 0.01f) {
        float flash = pulse * (0.5f + 0.5f * sin(t * 20.0f + time * 10.0f));
        col = glm::mix(col, glm::vec3(1.0f), flash * 0.6f);
        alpha = glm::min(alpha + flash * 0.3f, 1.0f);
    }

    return glm::vec4(col, alpha);
}

void EdgeLightingEffect::buildLineGeometry(float lineWidth, std::vector<LineVertex>& vertices) {
    const int SEGMENTS = 400;
    float totalLength = m_perimeter->getTotalLength();
    float halfWidth = lineWidth * 0.5f;
    float w = static_cast<float>(m_app->getWidth()) * 0.45f;
    float h = static_cast<float>(m_app->getHeight()) * 0.45f;

    vertices.clear();
    vertices.reserve((SEGMENTS + 1) * 2);

    for (int i = 0; i <= SEGMENTS; i++) {
        float t = static_cast<float>(i) / SEGMENTS;
        float dist = t * totalLength;

        PerimeterPoint pp = m_perimeter->getPointAtDistance(dist);
        glm::vec4 color = getGradientColor(t, m_elapsedTime, m_pulseIntensity);

        // Static rectangle - no displacement
        glm::vec2 basePos = pp.position;
        glm::vec2 offset = pp.normal * halfWidth;

        LineVertex v1, v2;
        v1.x = basePos.x + offset.x;
        v1.y = basePos.y + offset.y;
        v1.u = t;
        v1.v = 1.0f;
        v1.r = color.r;
        v1.g = color.g;
        v1.b = color.b;
        v1.a = color.a;

        v2.x = basePos.x - offset.x;
        v2.y = basePos.y - offset.y;
        v2.u = t;
        v2.v = -1.0f;
        v2.r = color.r;
        v2.g = color.g;
        v2.b = color.b;
        v2.a = color.a;

        vertices.push_back(v1);
        vertices.push_back(v2);
    }
}

void EdgeLightingEffect::update(float deltaTime) {
    m_elapsedTime += deltaTime;

    // Notification timer
    m_notifTimer -= deltaTime;
    if (m_notifTimer <= 0.0f) {
        m_shakeIntensity = 15.0f;
        m_pulseIntensity = 1.5f;
        m_notifTimer = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 5.0f;
    }

    // Decay shake and pulse (slower decay for longer effect)
    m_shakeIntensity *= 1.0f - 4.0f * deltaTime;
    if (m_shakeIntensity < 0.0f) m_shakeIntensity = 0.0f;
    m_pulseIntensity *= 1.0f - 1.2f * deltaTime;
    if (m_pulseIntensity < 0.0f) m_pulseIntensity = 0.0f;

    float glowPulse = 1.0f + 0.3f * sin(m_elapsedTime * 1.8f + 2.0f);
    float currentGlowWidth = m_baseGlowWidth * (glowPulse + m_pulseIntensity * 2.0f);
    m_glowAlphaScale = 0.08f + 0.12f * sin(m_elapsedTime * 1.2f + 3.0f) + m_pulseIntensity * 0.6f;

    std::vector<LineVertex> coreVertices;
    buildLineGeometry(m_baseCoreWidth, coreVertices);
    m_coreVertexCount = static_cast<int>(coreVertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_coreVBO);
    glBufferData(GL_ARRAY_BUFFER, coreVertices.size() * sizeof(LineVertex), coreVertices.data(), GL_DYNAMIC_DRAW);

    std::vector<LineVertex> glowVertices;
    buildLineGeometry(currentGlowWidth, glowVertices);
    m_glowVertexCount = static_cast<int>(glowVertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_glowVBO);
    glBufferData(GL_ARRAY_BUFFER, glowVertices.size() * sizeof(LineVertex), glowVertices.data(), GL_DYNAMIC_DRAW);
}

void EdgeLightingEffect::render(float deltaTime) {
    int w = m_app->getWidth();
    int h = m_app->getHeight();
    glViewport(0, 0, w, h);

    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    m_lineShader.use();

    // Apply screen shake to projection
    glm::mat4 proj = m_projection;
    if (m_shakeIntensity > 0.5f) {
        float sx = sin(m_elapsedTime * 137.0f) * m_shakeIntensity * 0.5f;
        float sy = sin(m_elapsedTime * 251.0f) * m_shakeIntensity * 0.5f;
        proj = glm::translate(proj, glm::vec3(sx, sy, 0.0f));
    }
    m_lineShader.setMat4("uProjection", proj);

    m_lineShader.setFloat("uAlphaScale", m_glowAlphaScale);
    glBindVertexArray(m_glowVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, m_glowVertexCount);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_lineShader.setFloat("uAlphaScale", 1.0f);
    glBindVertexArray(m_coreVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, m_coreVertexCount);

    glfwSwapBuffers(m_app->getWindow());
}