#include "EdgeLightingEffect/NeonWaveRingEffect.h"
#include "core/Shader.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static const float PI = 3.14159265f;

struct LineVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct NeonWaveRingEffect::Impl {
    Shader lineShader;
    glm::mat4 projection;

    float elapsedTime = 0.0f;
    float radius = 200.0f;
    float amplitude = 30.0f;
    float speed = 2.0f;
    float waveCount = 5.0f;
    float lineWidth = 6.0f;
    float glowWidth = 35.0f;
    float glowIntensity = 1.0f;

    unsigned int coreVAO = 0, coreVBO = 0;
    int coreVertexCount = 0;
    unsigned int glowVAO = 0, glowVBO = 0;
    int glowVertexCount = 0;
    unsigned int outerGlowVAO = 0, outerGlowVBO = 0;
    int outerGlowVertexCount = 0;

    int fbWidth = 0, fbHeight = 0;

    ~Impl() {
        if (coreVAO) glDeleteVertexArrays(1, &coreVAO);
        if (coreVBO) glDeleteBuffers(1, &coreVBO);
        if (glowVAO) glDeleteVertexArrays(1, &glowVAO);
        if (glowVBO) glDeleteBuffers(1, &glowVBO);
        if (outerGlowVAO) glDeleteVertexArrays(1, &outerGlowVAO);
        if (outerGlowVBO) glDeleteBuffers(1, &outerGlowVBO);
    }

    void updateProjection() {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        projection = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
    }

    glm::vec4 getGradientColor(float t) const {
        struct Stop { float pos; glm::vec3 color; };
        static const Stop stops[] = {
            {0.00f, glm::vec3(0.0f, 1.0f, 1.0f)},   // cyan
            {0.25f, glm::vec3(0.5f, 0.0f, 1.0f)},   // purple
            {0.50f, glm::vec3(1.0f, 0.0f, 0.5f)},   // pink
            {0.75f, glm::vec3(1.0f, 0.5f, 0.0f)},   // orange
            {1.00f, glm::vec3(0.0f, 1.0f, 1.0f)},   // cyan
        };
        static const int numStops = sizeof(stops) / sizeof(stops[0]);

        float shiftedT = fmod(t + elapsedTime * 0.05f, 1.0f);
        if (shiftedT < 0.0f) shiftedT += 1.0f;

        glm::vec3 col;
        if (shiftedT <= stops[0].pos) col = stops[0].color;
        else if (shiftedT >= stops[numStops - 1].pos) col = stops[numStops - 1].color;
        else {
            for (int i = 0; i < numStops - 1; i++) {
                if (shiftedT >= stops[i].pos && shiftedT < stops[i + 1].pos) {
                    float s = (shiftedT - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
                    col = glm::mix(stops[i].color, stops[i + 1].color, s);
                    break;
                }
            }
        }

        return glm::vec4(col, 1.0f);
    }

    void buildRingGeometry(float baseRadius, float amp, float width,
                           float alphaScale, std::vector<LineVertex>& vertices) {
        const int SEGMENTS = 200;
        float halfWidth = width * 0.5f;

        vertices.clear();
        vertices.reserve((SEGMENTS + 1) * 2);

        for (int i = 0; i <= SEGMENTS; i++) {
            float t = static_cast<float>(i) / SEGMENTS;
            float angle = t * 2.0f * PI;

            float wave = amp * sin(angle * waveCount + elapsedTime * speed);
            float r = baseRadius + wave;

            glm::vec2 pos(r * cos(angle), r * sin(angle));
            glm::vec2 normal(cos(angle), sin(angle));

            glm::vec4 color = getGradientColor(t);
            color.a *= alphaScale;

            glm::vec2 offset = normal * halfWidth;

            LineVertex v1, v2;
            v1.x = pos.x + offset.x;
            v1.y = pos.y + offset.y;
            v1.u = t;
            v1.v = 1.0f;
            v1.r = color.r;
            v1.g = color.g;
            v1.b = color.b;
            v1.a = color.a;

            v2.x = pos.x - offset.x;
            v2.y = pos.y - offset.y;
            v2.u = t;
            v2.v = -1.0f;
            v2.r = color.r;
            v2.g = color.g;
            v2.b = color.b;
            v2.a = color.a;

            vertices.push_back(v1);
            vertices.push_back(v2);
        }

        vertices.push_back(vertices[0]);
        vertices.push_back(vertices[1]);
    }

    void setupVAO(unsigned int vao, unsigned int vbo) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
};

NeonWaveRingEffect::NeonWaveRingEffect()
    : m_impl(new Impl()) {}

NeonWaveRingEffect::~NeonWaveRingEffect() { delete m_impl; }

bool NeonWaveRingEffect::init(int fbWidth, int fbHeight) {
    auto& p = *m_impl;

    if (!p.lineShader.load("assets/shaders/line.vert", "assets/shaders/line.frag")) {
        std::cerr << "Failed to load line shaders" << std::endl;
        return false;
    }

    p.fbWidth = fbWidth;
    p.fbHeight = fbHeight;
    p.updateProjection();

    glGenVertexArrays(1, &p.coreVAO);
    glGenBuffers(1, &p.coreVBO);
    p.setupVAO(p.coreVAO, p.coreVBO);

    glGenVertexArrays(1, &p.glowVAO);
    glGenBuffers(1, &p.glowVBO);
    p.setupVAO(p.glowVAO, p.glowVBO);

    glGenVertexArrays(1, &p.outerGlowVAO);
    glGenBuffers(1, &p.outerGlowVBO);
    p.setupVAO(p.outerGlowVAO, p.outerGlowVBO);

    return true;
}

void NeonWaveRingEffect::update(float deltaTime) {
    auto& p = *m_impl;
    p.elapsedTime += deltaTime;

    std::vector<LineVertex> vertices;

    // Core line
    p.buildRingGeometry(p.radius, p.amplitude, p.lineWidth, 1.0f, vertices);
    p.coreVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.coreVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);

