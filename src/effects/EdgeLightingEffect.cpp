#include "EdgeLightingEffect/EdgeLightingEffect.h"
#include "core/Shader.h"
#include "perimeter/Perimeter.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct LineVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct EdgeLightingEffect::Impl {
    Perimeter* perimeter = nullptr;
    Shader lineShader;
    glm::mat4 projection;

    float elapsedTime = 0.0f;
    float notifTimer = 2.0f;
    float shakeIntensity = 0.0f;
    float pulseIntensity = 0.0f;

    float baseGlowWidth = 50.0f;
    float baseCoreWidth = 10.0f;
    float glowAlphaScale = 0.15f;

    unsigned int coreVAO = 0, coreVBO = 0;
    int coreVertexCount = 0;
    unsigned int glowVAO = 0, glowVBO = 0;
    int glowVertexCount = 0;

    int fbWidth = 0, fbHeight = 0;

    ~Impl() {
        delete perimeter;
        if (coreVAO) glDeleteVertexArrays(1, &coreVAO);
        if (coreVBO) glDeleteBuffers(1, &coreVBO);
        if (glowVAO) glDeleteVertexArrays(1, &glowVAO);
        if (glowVBO) glDeleteBuffers(1, &glowVBO);
    }

    void updateProjection() {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        projection = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
    }

    glm::vec4 getGradientColor(float t, float time, float pulse) const {
        float shiftedT = fmod(t + time * 0.06f, 1.0f);

        struct MagStop { float pos; glm::vec3 color; };
        static const MagStop stops[] = {
            {0.00f, glm::vec3(0.0f, 0.8f, 1.0f)},
            {0.33f, glm::vec3(0.0f, 0.3f, 1.0f)},
            {0.66f, glm::vec3(0.4f, 0.0f, 0.8f)},
            {1.00f, glm::vec3(0.0f, 0.8f, 1.0f)},
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

        if (pulse > 0.01f) {
            float flash = pulse * (0.5f + 0.5f * sin(t * 20.0f + time * 10.0f));
            col = glm::mix(col, glm::vec3(1.0f), flash * 0.6f);
            alpha = glm::min(alpha + flash * 0.3f, 1.0f);
        }

        return glm::vec4(col, alpha);
    }

    void buildLineGeometry(float lineWidth, std::vector<LineVertex>& vertices) {
        const int SEGMENTS = 400;
        float totalLength = perimeter->getTotalLength();
        float halfWidth = lineWidth * 0.5f;

        vertices.clear();
        vertices.reserve((SEGMENTS + 1) * 2);

        for (int i = 0; i <= SEGMENTS; i++) {
            float t = static_cast<float>(i) / SEGMENTS;
            float dist = t * totalLength;

            PerimeterPoint pp = perimeter->getPointAtDistance(dist);
            glm::vec4 color = getGradientColor(t, elapsedTime, pulseIntensity);

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

        // Close the triangle strip back to the start to eliminate head-tail gap
        vertices.push_back(vertices[0]);
        vertices.push_back(vertices[1]);
    }
};

EdgeLightingEffect::EdgeLightingEffect()
    : m_width(0), m_height(0), m_cornerRadius(40.0f), m_impl(new Impl()) {}

EdgeLightingEffect::~EdgeLightingEffect() { delete m_impl; }

bool EdgeLightingEffect::init(int fbWidth, int fbHeight) {
    m_width = fbWidth;
    m_height = fbHeight;

    if (!m_impl->lineShader.load("assets/shaders/line.vert", "assets/shaders/line.frag")) {
        std::cerr << "Failed to load line shaders" << std::endl;
        return false;
    }

    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;
    m_impl->perimeter = new Perimeter(fbWidth * 0.9f, fbHeight * 0.9f, m_cornerRadius);
    m_impl->updateProjection();

    // Core line VAO
    glGenVertexArrays(1, &m_impl->coreVAO);
    glGenBuffers(1, &m_impl->coreVBO);
    glBindVertexArray(m_impl->coreVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_impl->coreVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Glow line VAO
    glGenVertexArrays(1, &m_impl->glowVAO);
    glGenBuffers(1, &m_impl->glowVBO);
    glBindVertexArray(m_impl->glowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_impl->glowVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return true;
}

void EdgeLightingEffect::update(float deltaTime) {
    auto& p = *m_impl;
    p.elapsedTime += deltaTime;

    p.notifTimer -= deltaTime;
    if (p.notifTimer <= 0.0f) {
        p.shakeIntensity = 15.0f;
        p.pulseIntensity = 1.5f;
        p.notifTimer = 4.0f + (static_cast<float>(rand()) / RAND_MAX) * 5.0f;
    }

    p.shakeIntensity *= 1.0f - 4.0f * deltaTime;
    if (p.shakeIntensity < 0.0f) p.shakeIntensity = 0.0f;
    p.pulseIntensity *= 1.0f - 1.2f * deltaTime;
    if (p.pulseIntensity < 0.0f) p.pulseIntensity = 0.0f;

    float glowPulse = 1.0f + 0.3f * sin(p.elapsedTime * 1.8f + 2.0f);
    float currentGlowWidth = p.baseGlowWidth * (glowPulse + p.pulseIntensity * 2.0f);
    p.glowAlphaScale = 0.08f + 0.12f * sin(p.elapsedTime * 1.2f + 3.0f) + p.pulseIntensity * 0.6f;

    std::vector<LineVertex> coreVertices;
    p.buildLineGeometry(p.baseCoreWidth, coreVertices);
    p.coreVertexCount = static_cast<int>(coreVertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, p.coreVBO);
    glBufferData(GL_ARRAY_BUFFER, coreVertices.size() * sizeof(LineVertex), coreVertices.data(), GL_DYNAMIC_DRAW);

    std::vector<LineVertex> glowVertices;
    p.buildLineGeometry(currentGlowWidth, glowVertices);
    p.glowVertexCount = static_cast<int>(glowVertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, p.glowVBO);
    glBufferData(GL_ARRAY_BUFFER, glowVertices.size() * sizeof(LineVertex), glowVertices.data(), GL_DYNAMIC_DRAW);
}

void EdgeLightingEffect::render() {
    auto& p = *m_impl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    p.lineShader.use();

    glm::mat4 proj = p.projection;
    if (p.shakeIntensity > 0.5f) {
        float sx = sin(p.elapsedTime * 137.0f) * p.shakeIntensity * 0.5f;
        float sy = sin(p.elapsedTime * 251.0f) * p.shakeIntensity * 0.5f;
        proj = glm::translate(proj, glm::vec3(sx, sy, 0.0f));
    }
    p.lineShader.setMat4("uProjection", proj);

    p.lineShader.setFloat("uAlphaScale", p.glowAlphaScale);
    glBindVertexArray(p.glowVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.glowVertexCount);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    p.lineShader.setFloat("uAlphaScale", 1.0f);
    glBindVertexArray(p.coreVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.coreVertexCount);
}

void EdgeLightingEffect::onResize(int fbWidth, int fbHeight) {
    m_width = fbWidth;
    m_height = fbHeight;
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;
    m_impl->updateProjection();
}

void EdgeLightingEffect::triggerNotification() {
    m_impl->shakeIntensity = 15.0f;
    m_impl->pulseIntensity = 1.5f;
}

void EdgeLightingEffect::setCornerRadius(float radius) {
    m_cornerRadius = radius;
}

void EdgeLightingEffect::setGlowWidth(float width) {
    m_impl->baseGlowWidth = width;
}

void EdgeLightingEffect::setCoreWidth(float width) {
    m_impl->baseCoreWidth = width;
}