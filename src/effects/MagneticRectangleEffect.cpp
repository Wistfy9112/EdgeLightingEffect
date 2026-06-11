#include "EdgeLightingEffect/MagneticRectangleEffect.h"
#include "core/Shader.h"
#include "perimeter/Perimeter.h"
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

struct MagneticRectangleEffect::Impl {
    Shader shader;
    glm::mat4 projection;

    unsigned int VAO = 0, VBO = 0;
    int vertexCount = 0;

    int fbWidth = 0, fbHeight = 0;
    float elapsedTime = 0.0f;

    float speed = 1.2f;
    float amplitude = 15.0f;
    float lineWidth = 4.0f;
    float cornerRadius = 40.0f;
    float colorShift = 0.0f;
    float scale = 0.7f;

    int segments = 300;

    ~Impl() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
    }

    void buildGeometry(std::vector<LineVertex>& vertices) {
        float w = static_cast<float>(fbWidth);
        float h = static_cast<float>(fbHeight);
        float rectW = w * scale;
        float rectH = h * scale;
        float maxR = 0.49f * (rectW < rectH ? rectW : rectH);
        float R = cornerRadius < maxR ? cornerRadius : maxR;

        Perimeter perimeter(rectW, rectH, R);
        float totalLen = perimeter.getTotalLength();

        vertices.clear();
        vertices.reserve((segments + 1) * 2);

        for (int i = 0; i <= segments; i++) {
            float t = static_cast<float>(i) / segments;
            float dist = t * totalLen;

            PerimeterPoint pp = perimeter.getPointAtDistance(dist);

            // Wave displacement along normal direction
            float wave = sin(dist * 0.04f + elapsedTime * speed) * amplitude;
            wave += sin(dist * 0.08f + elapsedTime * speed * 0.6f) * amplitude * 0.35f;
            wave += sin(dist * 0.015f + elapsedTime * speed * 1.3f) * amplitude * 0.2f;

            glm::vec2 displaced = pp.position + pp.normal * wave;

            // Color: magnetic palette flowing along the perimeter
            float ct = fmod(t + elapsedTime * 0.04f + colorShift, 1.0f);
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

            float alpha = 0.5f + 0.4f * sin(dist * 0.02f + elapsedTime * 0.8f);
            if (alpha > 1.0f) alpha = 1.0f;

            float halfW = lineWidth * 0.5f;

            LineVertex v1, v2;
            glm::vec2 inward = displaced - pp.normal * halfW;
            glm::vec2 outward = displaced + pp.normal * halfW;

            v1.x = inward.x;
            v1.y = inward.y;
            v1.u = t;
            v1.v = -1.0f;
            v1.r = col.r;
            v1.g = col.g;
            v1.b = col.b;
            v1.a = alpha;

            v2.x = outward.x;
            v2.y = outward.y;
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
};

MagneticRectangleEffect::MagneticRectangleEffect()
    : m_impl(new Impl()) {}

MagneticRectangleEffect::~MagneticRectangleEffect() { delete m_impl; }

bool MagneticRectangleEffect::init(int fbWidth, int fbHeight) {
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;

    if (!m_impl->shader.load("assets/shaders/line.vert", "assets/shaders/line.frag")) {
        std::cerr << "Failed to load shaders" << std::endl;
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

void MagneticRectangleEffect::update(float deltaTime) {
    m_impl->elapsedTime += deltaTime;

    std::vector<LineVertex> vertices;
    m_impl->buildGeometry(vertices);
    m_impl->vertexCount = static_cast<int>(vertices.size());

    glBindBuffer(GL_ARRAY_BUFFER, m_impl->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_DYNAMIC_DRAW);
}

void MagneticRectangleEffect::render() {
    auto& p = *m_impl;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    p.shader.use();
    p.shader.setMat4("uProjection", p.projection);
    p.shader.setFloat("uAlphaScale", 1.0f);

    glBindVertexArray(p.VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, p.vertexCount);
}

void MagneticRectangleEffect::onResize(int fbWidth, int fbHeight) {
    m_impl->fbWidth = fbWidth;
    m_impl->fbHeight = fbHeight;
    float w = static_cast<float>(fbWidth);
    float h = static_cast<float>(fbHeight);
    m_impl->projection = glm::ortho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f);
}

void MagneticRectangleEffect::setSpeed(float speed) { m_impl->speed = speed; }
void MagneticRectangleEffect::setAmplitude(float amp) { m_impl->amplitude = amp; }
void MagneticRectangleEffect::setLineWidth(float width) { m_impl->lineWidth = width; }
void MagneticRectangleEffect::setCornerRadius(float radius) { m_impl->cornerRadius = radius; }
void MagneticRectangleEffect::setColorShift(float shift) { m_impl->colorShift = shift; }
void MagneticRectangleEffect::setScale(float scale) { m_impl->scale = scale; }