    // Inner glow
    float gi = p.glowIntensity;
    p.buildRingGeometry(p.radius, p.amplitude, p.glowWidth, 0.3f * gi, vertices);
    p.glowVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.glowVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);

    // Outer glow (wider, fainter)
    p.buildRingGeometry(p.radius, p.amplitude, p.glowWidth * 2.0f, 0.1f * gi, vertices);
    p.outerGlowVertexCount = static_cast<int>(vertices.size());
    glBindBuffer(GL_ARRAY_BUFFER, p.outerGlowVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);
}

void NeonWaveRingEffect::render() {
    auto& p = *m_impl;

    glEnable(GL_BLEND);
    p.lineShader.use();
    p.lineShader.setMat4("uProjection", p.projection);

    float gi = p.glowIntensity;

    // Outer glow (widest, faintest) - draw first
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    p.lineShader.setFloat("uAlphaScale", 0.4f * gi);
    glBindVertexArray(p.outerGlowVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.outerGlowVertexCount);

    // Inner glow
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    p.lineShader.setFloat("uAlphaScale", 0.6f * gi);
    glBindVertexArray(p.glowVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.glowVertexCount);

    // Core line (brightest) - draw on top
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    p.lineShader.setFloat("uAlphaScale", 1.0f);
    glBindVertexArray(p.coreVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.coreVertexCount);
}

void NeonWaveRingEffect::onResize(int fbWidth, int fbHeight) {
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;
    m_impl->updateProjection();
}

void NeonWaveRingEffect::setRadius(float r) { m_impl->radius = r; }
void NeonWaveRingEffect::setAmplitude(float a) { m_impl->amplitude = a; }
void NeonWaveRingEffect::setSpeed(float s) { m_impl->speed = s; }
void NeonWaveRingEffect::setWaveCount(float c) { m_impl->waveCount = c; }
void NeonWaveRingEffect::setLineWidth(float w) { m_impl->lineWidth = w; }
void NeonWaveRingEffect::setGlowWidth(float w) { m_impl->glowWidth = w; }
void NeonWaveRingEffect::setGlowIntensity(float i) { m_impl->glowIntensity = i; }
