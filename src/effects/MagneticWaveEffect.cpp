#include "EdgeLightingEffect/MagneticWaveEffect.h"
#include "core/Shader.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct LineVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

struct MagneticWaveEffect::Impl {
    Shader shader;
    glm::mat4 projection;

    unsigned int VAO = 0, VBO = 0;
    int vertexCount = 0;

    int fbWidth = 0, fbHeight = 0;
    float elapsedTime = 0.0f;

    float speed = 1.5f;
    float amplitude = 30.0f;
    int waveCount = 6;
    float lineWidth = 6.0f;
    float colorShift = 0.0f;

    int segments = 200;

    ~Impl() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
    }

    void buildGeometry(std::vector<LineVertex>& vertices) {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        float halfH = h * 0.5f;
        float spacing = h / (waveCount + 1);

        vertices.clear();
        vertices.reserve((segments + 1) * 2 * waveCount);

        for (int line = 0; line < waveCount; line++) {
            float baseY = -halfH + spacing * (line + 1);
            float phaseOffset = line * 1.8f;
            float colorPhase = line / static_cast<float>(waveCount);

            for (int i = 0; i <= segments; i++) {
                float t = static_cast<float>(i) / segments;
                float x = -w * 0.5f + t * w;

                // Multi-wave displacement
                float wave = sin(t * 8.0f + elapsedTime * speed + phaseOffset) * amplitude;
                wave += sin(t * 3.0f + elapsedTime * speed * 0.7f + phaseOffset * 1.5f) * amplitude * 0.4f;

                float y = baseY + wave;

                // Color: magnetic palette flowing along the line and over time
                float ct = fmod(t + elapsedTime * 0.05f + colorPhase + colorShift, 1.0f);
                glm::vec3 col;
                if (ct < 0.33f) {
                    float s = ct / 0.33f;
                    col = glm::mix(glm::vec3(0.0f, 0.8f, 1.0f), glm::vec3(0.0f, 0.3f, 1.0f), s);
                } else if (ct < 0.66f) {
                    float s = (ct - 0.33f) / 0.33f;
                    col = glm::mix(glm::vec3(0.0f, 0.3f, 1.0f), glm::vec3(0.4f, 0.0f, 0.8f), s);
                } else {
                    float s = (ct - 0.66f) / 0.34f;
                    col = glm::mix(glm::vec3(0.4f, 0.0f, 0.8f), glm::vec3(0.0f, 0.8f, 1.0f), s);
                }

                float alpha = 0.4f + 0.4f * sin(t * 3.14159f + elapsedTime * 0.5f + phaseOffset);

                float halfW = lineWidth * 0.5f;

                LineVertex v1, v2;
                v1.x = x;
                v1.y = y - halfW;
                v1.u = t;
                v1.v = -1.0f;
                v1.r = col.r;
                v1.g = col.g;
                v1.b = col.b;
                v1.a = alpha;

                v2.x = x;
                v2.y = y + halfW;
                v2.u = t;
                v2.v = 1.0f;
                v2.r = col.r;
                v2.g = col.g;
                v2.b = col.b;
                v2.a = alpha;

                vertices.push_back(v1);
                vertices.push_back(v2);
            }
        }
    }
};

MagneticWaveEffect::MagneticWaveEffect()
    : m_impl(new Impl()) {}

MagneticWaveEffect::~MagneticWaveEffect() { delete m_impl; }

bool MagneticWaveEffect::init(int fbWidth, int fbHeight) {
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;

    if (!m_impl->shader.load("assets/shaders/line.vert", "assets/shaders/line.frag")) {
        std::cerr << "Failed to load wave shaders" << std::endl;
        return false;
    }

    float w = static_cast<float>(fbWidth);
    float h = static_cast<float>(fbHeight);
    m_impl->projection = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);

    glGenVertexArrays(1, &m_impl->VAO);
    glGenBuffers(1, &m_impl->VBO);
    glBindVertexArray(m_impl->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_impl->VBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return true;
}

void MagneticWaveEffect::update(float deltaTime) {
    m_impl->elapsedTime += deltaTime;

    std::vector<LineVertex> vertices;
    m_impl->buildGeometry(vertices);
    m_impl->vertexCount = static_cast<int>(vertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_impl->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);
}

void MagneticWaveEffect::render() {
    auto& p = *m_impl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    p.shader.use();
    p.shader.setMat4("uProjection", p.projection);
    p.shader.setFloat("uAlphaScale", 1.0f);

    glBindVertexArray(p.VAO);
    // Each wave line is a separate triangle strip
    int vertsPerLine = (p.segments + 1) * 2;
    for (int i = 0; i < p.waveCount; i++) {
        glDrawArrays(GL_TRIANGLE_STRIP, i * vertsPerLine, vertsPerLine);
    }
}

void MagneticWaveEffect::onResize(int fbWidth, int fbHeight) {
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;
    float w = static_cast<float>(fbWidth);
    float h = static_cast<float>(fbHeight);
    m_impl->projection = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
}

void MagneticWaveEffect::setSpeed(float speed) { m_impl->speed = speed; }
void MagneticWaveEffect::setAmplitude(float amp) { m_impl->amplitude = amp; }
void MagneticWaveEffect::setWaveCount(int count) { m_impl->waveCount = count > 0 ? count : 1; }
void MagneticWaveEffect::setLineWidth(float width) { m_impl->lineWidth = width; }
void MagneticWaveEffect::setColorShift(float shift) { m_impl->colorShift = shift; }